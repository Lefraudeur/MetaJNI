#include "modules.hpp"
#include "../gui/gui.hpp"
#include <imgui/imgui.h>
#include <iostream>

void modules::triggerbot::run(::cache& cache)
{
	if (gui::draw)
	{
		prev_attack_tick = -1;
		return;
	}
	if (sword_only)
	{
		maps::Item item = cache.player.getMainHandStack().item.get();
		if (item && !item.is_instance_of<maps::SwordItem>())
		{
			prev_attack_tick = -1;
			return;
		}
	}

	maps::HitResult result = cache.instance.crosshairTarget.get();
	maps::HitResult$Type type = result.getType();
	maps::HitResult$Type ENTITY = maps::HitResult$Type{}.ENTITY.get();

	if (!type || !type.is_same_object(ENTITY))
	{
		prev_attack_tick = -1;
		return;
	}
	maps::Entity entity = maps::EntityHitResult(result).entity.get();
	if (!entity || !entity.is_instance_of<maps::LivingEntity>() || !entity.isAlive())
	{
		prev_attack_tick = -1;
		return;
	}

	int last_attack_tick = cache.player.lastAttackedTicks.get();
	if (last_attack_tick < prev_attack_tick)
		prev_attack_tick = -1;

	float progress = cache.player.getAttackCooldownProgress(0.5f);
	if (progress == 1.f && last_attack_tick != prev_attack_tick)
	{
		if (wait_critical && (cache.player.onGround.get() == JNI_FALSE || (GetKeyState(VK_SPACE) & 0x8000) ) && cache.player.fallDistance.get() <= 0.0f)
		{
			prev_attack_tick = -1;
			return;
		}
		POINT cursorPos{};
		GetCursorPos(&cursorPos);
		ScreenToClient(gui::get_window(), &cursorPos);
		PostMessageA(gui::get_window(), WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(cursorPos.x, cursorPos.y));
		PostMessageA(gui::get_window(), WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(cursorPos.x, cursorPos.y));
		prev_attack_tick = last_attack_tick;
	}

}

void modules::triggerbot::render_options()
{
	ImGui::Checkbox("sword only##idtrigger", &sword_only);
	ImGui::Checkbox("wait critical", &wait_critical);
}

void modules::triggerbot::on_disable(::cache& cache)
{
	prev_attack_tick = -1;
}
