// ========================================================================== //
// File : environment.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#include <Aura/Core/render.hpp>
// Internal includes.
#include <Aura/Core/settings.hpp>
#include <Aura/Core/nucleus.hpp>
#include <Aura/Core/Environment/structures.hpp>
// Standard includes.
#include <cstdint>
#include <exception>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <shared_mutex>
#include <vector>
// External includes.
#pragma warning(disable : 26812)
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#pragma warning(default : 26812)

namespace Aura::Core
{
	// ------------------------------------------------------------------ //
	// Set-up and tear-down.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Sets-up the environment.
	/// </summary>
	Environment::Environment(Nucleus & nucleus) :
		core_nucleus(nucleus), scene(nullptr)
	{
		scene = new Scene();
	}
	/// <summary>
	/// Stops rendering and tears-down the core.
	/// </summary>
	Environment::~Environment() noexcept
	{
		delete scene;
	}

	// ------------------------------------------------------------------ //
	// Scene modification.
	// ------------------------------------------------------------------ //
	/// <summary>
	/// Replaces created scene with the new given scene. Takes ownership of the
	/// new scene.
	/// </summary>
	void Environment::replaceScene(Scene * const new_scene)
	{
		Scene * old_scene = scene;
		{
			std::unique_lock<std::shared_mutex> lock(guard);
			scene = new_scene;
		}
		delete old_scene;
	}
	/// <summary>
	/// Adds the given vertex to the scene. The vertex is moved to the
	/// returned address.
	/// </summary>
	Vertex * Environment::addVertex(Vertex & vertex, std::uint32_t & idx)
	{
		std::shared_lock<std::shared_mutex> scene_lock(guard);
		{
			std::unique_lock<std::mutex> vertex_lock(scene->vertices.guard);
			std::size_t const n_vertices = scene->vertices.data.size();
			if(n_vertices == EnvLimits::limit_vertices)
			{
				return nullptr;
			}
			idx = static_cast<std::uint32_t>(n_vertices);
			Vertex & new_vertex = scene->vertices.data.emplace_back(std::move(vertex));
			scene->vertices.updated = true;
			return &new_vertex;
		}
	}
	/// <summary>
	/// Adds the given transform to the scene. The transform is moved to the
	/// returned address.
	/// </summary>
	Transform * Environment::addTransform(Transform & transform, std::uint32_t & idx)
	{
		std::shared_lock<std::shared_mutex> scene_lock(guard);
		{
			std::unique_lock<std::mutex> transform_lock(scene->transforms.guard);
			std::size_t const n_transforms = scene->transforms.data.size();
			if(n_transforms == EnvLimits::limit_entities)
			{
				return nullptr;
			}
			idx = static_cast<std::uint32_t>(n_transforms);
			Transform & new_transform = scene->transforms.data.emplace_back(std::move(transform));
			scene->transforms.updated = true;
			return &new_transform;
		}
	}
	/// <summary>
	/// Adds the given primitive to the scene. The primitive is moved to the
	/// returned address.
	/// </summary>
	Primitive * Environment::addPrimitive(Primitive & primitive, std::uint32_t & idx)
	{
		std::shared_lock<std::shared_mutex> scene_lock(guard);
		{
			std::unique_lock<std::mutex> primitive_lock(scene->primitives.guard);
			std::size_t const n_primitives = scene->primitives.data.size();
			if(n_primitives == EnvLimits::limit_primitives)
			{
				return nullptr;
			}
			idx = static_cast<std::uint32_t>(n_primitives);
			Primitive & new_primitive = scene->primitives.data.emplace_back(std::move(primitive));
			scene->primitives.updated = true;
			return &new_primitive;
		}
	}
	/// <summary>
	/// Adds the given material to the scene. The material is moved to the
	/// returned address.
	/// </summary>
	Material * Environment::addMaterial(Material & material, std::uint32_t & idx)
	{
		std::shared_lock<std::shared_mutex> scene_lock(guard);
		{
			std::unique_lock<std::mutex> material_lock(scene->materials.guard);
			std::size_t const n_materials = scene->materials.data.size();
			if(n_materials == EnvLimits::limit_materials)
			{
				return nullptr;
			}
			idx = static_cast<std::uint32_t>(n_materials);
			Material & new_material = scene->materials.data.emplace_back(std::move(material));
			scene->materials.updated = true;
			return &new_material;
		}
	}
	/// <summary>
	/// Adds the given entity to the scene. The entity is moved to the
	/// returned address.
	/// </summary>
	Entity * Environment::addEntity(Entity & entity, std::uint32_t & idx)
	{
		std::shared_lock<std::shared_mutex> scene_lock(guard);
		{
			std::unique_lock<std::mutex> entity_lock(scene->entities.guard);
			std::size_t const n_entities = scene->entities.data.size();
			if(n_entities == EnvLimits::limit_entities)
			{
				return nullptr;
			}
			idx = static_cast<std::uint32_t>(n_entities);
			Entity & new_entity = scene->entities.data.emplace_back(entity);
			scene->entities.updated = true;
			return &new_entity;
		}
	}
	/// <summary>
	/// Creates a material and updates given index.
	/// Returns whenever a new material was created.
	/// </summary>
	bool Environment::newMaterial(Material & material, std::uint32_t & idx)
	{
		if(!addMaterial(material, idx))
		{
			return false;
		}
		return true;
	}
	/// <summary>
	/// Creates a vertex and updates given index.
	/// Returns whenever a new vertex was created.
	/// </summary>
	bool Environment::newVertex(Vertex & vertex, std::uint32_t & idx)
	{
		if(!addVertex(vertex, idx))
		{
			return false;
		}
		return true;
	}
	/// <summary>
	/// Creates an empty entity and updates given index.
	/// Returns whenever a new entity was created.
	/// </summary>
	bool Environment::newEntity(std::uint32_t const material_idx, std::uint32_t & idx)
	{
		std::uint32_t transform_idx { 0U };
		Transform transform {};
		if(!addTransform(transform, transform_idx))
		{
			return false;
		}
		Entity entity { transform_idx, material_idx, {} };
		if(!addEntity(entity, idx))
		{
			return false;
		}
		return true;
	}
	/// <summary>
	/// Adds a the given primitive to the scene. The primitive's material
	/// and transform indices are ignored and it is moved into the scene. 
	/// </summary>
	bool Environment::entityAddPrimitive(std::uint32_t const entity_idx, Primitive & primitive)
	{
		{
			std::shared_lock<std::shared_mutex> scene_lock(guard);
			std::unique_lock<std::mutex> entity_lock(scene->entities.guard);
			Entity & entity = scene->entities.data[entity_idx];
			primitive.material_idx = entity.material_idx;
			primitive.transform_idx = entity.transform_idx;
		}
		std::uint32_t primitive_idx { 0U };
		Primitive * p_primitive { addPrimitive(primitive, primitive_idx) };
		if(!p_primitive)
		{
			return false;
		}
		{
			std::shared_lock<std::shared_mutex> scene_lock(guard);
			std::unique_lock<std::mutex> entity_lock(scene->entities.guard);
			Entity & entity = scene->entities.data[entity_idx];
			{
				std::unique_lock<std::mutex> primitive_lock(scene->primitives.guard);
				entity.primitives.emplace_back(p_primitive);
			}
		}
		return true;
	}
	/// <summary>
	/// Sets material as entity's material.
	/// </summary>
	void Environment::entityMaterial(std::uint32_t const entity_idx, std::uint32_t const material_idx)
	{
		std::shared_lock<std::shared_mutex> scene_lock(guard);
		std::unique_lock<std::mutex> entity_lock(scene->entities.guard);
		Entity & entity = scene->entities.data[entity_idx];
		entity.material_idx = material_idx;
		{
			std::unique_lock<std::mutex> primitive_lock(scene->primitives.guard);
			for(std::size_t i { 0U }; i < entity.primitives.size(); ++i)
			{
				entity.primitives[i]->material_idx = material_idx;
			}
		}
	}
	/// <summary>
	/// Sets the entity translation matrix. Each vector component is a
	/// translation in respect to each axis.
	/// </summary>
	void Environment::entityTranslate(std::uint32_t const entity_idx, glm::vec3 const translate)
	{
		glm::mat4 translation = glm::translate(glm::identity<glm::mat4>(), translate);
		{
			std::shared_lock<std::shared_mutex> scene_lock(guard);
			std::unique_lock<std::mutex> entity_lock(scene->entities.guard);
			Entity & entity = scene->entities.data[entity_idx];
			{
				std::unique_lock<std::mutex> transform_lock(scene->transforms.guard);
				scene->transforms.data[entity.transform_idx].translation = translation;
			}
		}
	}
	/// <summary>
	/// Sets the entity scaling matrix. Each vector component is a scale
	/// in respect to each axis.
	/// </summary>
	void Environment::entityScale(std::uint32_t const entity_idx, glm::vec3 const scale)
	{
		glm::mat4 scaling = glm::scale(glm::identity<glm::mat4>(), scale);
		{
			std::shared_lock<std::shared_mutex> scene_lock(guard);
			std::unique_lock<std::mutex> entity_lock(scene->entities.guard);
			Entity & entity = scene->entities.data[entity_idx];
			{
				std::unique_lock<std::mutex> transform_lock(scene->transforms.guard);
				scene->transforms.data[entity.transform_idx].scaling = scaling;
			}
		}
	}
	/// <summary>
	/// Sets the entity rotation matrix. Each vector component is a rotation
	/// in respect to each axis.
	/// </summary>
	void Environment::entityRotate(std::uint32_t const entity_idx, glm::vec3 const rotate)
	{
		glm::mat4 rotation = glm::rotate(glm::rotate(glm::rotate(glm::identity<glm::mat4>(),
			rotate.x, glm::vec3(1.0f, 0.0f, 0.0f)), rotate.y, glm::vec3(0.0f, 1.0f, 0.0f)), rotate.z, glm::vec3(0.0f, 0.0f, 1.0f));
		{
			std::shared_lock<std::shared_mutex> scene_lock(guard);
			std::unique_lock<std::mutex> entity_lock(scene->entities.guard);
			Entity & entity = scene->entities.data[entity_idx];
			{
				std::unique_lock<std::mutex> transform_lock(scene->transforms.guard);
				scene->transforms.data[entity.transform_idx].rotation = rotation;
			}
		}
	}
	/// <summary>
	/// Loads the model at path into the given entity.
	/// </summary>
	bool Environment::entityLoadModel(std::uint32_t const entity_idx, std::string const path)
	{
		std::ifstream file {};
		if(!path.ends_with(".obj"))
		{
			std::cout << "Can only open OBJ files: " << path << std::endl;
			return false;
		}
		file.open(path);
		if(!file.is_open())
		{
			std::cout << "Could not open file: " << path << std::endl;
			return false;
		}
		// Read object.
		{
			std::string header {}, dump {}; Vertex v {}; Primitive p {};
			std::uint32_t vertex_idx {}; std::int64_t face_vertices {};
			std::vector<std::uint32_t> vertices {};
			while(true)
			{
				file >> header;
				// File is over
				if(file.eof())
				{
					break;
				}
				// Is a vertex line.
				if(header == "v")
				{
					file >> v.position.x >> v.position.y >> v.position.z;
					if(!addVertex(v, vertex_idx))
					{
						std::cout << "Vertex limit reached: " << path << std::endl;
						file.close(); return false;
					}
					vertices.emplace_back(vertex_idx);
				}
				// Is a face line.
				else if(header == "f")
				{
					std::string line {};
					std::getline(file, line);
					std::istringstream stream(line);
					face_vertices = std::count(line.begin(), line.end(), ' ');
					switch(face_vertices)
					{
					case 3:
						p.type = Primitive::Types::Triangle;
						stream >> vertex_idx >> dump;
						p.vertices.x = vertices[vertex_idx - 1];
						stream >> vertex_idx >> dump;
						p.vertices.y = vertices[vertex_idx - 1];
						stream >> vertex_idx >> dump;
						p.vertices.z = vertices[vertex_idx - 1];
						if(!entityAddPrimitive(entity_idx, p))
						{
							std::cout << "Primitive limit reached: " << path << std::endl;
							file.close(); return false;
						}
						break;
					case 4:
						p.type = Primitive::Types::Triangle;
						stream >> vertex_idx >> dump;
						p.vertices.x = vertices[vertex_idx - 1];
						stream >> vertex_idx >> dump;
						p.vertices.y = vertices[vertex_idx - 1];
						stream >> vertex_idx >> dump;
						p.vertices.z = vertices[vertex_idx - 1];
						if(!entityAddPrimitive(entity_idx, p))
						{
							std::cout << "Primitive limit reached: " << path << std::endl;
							file.close(); return false;
						}
						p.vertices.x = p.vertices.z;
						stream >> vertex_idx >> dump;
						p.vertices.z = vertices[vertex_idx - 1];
						if(!entityAddPrimitive(entity_idx, p))
						{
							std::cout << "Primitive limit reached: " << path << std::endl;
							file.close(); return false;
						}
						break;
					default:
						std::cout << "Loader only supports triangles or quads: " << path << std::endl;
						file.close();
						return false;
					}
				}
				// Skip line.
				else
				{
					file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				}
			}
		}
		file.close();
		return true;
	}
}
