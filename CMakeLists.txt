# CMakeList.txt : CMake project for MetaJNI2, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  cmake_policy(SET CMP0091 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("MetaJNI")

# Add source to this project's executable.
add_library(MetaJNI SHARED "src/main.cpp" "src/jvmti/jvmti.cpp" "src/gui/gui.cpp" 
	"src/imgui/imgui.cpp" "src/imgui/imgui_demo.cpp" "src/imgui/imgui_draw.cpp" "src/imgui/imgui_impl_opengl3.cpp" "src/imgui/imgui_impl_win32.cpp" "src/imgui/imgui_tables.cpp" "src/imgui/imgui_widgets.cpp" 
	"src/cache/cache.cpp" 
	"src/modules/modules.cpp" 
	"src/modules/hitbox.cpp" 
	"src/modules/aimassist.cpp" 
	"src/maths/maths.cpp" 
	"src/modules/autoclick.cpp" 
	"src/modules/triggerbot.cpp" "src/utils/utils.cpp" "src/modules/glow_esp.cpp" "src/modules/keepsprint.cpp" "src/java_hook/java_hook.cpp" "src/logger/logger.cpp" "src/gui/render_info.cpp" "src/scheduler/scheduler.cpp")

target_include_directories(MetaJNI PUBLIC "include")
target_include_directories(MetaJNI PUBLIC "include/jni")
target_include_directories(MetaJNI PUBLIC "include/imgui")

target_link_directories(MetaJNI PUBLIC "lib")
target_link_libraries(MetaJNI PUBLIC "jvm")

if(MSVC)
	set_property(TARGET MetaJNI PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET MetaJNI PROPERTY CXX_STANDARD 20)
endif()

if(WIN32)
	target_compile_definitions(MetaJNI PUBLIC NOMINMAX)
	target_include_directories(MetaJNI PUBLIC "include/jni/win32")
	target_link_libraries(MetaJNI PUBLIC "opengl32.lib" "libMinHook.x64.lib")
elseif(UNIX)
	target_include_directories(MetaJNI PUBLIC "include/jni/linux")
	target_link_libraries(MetaJNI PUBLIC "X11") # apt install libx11-dev
endif()

# TODO: Add tests and install targets if needed.
