#pragma once
#include "../cache/cache.hpp"
#include <vector>
#include <random>
#include "../utils/utils.hpp"
#include <atomic>
#include <memory>
#include <glm/vec3.hpp>
#include <mutex>
#include <imgui.h>

namespace modules
{
	void init();
	void shutdown(::cache& cache);

	class module
	{
	public:
		module(const char* name = "no name");
		virtual ~module() = default;

		virtual void run(::cache& cache); // called periodically, if module is enabled (dll thread)
		virtual void render_options(); // extra imgui widgets (render thread)
		virtual void render(); // module specific overlay (render thread)
		virtual void on_enable(::cache& cache); // (dll thread)
		virtual void on_disable(::cache& cache); // called on uninject if cache is valid (if in game) (dll thread)

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
		maps::AbstractClientPlayerEntity locked_target{ jni::GLOBAL };
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

	class block_esp : public module
	{

	public:
		block_esp() : module("block esp"),
			mtx(),
			render_data(),
			render_data_task_id(),
			radius(20), 
			blocks_mutex(),
			blocks(),
			name_buff{'\0'},
			color_buff{ 0.f, 0.f, 0.f, 1.f }
		{}

		void on_enable(::cache& cache) override;
		void on_disable(::cache& cache) override;
		void render() override;
		void render_options() override;

	private:

		struct block
		{
			block(const glm::vec3& block_pos, const ImVec4& color);

			struct quad_2d
			{
				glm::vec2 p0;
				glm::vec2 p1;
				glm::vec2 p2;
				glm::vec2 p3;

				bool is_valid() const;
			};

			struct quad
			{
				glm::vec3 p0;
				glm::vec3 p1;
				glm::vec3 p2;
				glm::vec3 p3;

				quad_2d to_quad_2d() const;
			};
			quad faces[6];

			ImVec4 color;

			std::array<quad_2d, 6> get_screen_faces() const;
		};

		std::mutex mtx;
		std::vector<block> render_data;

		std::atomic<int> radius;
		static constexpr int NAME_SIZE = 256;
		struct block_name_color
		{
			std::array<char, NAME_SIZE> name;
			ImVec4 color;
		};
		std::mutex blocks_mutex;
		std::vector<block_name_color> blocks;
		std::array<char, NAME_SIZE> name_buff;
		float color_buff[4];


		int render_data_task_id;
	};

	class velocity : public module
	{
	public:
		velocity() : module("Velocity"), multiplier(1.f), last_changed_velocity() {}

		void run(::cache& cache) override;
		void render_options() override;

	private:
		float multiplier;
		glm::dvec3 last_changed_velocity;
	};
}