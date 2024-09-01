#pragma once
#include "../cache/cache.hpp"
#include <vector>
#include <random>
#include "../utils/utils.hpp"

namespace modules
{
	void init();
	void shutdown(::cache& cache);

	class module
	{
	public:
		module(const char* name = "no name");
		virtual ~module() = default;

		virtual void run(::cache& cache); // called periodically, if module is enabled
		virtual void render_options(); // extra imgui widgets
		virtual void on_enable(::cache& cache);
		virtual void on_disable(::cache& cache); // called on uninject if cache is valid (if in game)

		int get_keybind();
		const char* get_name();
		void key_bind_selector();

		bool display_options = false;
		bool enabled = false;
		bool prev_enabled = false; // used to detect and send on_enable / on_disable events

	protected:
		int keybind = 0;
		std::random_device rd{};
		std::mt19937_64 gen{ rd() };

	private:
		utils::null_string keycode_to_string(int keycode);

		bool scanning = false;
		const char* name = "no name";
	};

	const std::vector<module*>& get_modules();

	class fastbreak : public module
	{
	public:
		fastbreak() : module("fast break") {}

		void run(::cache& cache) override;
	};

	class test : public module
	{
	public:
		test() : module("test") {}

		void on_enable(::cache& cache) override;
	};
}