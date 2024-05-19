#ifdef _WIN32
    #include <Windows.h>
#elif defined(__linux__)
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
#endif

#include "meta_jni.hpp"
#include "mappings.hpp"
#include <thread>
#include <iostream>


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
#ifdef _WIN32
    AllocConsole();
    FILE* buff1 = nullptr;
    freopen_s(&buff1, "CONOUT$", "w", stdout);
#elif defined(__linux__)
    display = XOpenDisplay(NULL);
#endif

    JavaVM* jvm = nullptr;
    JNI_GetCreatedJavaVMs(&jvm, 1, nullptr);
    JNIEnv* env = nullptr;
    jvm->AttachCurrentThread((void**)&env, nullptr);
    jni::init();

    jni::set_thread_env(env); //this is needed for every new thread that uses the lib

    env->PushLocalFrame(100); //every local ref created after this will be deleted on PopLocalFrame

    maps::Minecraft Minecraft{};
    maps::EntityPlayerSP EntityPlayerSP{};
    maps::String String{};

    std::cout << "injected\n";
    std::cout << Minecraft.get_name() << '\n';
    std::cout << Minecraft.get_signature() << '\n';
    maps::Minecraft theMinecraft = Minecraft.theMinecraft.get();
    maps::Minecraft g_theMinecraft = maps::Minecraft(theMinecraft, true);
    std::cout << "display width test: " << theMinecraft.displayWidth.get() << '\n';
    theMinecraft.displayWidth = 100;
    std::cout << "display width test after change: " << theMinecraft.displayWidth.get() << '\n';
    theMinecraft.clickMouse();
    std::cout << Minecraft.clickMouse.get_signature() << '\n';

    theMinecraft.resize(800, 600);

    maps::EntityPlayerSP thePlayer = theMinecraft.thePlayer.get();
    thePlayer.sendChatMessage(String.create("test"));
    maps::String clientBrand = thePlayer.getClientBrand.call();
    std::cout << clientBrand.to_string() << '\n';
    jni::array<maps::EntityPlayerSP> testArray = jni::array<maps::EntityPlayerSP>::create({});
    std::cout << "test array: " << jobject(testArray) << '\n';

    maps::WorldClient theWorld = theMinecraft.theWorld.get();
    std::vector<maps::EntityPlayer> playerEntities = jni::array<maps::EntityPlayer>(theWorld.playerEntities.get().toArray()).to_vector();

    for (maps::EntityPlayer& p : playerEntities)
    {
        std::cout << p.getName().to_string() << ' ' << p.getHealth() << '\n';
    }

    maps::URL url = maps::URL::new_object(&maps::URL::constructor, String.create("http://www.example.com/docs/resource1.html"));
    std::cout << url.toString().to_string() << '\n';

    env->PopLocalFrame(nullptr);

    while (!is_uninject_key_pressed())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    jni::shutdown();
    jvm->DetachCurrentThread();

#ifdef _WIN32
    fclose(buff1);
    FreeConsole();
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