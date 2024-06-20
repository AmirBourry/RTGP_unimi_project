cmake_minimum_required(VERSION 3.28)
project(work06b)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Od /Zi /EHsc")
include_directories(../../include)
link_directories(../../libs/win)
add_executable(work06b ../../include/glad/glad.c work06b.cpp
        util3d/mesh.h
        util3d/model.h)
target_link_libraries(work06b glfw3 assimp-vc143-mt zlib minizip kubazip poly2tri polyclipping draco pugixml Bullet3Common BulletCollision BulletDynamics LinearMath gdi32 user32 Shell32 Advapi32)
set_property(TARGET work06b PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)