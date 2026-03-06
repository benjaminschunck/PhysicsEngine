cmake_minimum_required(VERSION 3.16)

project(PhysicsEngine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# raylib automatisch herunterladen & einbinden
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(raylib)

# Dein Executable
add_executable(${PROJECT_NAME}
    PhysicsEngine/PhysicsEngine.cpp
    PhysicsEngine/PhysicsEngine.h
 )

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
