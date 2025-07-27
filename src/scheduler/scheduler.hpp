#pragma once
#include "../maths/maths.hpp"
#include <jni.h>
#include <thread>
#include <functional>
#include <chrono>
#include <vector>
#include <mutex>
#include <atomic>

class scheduler
{
public:
	scheduler(JavaVM* jvm);
	scheduler(const scheduler& other) = delete;
	scheduler(scheduler&& other) = delete;
	~scheduler();

	struct task
	{
	friend scheduler;

		task(const std::function<void()>& action, std::chrono::milliseconds every_x_ms);

	private:
		std::function<void()> action;
		maths::timer timer;
		int task_id;

		static int next_task_id;
	};

	int schedule(const task& t);
	void unschedule(int task_id);

	static scheduler* get_main();
private:
	std::mutex tasks_mutex;
	std::atomic_bool run_tasks;
	std::vector<task> tasks;
	std::thread task_consumer_thread;

	static scheduler* main;
};