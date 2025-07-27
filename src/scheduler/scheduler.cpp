#include "scheduler.hpp"
#include "../meta_jni.hpp"

int scheduler::task::next_task_id = 0;
scheduler* scheduler::main = nullptr;

scheduler::scheduler(JavaVM* jvm) : 
	tasks_mutex(),
	run_tasks(true),
	tasks(),
	task_consumer_thread
	([jvm, this]()
		{
			JNIEnv* env = nullptr;
			jvm->AttachCurrentThread((void**)&env, nullptr);
			jni::set_thread_env(env);

			while (run_tasks)
			{
				{
					std::lock_guard lock{ tasks_mutex };
					for (task& t : tasks)
					{
						if (t.timer.is_elapsed())
							t.action();
					}
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			}

			jvm->DetachCurrentThread();
		}
	)
{
	if (!main) main = this;
}

scheduler::~scheduler()
{
	if (!task_consumer_thread.joinable()) return;
	run_tasks = false;
	task_consumer_thread.join();
}

int scheduler::schedule(const task& t)
{
	std::lock_guard lock{ tasks_mutex };
	tasks.push_back(t);
	return t.task_id;
}

void scheduler::unschedule(int task_id)
{
	std::lock_guard lock{ tasks_mutex };
	for (std::vector<task>::iterator it = tasks.begin(); it != tasks.end(); ++it)
	{
		if (it->task_id != task_id) continue;
		tasks.erase(it);
		break;
	}
}

scheduler* scheduler::get_main()
{
	return main;
}

scheduler::task::task(const std::function<void()>& action, std::chrono::milliseconds every_x_ms) :
	action(action),
	timer(every_x_ms),
	task_id(next_task_id++)
{
}
