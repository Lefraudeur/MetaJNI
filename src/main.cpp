#ifdef _WIN32
    #include <Windows.h>
#elif defined(__linux__)
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
#endif

#include <meta_jni.hpp>
#include "jvmti/jvmti.hpp"
#include <thread>
#include <iostream>
#include "gui/gui.hpp"
#include "cache/cache.hpp"
#include "modules/modules.hpp"

#ifdef __linux__
static Display* display = nullptr;
#endif

static bool is_uninject_key_pressed()
{
#ifdef _WIN32
    return GetAsyncKeyState(VK_END);
#elif __linux__
    static KeyCode keycode = XKeysymToKeycode(display, XK_End);

    char key_states[32] = { '\0' };
    XQueryKeymap(display, key_states);

    // <<3 same as /8 (logic 2^3 = 8) and &7 same as %8 (idk y)
    return (key_states[keycode << 3] & (1 << (keycode & 7)));
#endif
}

static void mainThread(void* dll)
{
#if defined(_WIN32) /* && !defined(NDEBUG) */
    AllocConsole();
    FILE* buff1, * buff2, *buff3 = nullptr;
    freopen_s(&buff1, "CONOUT$", "w", stdout);
    freopen_s(&buff2, "CONOUT$", "w", stderr);
    freopen_s(&buff3, "CONIN$", "r", stdin);
#elif defined(__linux__)
    display = XOpenDisplay(NULL);
#endif
{
    JavaVM* jvm = nullptr;
    JNI_GetCreatedJavaVMs(&jvm, 1, nullptr);
    JNIEnv* env = nullptr;
    jvm->AttachCurrentThread((void**)&env, nullptr);
    jni::init();
    jni::set_thread_env(env); //this is needed for every new thread that uses the lib

    {
        ::jvmti jvmti{ jvm };

        jni::frame frame{}; // every local ref follow this frame object lifetime

        java::lang::Class minecraftClass = jvmti.find_loaded_class(net::minecraft::client::Minecraft::get_name());
        java::net::URLClassLoader minecraftClassLoader(jvmti.get_class_ClassLoader(minecraftClass), true);
        jni::set_custom_find_class([&minecraftClassLoader](const char* class_name) -> jclass
            {
                jni::frame frame{ 3 };
                JNIEnv* env = jni::get_env();

                std::string name = class_name;
                for (size_t it = name.find('/'); it != std::string::npos; it = name.find('/', it + 1))
                    name[it] = '.';
                jclass found = minecraftClassLoader.findClass(java::lang::String::create(name.c_str()));
                if (env->ExceptionCheck())
                    env->ExceptionClear();

                return found;
            });

        modules::init();
        gui::init();

        ::cache cache{};


        while (!is_uninject_key_pressed())
        {
            if (!cache.update())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(64));
                continue;
            }

            for (modules::module* module : modules::get_modules())
            {
                jni::frame frame{ 32 };
                if (module->enabled)
                {
                    if (!module->prev_enabled)
                    {
                        module->prev_enabled = true;
                        module->on_enable(cache);
                    }
                    module->run(cache);
                }
                else if (module->prev_enabled)
                {
                    module->prev_enabled = false;
                    module->on_disable(cache);
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }

        gui::shutdown();
        modules::shutdown(cache);
    }


    jni::shutdown();
    jvm->DetachCurrentThread();
}

#if defined(_WIN32) /* && !defined(NDEBUG) */
    fclose(buff1);
    fclose(buff2);
    fclose(buff3);
    FreeConsole();
#endif

#if defined(_WIN32)
    FreeLibraryAndExitThread((HMODULE)dll, 0);
#elif defined(__linux__)
    XCloseDisplay(display);
#endif
    return;
}

#ifdef _WIN32

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved)  // reserved
{
    // Perform actions based on the reason for calling.
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        // Initialize once for each new process.
        // Return FALSE to fail DLL load.
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)mainThread, hinstDLL, 0, 0));
        break;

    case DLL_THREAD_ATTACH:
        // Do thread-specific initialization.
        break;

    case DLL_THREAD_DETACH:
        // Do thread-specific cleanup.
        break;

    case DLL_PROCESS_DETACH:

        if (lpvReserved != nullptr)
        {
            break; // do not do cleanup if process termination scenario
        }

        // Perform any necessary cleanup.
        break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

#elif defined(__linux__)

void __attribute__((constructor)) onload_linux()
{
    pthread_t thread = 0U;
    pthread_create(&thread, nullptr, (void* (*)(void*))mainThread, nullptr);
    return;
}
void __attribute__((destructor)) onunload_linux()
{
    return;
}

#endif