#include "modules.hpp"
#include "../gui/gui.hpp"
#include <imgui/imgui.h>
#include <algorithm>
#include <iostream>

void modules::aimassist::run(::cache& cache)
{
	if (min_force_yaw > max_force_yaw) min_force_yaw = max_force_yaw;
	if (min_force_pitch > max_force_pitch) min_force_pitch = max_force_pitch;
	if (min_width_randomness > max_width_randomness) min_width_randomness = max_width_randomness;

	if ((require_left_click && !(GetKeyState(VK_LBUTTON) & 0x8000)) || gui::draw)
	{
		locked_target.clear_ref();
		return;
	}

	float tickDelta = cache.instance.renderTickCounter.get().tickDelta.get();
	if (tickDelta > 1.0f) return;

	maths::vector3d current_player_position = cache.player.get_position();
	maths::vector3d prev_player_position = cache.player.get_prev_position();
	maths::vector3d player_position = prev_player_position + (current_player_position - prev_player_position) * tickDelta;

	maths::angles current_player_angles = cache.player.get_angles();
	maths::angles prev_player_angles = cache.player.get_prev_angles();
	maths::angles player_angles = prev_player_angles + (current_player_angles - prev_player_angles) * tickDelta;

	maps::AbstractClientPlayerEntity selected_target{nullptr};
	maths::angles selected_target_angles_delta{};
	maths::vector3d selected_target_position{};
	for (maps::AbstractClientPlayerEntity& playerEntity : jni::array<maps::AbstractClientPlayerEntity>(cache.players.toArray()).to_vector())
	{
		if (!playerEntity || playerEntity.is_same_object(cache.player)) continue;

		maths::vector3d current_target_position = playerEntity.get_position();
		maths::vector3d prev_target_position = playerEntity.get_prev_position();
		maths::vector3d target_position = prev_target_position + (current_target_position - prev_target_position) * tickDelta;

		maths::vector3d player_target = (target_position - player_position);
		double distance = player_target.length();
		if (distance > max_distance || distance < min_distance) continue;

		maths::angles angles = player_target.get_angles();
		maths::angles angles_delta = (angles - player_angles).mod360();
		if (std::abs(angles_delta.yaw) > max_angle) continue;


		if (lock_target && locked_target && playerEntity.is_same_object(locked_target))
		{
			selected_target = playerEntity;
			selected_target_angles_delta = angles_delta;
			selected_target_position = target_position;
			break;
		}


		if (!selected_target || std::abs(angles_delta.yaw) < std::abs(selected_target_angles_delta.yaw))
		{
			selected_target = playerEntity;
			selected_target_angles_delta = angles_delta;
			selected_target_position = target_position;
		}
	}

	if (lock_target)
		locked_target = selected_target;
	if (!selected_target) return;

	maps::Box selected_target_bb = selected_target.boundingBox.get();

	maths::vector3d render_vec = (selected_target.get_position() - selected_target.get_prev_position()) * (1.f - tickDelta);
	maths::vector3d camera_pos = player_position + maths::vector3d{ 0.0, cache.player.getEyeHeight(cache.player.getPose()), 0.0 };
	double widthX = selected_target_bb.maxX.get() - selected_target_bb.minX.get();
	double widthZ = selected_target_bb.maxZ.get() - selected_target_bb.minZ.get();
	std::uniform_real_distribution<> dis(min_width_randomness, max_width_randomness);
	std::uniform_real_distribution<> dis2(min_force_yaw, max_force_yaw);
	std::uniform_real_distribution<> dis3(min_force_pitch, max_force_pitch);
	double new_width = (1.0 + width * (-1.0 + dis(gen)));
	double x = (widthX / 2.0) * new_width;
	double z = (widthZ / 2.0) * new_width;
	maths::vector3d box_min = maths::vector3d{ selected_target_bb.minX.get() + x, selected_target_bb.minY.get(), selected_target_bb.minZ.get() + z } - render_vec;
	maths::vector3d box_max = maths::vector3d{ selected_target_bb.maxX.get() - x, selected_target_bb.maxY.get(), selected_target_bb.maxZ.get() - z } - render_vec;
	aimassist::intersection intersection = find_intersection(camera_pos, player_angles.get_vector(), box_min, box_max);

	if (intersection.intersects) return;

	maths::angles required_angles = (intersection.point - camera_pos).get_angles();
	maths::angles delta = (required_angles - player_angles).mod360();

	aimassist::intersection intersection2 = find_intersection(camera_pos, (player_angles + maths::angles{ float((delta.yaw > 0.0f ? 1.f : -1.f) * (std::abs(delta.yaw) + 0.1)), 0.f }).get_vector(), box_min, box_max);

	delta.yaw = (delta.yaw > 0.0f ? 1.f : -1.f) * std::abs(std::log(std::abs(delta.yaw + std::exp(dis2(gen))))) * dis2(gen);
	if (intersection2.intersects)
		delta.pitch = 0.0f;
	else
		delta.pitch = (delta.pitch > 0.0f ? 1.f : -1.f) * std::abs(std::log(std::abs(delta.pitch + std::exp(dis3(gen))))) * dis3(gen);

	double sens = maps::Double(cache.options.mouseSensitivity.get().value.get()).doubleValue();
	double deltaX = (delta.yaw) / (8.0 * 0.15 * std::pow(sens * 0.6 + 0.2, 3));
	double deltaY = (delta.pitch) / (8.0 * 0.15 * std::pow(sens * 0.6 + 0.2, 3));
	cache.mouse.cursorDeltaX = deltaX;
	cache.mouse.cursorDeltaY = deltaY;
}

void modules::aimassist::on_disable(::cache& cache)
{
	locked_target.clear_ref();
}

void modules::aimassist::render_options()
{
	ImGui::Checkbox("lock target", &lock_target);
	ImGui::Checkbox("require left click", &require_left_click);
	ImGui::SliderFloat("min distance", &min_distance, 0.0f, 6.0f, "%.2f");
	ImGui::SliderFloat("max distance", &max_distance, 0.1f, 6.0f, "%.2f");
	ImGui::SliderFloat("max angle", &max_angle, 0.1f, 180.0f, "%.2f");
	ImGui::SliderFloat("width", &width, 0.1f, 1.0f, "%.2f");
	ImGui::SliderFloat("min width random", &min_width_randomness, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("max width random", &max_width_randomness, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("min force yaw", &min_force_yaw, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("max force yaw", &max_force_yaw, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("min force pitch", &min_force_pitch, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("max force pitch", &max_force_pitch, 0.0f, 1.0f, "%.2f");
}

modules::aimassist::intersection modules::aimassist::find_intersection(maths::vector3d ray_start, maths::vector3d ray_dir, maths::vector3d box_min, maths::vector3d box_max)
{
	double ts[] = { (box_min.x - ray_start.x) / ray_dir.x , (box_min.y - ray_start.y) / ray_dir.y , (box_min.z - ray_start.z) / ray_dir.z ,
	(box_max.x - ray_start.x) / ray_dir.x , (box_max.y - ray_start.y) / ray_dir.y , (box_max.z - ray_start.z) / ray_dir.z };

	double best_distance = -1.0;
	maths::vector3d best_intersection{};
	for (double t : ts)
	{
		if (t < 0.0) continue;
		maths::vector3d plane_intersection = ray_start + ray_dir * t;
		bool between_x = plane_intersection.x >= box_min.x - 0.00000001 && plane_intersection.x <= box_max.x + 0.00000001;
		bool between_y = plane_intersection.y >= box_min.y - 0.00000001 && plane_intersection.y <= box_max.y + 0.00000001;
		bool between_z = plane_intersection.z >= box_min.z - 0.00000001 && plane_intersection.z <= box_max.z + 0.00000001;
		if (between_x && between_y && between_z) return { plane_intersection, true };

		maths::vector3d side_intersection = plane_intersection;

		if (side_intersection.x < box_min.x) side_intersection.x = box_min.x;
		else if (side_intersection.x > box_max.x) side_intersection.x = box_max.x;

		if (side_intersection.y < box_min.y) side_intersection.y = box_min.y;
		else if (side_intersection.y > box_max.y) side_intersection.y = box_max.y;

		if (side_intersection.z < box_min.z) side_intersection.z = box_min.z;
		else if (side_intersection.z > box_max.z) side_intersection.z = box_max.z;



		double distance = (side_intersection - plane_intersection).length();

		if (best_distance == -1.0 || distance < best_distance)
		{
			best_distance = distance;
			best_intersection = side_intersection;
		}
	}

	return { best_intersection, false };
}