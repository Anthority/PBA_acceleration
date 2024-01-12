cmake_minimum_required (VERSION 3.9)                  # CMake minimum version

project(optimizer)                                       # your OpenTimer application


# set(CMAKE_VERBOSE_MAKEFILE ON)                        # Turn on the verbose

set(CMAKE_CXX_STANDARD 17)                            # enable c++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)                        # thread library (pthread)

# add_subdirectory(${PROJECT_SOURCE_DIR}/../OpenTimer)                           # add OpenTimer project

# 假设add_subdirectory需要添加的目录，是当前cmakelist.txt文件所在目录的同级目录的子目录
if(NOT OT_DIR)
    set( OT_DIR ${CMAKE_SOURCE_DIR}/../OpenTimer)
endif()
 
if( IS_DIRECTORY ${OT_DIR} )
    # 操作完了之后，OT_DIR输出目录的名字变为OT_DIROut
    add_subdirectory( ${OT_DIR} OT_DIR_Out)
else()
    message(FATAL_ERROR "INVALID FOLDER 'OT_DIR'=${OT_DIR}" ) 
endif()

include_directories(${PROJECT_SOURCE_DIR}/../OpenTimer)  # add OpenTimer include

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/)

set(CMAKE_BUILD_TYPE Debug)

add_executable(optimizer optimizer.cpp) # link to your app.cpp
target_link_libraries(optimizer OpenTimer Threads::Threads stdc++fs)