cmake_minimum_required(VERSION 3.10)
project(InsideTheFrame)

# Menggunakan standar C++ 17
set(CMAKE_CXX_STANDARD 17)

# Memberi tahu compiler letak file Header (.h / .hpp)
include_directories(Dependencies/glfw/include)
include_directories(Dependencies/glad/include)
include_directories(Dependencies)
include_directories(Dependencies/stb)

# Memberi tahu compiler letak file Library (.a / .lib)
link_directories(Dependencies/glfw/lib-mingw-w64)

# Menentukan file mana saja yang akan di-compile menjadi program .exe
add_executable(InsideTheFrame src/main.cpp Dependencies/glad/src/glad.c)

# Menghubungkan library OpenGL ke program utama
target_link_libraries(InsideTheFrame glfw3 opengl32 gdi32)