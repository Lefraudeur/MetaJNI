#ifdef _WIN32
    #include <Windows.h>
#elif defined(__linux__)
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
#endif

#include "logger/logger.hpp"
#include "meta_jni.hpp"
#include "mappings.hpp"
#include "jvmti/jvmti.hpp"
#include <thread>
#include <iostream>
#include "gui/gui.hpp"
#include "cache/cache.hpp"
#include "modules/modules.hpp"
#include "scheduler/scheduler.hpp"

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

static void do_with_jni(JavaVM* jvm)
{
    ::jvmti jvmti{ jvm };
    if (!jvmti)
        return;

    jni::frame frame{}; // every local ref follow this frame object lifetime

    maps::Class minecraftClass(jvmti.find_loaded_class(maps::MinecraftClient::get_name()));
    maps::URLClassLoader minecraftClassLoader(jvmti.get_class_ClassLoader(minecraftClass), jni::GLOBAL_REF);
    jni::set_custom_find_class([&minecraftClassLoader](const char* class_name) -> jclass
        {
            jni::frame frame{ 3 };
            JNIEnv* env = jni::get_env();

            std::string name = class_name;
            for (size_t it = name.find('/'); it != std::string::npos; it = name.find('/', it + 1))
                name[it] = '.';
            jclass found = minecraftClassLoader.loadClass(maps::String::create(name.c_str()));
            if (env->ExceptionCheck())
                env->ExceptionClear();

            return found;
        });

    scheduler main_scheduler{jvm};

    modules::init();
    gui::init(jvm);

    ::cache cache{};

    while (!is_uninject_key_pressed())
    {
        // concern : since we do not run our jni code in the game thread, isn't there a risk of race condition between the java code and the jni code ?
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

static void do_with_attached_thread(JavaVM* jvm, JNIEnv* env)
{
    if (!jni::init())
        return;
    jni::set_thread_env(env); //this is needed for every new thread that uses the lib

    do_with_jni(jvm);

    jni::shutdown();
}

static void do_with_logger()
{
    JavaVM* jvm = nullptr;
    JNI_GetCreatedJavaVMs(&jvm, 1, nullptr);
    if (!jvm)
    {
        logger::error("failed to get JavaVM*");
        return;
    }

    JNIEnv* env = nullptr;
    jvm->AttachCurrentThread((void**)&env, nullptr);
    if (!env)
    {
        logger::error("failed to attach current thread");
        return;
    }

    do_with_attached_thread(jvm, env);

    jvm->DetachCurrentThread();
}

static void mainThread(void* dll)
{
    logger::init();
    do_with_logger();
    logger::shutdown();
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