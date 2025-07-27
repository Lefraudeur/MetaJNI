#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace render_info // must be used from render thread
{
	void update();
	// render info might hold some jni references, call shutdown to delete them
	void shutdown();

	bool is_valid();

	glm::vec2 world_to_screen(const glm::vec3& world_pos);
}