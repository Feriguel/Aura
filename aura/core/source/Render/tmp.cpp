// ========================================================================== //
// File : tmp.cpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
// This is all the previous code.

struct Vertex
{
	gpu_vec4 position;
};

struct Material
{
	enum struct Types : gpu_uint
	{
		Bounding = 0,
		Test = 1,
		Diffuse = 2,
		Specular = 3
	};
	gpu_vec4 colour {};
	gpu_real refractive_index {};
	gpu_real fuzziness {};
	Material::Types type {};
};

struct Primitive
{
	enum struct Types : gpu_uint
	{
		Empty = 0,
		Sphere = 1,
		Cuboid = 2,
		Triangle = 3,
		Quad = 4
	};
	gpu_uvec4 vertex_indexes {};
	gpu_real radius {};
	gpu_uint material_index {};
	gpu_uint transform_index {};
	Primitive::Types type {};
};