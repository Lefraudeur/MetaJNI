#pragma once
#include <Windows.h>

namespace gui
{
	inline bool draw = false;

	bool init();
	void shutdown();


	int get_last_pressed_key();
	void reset_last_pressed_key();
	HWND get_window();
}