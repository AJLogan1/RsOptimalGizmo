cmake_minimum_required(VERSION 3.5)
project(rsperks)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -ffast-math")
if (USE_OPENMP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -qopenmp")
endif ()

find_package(Boost REQUIRED OPTIONAL_COMPONENTS program_options)
IF (Boost_PROGRAM_OPTIONS_FOUND)
    MESSAGE("Boost Program Options was found.")
ENDIF ()
include_directories(${Boost_INCLUDE_DIRS})

find_package(Drogon CONFIG REQUIRED)
find_package(Trantor REQUIRED)


set(RS_SOURCES
        rs/OptimalGizmo.cpp
        rs/InventionData.cpp
        rs/InventionCalc.cpp rs/Gizmo.cpp rs/Gizmo.h)


# Server
add_executable(optimal_gizmo_server optimal_gizmo_server.cpp ${RS_SOURCES})
target_link_libraries(optimal_gizmo_server Drogon::Drogon)
