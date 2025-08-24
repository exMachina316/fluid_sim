cmake_minimum_required(VERSION 3.10)
project(FluidSim)

set(CMAKE_CXX_STANDARD 17)

# Set binary output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Find OpenGL, GLFW, and GLM
set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

# Add source files
add_executable(fluid_sim src/main.cpp src/glad.c src/fluid_sim.cpp)

# Include directories
target_include_directories(fluid_sim PRIVATE include ${GLFW_INCLUDE_DIRS})

# Link libraries
target_link_libraries(fluid_sim OpenGL::GL glfw)

# Add optimization and debug flags
target_compile_options(fluid_sim PRIVATE -O2 -g)
