#include "modules.hpp"
#include "../gui/gui.hpp"
#include <imgui/imgui.h>
#include "../utils/utils.hpp"

static std::vector<modules::module*> _modules{};

void modules::init()
{
	jni::frame frame{};
	_modules =
	{
		new fastbreak(),
		new test()
	};
}

void modules::shutdown(::cache& cache)
{
	jni::frame frame{};
	for (modules::module* module : _modules)
	{
		if (module->enabled && cache.is_valid()) module->on_disable(cache);
		delete module;
	}
}

const std::vector<modules::module*>& modules::get_modules()
{
	return _modules;
}


modules::module::module(const char* name) : 
	name(name)
{
}

void modules::module::run(::cache& cache)
{
}

void modules::module::render_options()
{
}

void modules::module::on_enable(::cache& cache)
{
}

void modules::module::on_disable(::cache& cache)
{
}

int modules::module::get_keybind()
{
	return keybind;
}

const char* modules::module::get_name()
{
	return name;
}

void modules::module::key_bind_selector()
{
	utils::null_string display = (scanning ? "scanning..." : keycode_to_string(keybind));
	if (ImGui::Button((utils::null_string("keybind: ") + display + "##" + name).data))
	{
		scanning = true;
		gui::reset_last_pressed_key();
	}

	if (scanning)
	{
		int last_pressed = gui::get_last_pressed_key();
		if (!last_pressed) return;

		if (last_pressed == VK_ESCAPE)
			last_pressed = 0;

		keybind = last_pressed;
		scanning = false;
	}
}

utils::null_string modules::module::keycode_to_string(int keycode)
{
	if (!keycode)
		return "none";

	switch (keycode)
	{
	case VK_MENU:
	case VK_LMENU:
		return "LALT";
	case VK_RMENU:
		return "RALT";
	case VK_SPACE:
		return "SPACE";
	case VK_LBUTTON:
		return "LBUTTON";
	case VK_RBUTTON:
		return "RBUTTON";
	case VK_MBUTTON:
		return "MBUTTON";
	case VK_TAB:
		return "TAB";
	}

	char character = MapVirtualKeyA(keybind, MAPVK_VK_TO_CHAR);
	if (!character)
		return "?";
		
	utils::null_string str{ "a" };
	str.data[0] = character;
	return str;
}

void modules::test::on_enable(::cache& cache)
{
	cache.theMinecraft.ingameGUI.get().getChatGUI().printChatMessage
	(
		(net::minecraft::util::IChatComponent)net::minecraft::util::ChatComponentText::new_object
		(
			&net::minecraft::util::ChatComponentText::_init_, java::lang::String::create("§caaa")
		)
	);
}
