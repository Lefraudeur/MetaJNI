#pragma once
#include <jni.h>
#include <Windows.h>

namespace gui
{
	inline bool draw = false;

	bool init(JavaVM* jvm);
	void shutdown();


	int get_last_pressed_key();
	void reset_last_pressed_key();
	HWND get_window();
}