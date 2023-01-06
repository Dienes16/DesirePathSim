cmake_minimum_required(VERSION 3.24)

project(DesirePathSim LANGUAGES CXX VERSION 0.1.0)

find_package(raylib 4.2 REQUIRED)

configure_file("src/VersionConf.hpp.in" "VersionConf.hpp" @ONLY)

add_executable(${PROJECT_NAME} "src/main.cpp" "src/Bitmap.cpp" "src/Villager.cpp" "src/DesirePaths.cpp" "src/DesirePathSim.cpp" "src/WorldGen.cpp")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
