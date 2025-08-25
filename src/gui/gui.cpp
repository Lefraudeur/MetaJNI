#include "gui.hpp"
#include <Windows.h>
#include "../imgui/imgui_impl_opengl3.h"
#include "../imgui/imgui_impl_win32.h"
#include "../modules/modules.hpp"
#include "render_info.hpp"
#include <MinHook.h>

namespace
{
	typedef BOOL(WINAPI* wglSwapBuffers_t)(HDC);

	wglSwapBuffers_t wglSwapBuffers = nullptr;
	wglSwapBuffers_t original_wglSwapBuffers = nullptr;
	WNDPROC original_WndProc = nullptr;

	HWND window = nullptr;
	ImGuiContext* imGuiContext = nullptr;
	HGLRC original_context = nullptr;
	HGLRC new_context = nullptr;

	JavaVM* jvm = nullptr;

	std::atomic<bool> request_shutdown = false;

	int last_pressed_key = 0;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK detour_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HCURSOR arrow_cursor = LoadCursorA(nullptr, IDC_ARROW);

	if (msg == WM_KEYDOWN && wParam == VK_INSERT)
	{
		gui::draw = !gui::draw;
		ClipCursor(NULL);
	}

	if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN || msg == WM_MBUTTONDOWN)
		last_pressed_key = (int)wParam;

	if (gui::draw)
	{
		LRESULT result = ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
		if (result)
			return result;

		if (msg == WM_SIZE || msg == WM_ENTERSIZEMOVE || msg == WM_EXITSIZEMOVE || msg == WM_SIZING)
		{
			POINT pos{};
			GetCursorPos(&pos);
			LRESULT r = CallWindowProcA(original_WndProc, hWnd, msg, wParam, lParam);
			ClipCursor(NULL);
			SetCursorPos(pos.x, pos.y);
			SetCursor(arrow_cursor);
			return r;
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN || msg == WM_MBUTTONDOWN)
	{
		int key = (int)wParam;
		for (modules::module* module : modules::get_modules())
			if (module->get_keybind() && key && module->get_keybind() == key)
				module->enabled = !module->enabled;
	}

	return CallWindowProcA(original_WndProc, hWnd, msg, wParam, lParam);
}

static void uninit(HDC device)
{
	SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)original_WndProc);
	original_context = wglGetCurrentContext();
	wglMakeCurrent(device, new_context); // switch to the context we created, where imgui was initiated
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext(imGuiContext);
	wglMakeCurrent(device, original_context);
	wglDeleteContext(new_context);
}

static BOOL WINAPI detour_wglSwapBuffers(HDC device)
{
	static bool update_context = true;

	if (request_shutdown)
	{
		render_info::shutdown();
		MH_DisableHook(wglSwapBuffers);
		uninit(device);
		request_shutdown = false;
		return wglSwapBuffers(device);
	}

	HWND current_window = WindowFromDC(device);

	// if already init, and window changed, cleanup and reinit
	if (!update_context && window != current_window)
	{
		uninit(device);
		update_context = true;
	}

	if (update_context)
	{
		// set jni env for render thread
		JNIEnv* env = nullptr;
		if (jvm->GetEnv((void**)&env, JNI_VERSION_10) != JNI_OK || !env) logger::error("failed to get env for render thread");
		jni::set_thread_env(env);

		window = current_window;
		original_context = wglGetCurrentContext();
		new_context = wglCreateContext(device);
		wglMakeCurrent(device, new_context);

		original_WndProc = (WNDPROC)SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)detour_WndProc);

		imGuiContext = ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.IniFilename = nullptr;
		io.LogFilename = nullptr;

		io.Fonts->AddFontDefault();
		ImGui::StyleColorsDark();
		ImGui_ImplOpenGL3_Init();
		ImGui_ImplWin32_Init(window);
		update_context = false;
	}

	wglMakeCurrent(device, new_context);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();


	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	render_info::update();
	ImGui::Begin("Overlay", nullptr,
		ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);
	{
		for (modules::module* module : modules::get_modules())
			if (module->enabled)
				module->render();
	}
	ImGui::End();

	if (gui::draw)
	{
		ImGui::SetNextWindowBgAlpha(0.9f);
		ImGui::Begin("GUI", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		{
			for (int i = 0; modules::module* module : modules::get_modules())
			{
				ImGui::PushID(i);
				ImGui::Checkbox(module->get_name(), &module->enabled);
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
					module->display_options = !module->display_options;
				if (module->display_options)
				{
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFrameHeight());
					ImGui::BeginGroup();
					module->key_bind_selector();
					module->render_options();
					ImGui::EndGroup();
				}
				ImGui::PopID();
				++i;
			}
		}
		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	wglMakeCurrent(device, original_context);
	return original_wglSwapBuffers(device);
}

bool gui::init(JavaVM* jvm)
{
	::jvm = jvm;

	HMODULE opengl = GetModuleHandleA("opengl32.dll");
	if (!opengl) return false;
	wglSwapBuffers = (wglSwapBuffers_t)GetProcAddress(opengl, "wglSwapBuffers");

	if (!wglSwapBuffers)
		return false;

	if (MH_Initialize() != MH_OK) return false;

	if (MH_CreateHook(wglSwapBuffers, detour_wglSwapBuffers, (void**)&original_wglSwapBuffers) != MH_OK) return false;

	if (MH_EnableHook(wglSwapBuffers) != MH_OK) return false;

	return true;
}

void gui::shutdown()
{
	request_shutdown = true;
	while (request_shutdown);
	MH_RemoveHook(wglSwapBuffers);
	MH_Uninitialize();
}

int gui::get_last_pressed_key()
{
	return last_pressed_key;
}

void gui::reset_last_pressed_key()
{
	last_pressed_key = 0;
}

HWND gui::get_window()
{
	return window;
}
