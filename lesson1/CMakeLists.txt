cmake_minimum_required(VERSION 3.11)
project(lesson1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake-sdl2")


include_directories(include)

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(lesson1
	src/main.cpp
)

include_directories(
        ${SDL2_INCLUDE_DIR}
        ${Vulkan_INCLUDE_DIRS}
)


include(FindPkgConfig)

target_include_directories(lesson1 PUBLIC ${SDL2_INCLUDE_DIR} ${Vulkan_INCLUDE_DIRS} )
target_link_libraries(lesson1 LINK_PUBLIC ${SDL2_LIBRARY}  ${Vulkan_LIBRARIES}	m)

