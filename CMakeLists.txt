cmake_minimum_required(VERSION 3.5)
project(rsperks)

set(CMAKE_CXX_STANDARD 17)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -flto -Ofast -fomit-frame-pointer -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

find_package(Threads REQUIRED)

set(RS_SOURCES
        rs/Component.h rs/Component.cpp
        rs/Perk.h rs/Perk.cpp
        rs/Probability.h
        rs/Gizmo.cpp
        rs/OptimalGizmoSearch.cpp)

# Command Line Search Tool
add_executable(gizmo-search cmd/cmd_search.cpp ${RS_SOURCES})
target_link_libraries(gizmo-search Threads::Threads)
