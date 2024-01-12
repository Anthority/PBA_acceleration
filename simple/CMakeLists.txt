cmake_minimum_required (VERSION 3.9)                  # CMake minimum version

project(simple)                                       # your OpenTimer application


# set(CMAKE_VERBOSE_MAKEFILE ON)                        # Turn on the verbose

set(CMAKE_CXX_STANDARD 17)                            # enable c++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)                        # thread library (pthread)

add_subdirectory(OpenTimer)                           # add OpenTimer project
include_directories(${PROJECT_SOURCE_DIR}/OpenTimer)  # add OpenTimer include

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/)

set(CMAKE_BUILD_TYPE Debug)

add_executable(simple simple.cpp) # link to your app.cpp
target_link_libraries(simple OpenTimer Threads::Threads stdc++fs)