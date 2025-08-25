#include "modules.hpp"

void modules::velocity::run(::cache& cache)
{
	if (cache.player.hurtTime.get() != 10) return;
	maps::Vec3d vec3d = cache.player.velocity.get();
	glm::dvec3 velocity = vec3d.to_glm_dvec3();
	if (velocity == last_changed_velocity) return;
	velocity *= (1.0 - multiplier);
	vec3d.set_glm_dvec3(velocity);
	last_changed_velocity = velocity;
}

void modules::velocity::render_options()
{
	ImGui::SliderFloat("multiplier", &multiplier, 0.01f, 1.0f, "%.2f");
}