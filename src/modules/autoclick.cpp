#include "modules.hpp"
#include <imgui/imgui.h>
#include "../gui/gui.hpp"

void modules::autoclick::run(::cache& cache)
{
	static maths::timer timer{std::chrono::milliseconds(int(1000.0 / (min_cps * 2.0)))};

	if (min_cps > max_cps) min_cps = max_cps;

	if (!(GetKeyState(VK_LBUTTON) & 0x8000) || gui::draw)
	{
		down = false;	
		return;
	}
	if (!timer.is_elapsed()) return;



	POINT cursorPos{};
	GetCursorPos(&cursorPos);
	ScreenToClient(gui::get_window(), &cursorPos);
	PostMessageA(gui::get_window(), (down ? WM_LBUTTONDOWN : WM_LBUTTONUP), MK_LBUTTON, MAKELPARAM(cursorPos.x, cursorPos.y));
	down = !down;

	double max_delay = 1000.0 / (min_cps * 2.0);
	double min_delay = 1000.0 / (max_cps * 2.0);
	std::uniform_real_distribution<> dis(min_delay, max_delay);
	timer.set_every(std::chrono::milliseconds(int(dis(gen))));
}

void modules::autoclick::render_options()
{
	ImGui::SliderFloat("min cps", &min_cps, 0.1f, 150.0f);
	ImGui::SliderFloat("max cps", &max_cps, 0.1f, 150.0f);
}

void modules::autoclick::on_disable(::cache& cache)
{
	down = false;
}
