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

	class hitbox : public module
	{
	public:
		hitbox() : module("Hitbox") {}

		void run(::cache& cache) override;
		void render_options() override;
		void on_disable(::cache& cache) override;

	private:
		float expand = 0.1f;
		bool sword_only = false;
		bool prev_enabled2 = false;
	};

	class autoclick : public module
	{
	public:
		autoclick() : module("Autoclick") {}

		void run(::cache& cache) override;
		void render_options() override;
		void on_disable(::cache& cache) override;

	private:
		float min_cps = 7.0;
		float max_cps = 14.0;
		bool down = false;
	};

	class triggerbot : public module
	{
	public:
		triggerbot() : module("Triggerbot") {};

		void run(::cache& cache) override;
		void render_options() override;
		void on_disable(::cache& cache) override;

	private:
		bool sword_only = true;
		bool wait_critical = true;
		int prev_attack_tick = -1;
	};

	class aimassist : public module
	{
	public:
		aimassist() : module("Aimassist") {}

		void run(::cache& cache) override;
		void on_disable(::cache& cache) override;
		void render_options() override;

	private:
		float min_distance = 0.0f;
		float max_distance = 6.0f;
		float max_angle = 80.0f;
		float width = 1.0f;
		float min_width_randomness = 0.0f;
		float max_width_randomness = 0.0f;
		float min_force_yaw = 0.4f;
		float max_force_yaw = 1.0f;
		float min_force_pitch = 0.4f;
		float max_force_pitch = 1.0f;

		struct intersection
		{
			maths::vector3d point;
			bool intersects;
		};
		intersection find_intersection(maths::vector3d ray_start, maths::vector3d ray_dir, maths::vector3d box_min, maths::vector3d box_max);

		bool lock_target = true; // continue aiming the same target even if a closer target exist
		bool require_left_click = true;
		maps::AbstractClientPlayerEntity locked_target{ nullptr, true };
	};

	class reach : public module
	{
	public:
	private:
	};

	class glow_esp : public module
	{
	public:
		glow_esp() : module("glow esp") {}

		void run(::cache& cache) override;
		void on_disable(::cache& cache) override;
	private:
	};

	class keepsprint : public module
	{
	public:
		keepsprint() : module("keep sprint") {}

		void run(::cache& cache) override;
	};
}