# CMakeList.txt : CMake project for MetaJNI2, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.12)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  cmake_policy(SET CMP0091 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("MetaJNI")

# Add source to this project's executable.
add_library(MetaJNI SHARED "src/main.cpp" )


target_include_directories(MetaJNI PUBLIC "include/jni")

target_link_directories(MetaJNI PUBLIC "lib")
target_link_libraries(MetaJNI PUBLIC "jvm")

if(MSVC)
	set_property(TARGET MetaJNI PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set_property(TARGET MetaJNI PROPERTY CXX_STANDARD 20)

if(WIN32)
	target_include_directories(MetaJNI PUBLIC "include/jni/win32")
elseif(UNIX)
	target_include_directories(MetaJNI PUBLIC "include/jni/linux")
	target_link_libraries(MetaJNI PUBLIC "X11") # apt install libx11-dev
endif()

# TODO: Add tests and install targets if needed.
