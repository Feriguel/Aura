// ========================================================================== //
// File : thread_pool.hpp
//
// Author : Miguel Ângelo Crespo Ferreira
// ========================================================================== //
#pragma once
#ifndef AURACORE_THREAD_POOL
#define AURACORE_THREAD_POOL
// Internal includes.
#include <Aura/Core/types.hpp>
// Standard includes.
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
// External includes.
#include <iostream>

namespace Aura::Core
{
	/// <summary>
	/// Core thread pool, contains the base thread pool control system.
	/// </summary>
	class ThreadPool
	{
		// Basic function task type.
		using Task = std::function<void()>;
		// Thread pool.
		std::vector<std::thread> pool;
		// Thread event flag.
		std::condition_variable event_flag;
		// Thread event flag guard.
		std::mutex event_guard;
		// Stop flag.
		bool stopping;
		// Task queue.
		std::queue<Task> tasks;

		// ------------------------------------------------------------------ //
		// Set-up, tear-down pool.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Sets-up the thread pool and starts thread's cycle.
		/// </summary>
		explicit ThreadPool(std::size_t n_threads) :
			stopping(false)
		{
			for(std::size_t i = 0; i < n_threads; i++)
			{ pool.emplace_back([&] { cycle(); }); }
		}
		/// <summary>
		/// Stops the treads, wait for them to finish and tears-down the pool.
		/// </summary>
		~ThreadPool()
		{
			{
				std::unique_lock<std::mutex> lock { event_guard };
				stopping = true;
			}
			event_flag.notify_all();
			for(auto & thread : pool)
			{ thread.join(); }
		}

		// ------------------------------------------------------------------ //
		// Thread cycle and work enqueue methods.
		// ------------------------------------------------------------------ //
		public:
		/// <summary>
		/// Enqueue a task in the thread pool.
		/// </summary>
		template <class T>
		auto enqueue(T task) -> std::future<decltype(task())>
		{
			auto wrapper { std::make_shared<std::packaged_task<decltype(task()) ()>>(std::move(task)) };
			{
				std::unique_lock<std::mutex> lock { event_guard };
				tasks.emplace([=] { (*wrapper)(); });
			}
			event_flag.notify_one();
			return wrapper->get_future();
		}
		/// <summary>
		/// Retrieve thread indices and stores on given vector.
		/// </summary>
		void threadIndices(std::vector<std::thread::id> & indices)
		{
			indices.resize(pool.size());
			for(size_t i = 0; i < pool.size(); i++)
			{
				indices[i] = pool[i].get_id();
			}
		}
		private:
		/// <summary>
		/// Thread cycle.
		/// </summary>
		void cycle()
		{
			while(true)
			{
				Task task {};
				{
					std::unique_lock<std::mutex> lock { event_guard };
					event_flag.wait(lock, [&] { return stopping || !tasks.empty(); });
					if(stopping && tasks.empty()) { break; }
					task = std::move(tasks.front());
					tasks.pop();
				}
				task();
			}
		}
	};
}

#endif
