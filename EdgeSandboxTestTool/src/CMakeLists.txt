cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_SYSTEM_NAME Windows)

# set the project name
project(ESTT)

add_compile_options(/guard:cf /WX /wd4267)

add_subdirectory(child)
add_subdirectory(sandbox)

# add the executable
add_executable(estt $<TARGET_OBJECTS:estt_core> estt.cc)
add_executable(renderer $<TARGET_OBJECTS:renderer_process> child/renderer/renderer.cc)
