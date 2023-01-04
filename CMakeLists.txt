cmake_minimum_required(VERSION 3.24)

project(DesirePathSim)

find_package(raylib 4.2 REQUIRED)

add_executable(${PROJECT_NAME} "src/main.cpp" "src/Bitmap.cpp" "src/Villager.cpp" "src/DesirePaths.cpp" "src/DesirePathSim.cpp" "src/WorldGen.cpp")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
