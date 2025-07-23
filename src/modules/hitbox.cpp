#include "modules.hpp"
#include <imgui/imgui.h>
#include <iostream>

void modules::hitbox::run(::cache& cache)
{
	if (sword_only)
	{
		maps::Item item = cache.player.getMainHandStack().item.get();
		if (!item || !item.is_instance_of<maps::SwordItem>())
		{
			if (prev_enabled2)
			{
				// on disable
				on_disable(cache);
			}
			return;
		}
	}
	prev_enabled2 = true;

	for (const maps::AbstractClientPlayerEntity& playerEntity : jni::array<maps::AbstractClientPlayerEntity>(cache.players.toArray()).to_vector())
	{
		if (playerEntity.is_same_object(cache.player)) continue;

		maps::Box boundingBox = playerEntity.boundingBox.get();

		double minX = boundingBox.minX.get();
		double maxX = boundingBox.maxX.get();

		double minY = boundingBox.minY.get();
		double maxY = boundingBox.maxY.get();

		double minZ = boundingBox.minZ.get();
		double maxZ = boundingBox.maxZ.get();

		double width = maxX - minX;
		double height = maxY - minY;

		if (width > 0.601 || height > 1.801) continue;

		boundingBox.minX = minX - expand;
		boundingBox.maxX = maxX + expand;

		boundingBox.minY = minY - expand;
		boundingBox.maxY = maxY + expand;

		boundingBox.minZ = minZ - expand;
		boundingBox.maxZ = maxZ + expand;
	}
}

void modules::hitbox::render_options()
{
	ImGui::SliderFloat("expand", &expand, 0.01f, 2.0f, "%.2f");
	ImGui::Checkbox("sword only", &sword_only);
}

void modules::hitbox::on_disable(::cache& cache)
{
	prev_enabled2 = false;
	for (const maps::AbstractClientPlayerEntity& playerEntity : jni::array<maps::AbstractClientPlayerEntity>(cache.players.toArray()).to_vector())
	{
		if (playerEntity.is_same_object(cache.player)) continue;

		maps::Box boundingBox = playerEntity.boundingBox.get();

		double minX = boundingBox.minX.get();
		double maxX = boundingBox.maxX.get();

		double minY = boundingBox.minY.get();
		double maxY = boundingBox.maxY.get();

		double minZ = boundingBox.minZ.get();
		double maxZ = boundingBox.maxZ.get();

		double width = maxX - minX;
		double height = maxY - minY;

		if (std::abs(0.6 - width) < 0.001 && std::abs(1.8 - height) < 0.001) continue;

		double center_x = minX + width / 2.0;
		boundingBox.minX = center_x - 0.3;
		boundingBox.maxX = center_x + 0.3;

		double center_y = minY + height / 2.0;
		boundingBox.minY = center_y - 0.9;
		boundingBox.maxY = center_y + 0.9;

		double center_z = minZ + width / 2.0;
		boundingBox.minZ = center_z - 0.3;
		boundingBox.maxZ = center_z + 0.3;
	}
}
