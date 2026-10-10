cmake_minimum_required(VERSION 3.10)
project(MyOpenGLProject)

set(CMAKE_CXX_STANDARD 17)

# Find the required libraries
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)

# All sources in src/
file(GLOB SOURCES "src/*.cpp")

# Add source files
add_executable(${PROJECT_NAME} ${SOURCES})

# Tell the compiler where to find header files
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Link the libraries to your executable
target_link_libraries(${PROJECT_NAME}
    glfw
    GLEW::GLEW
    OpenGL::GL
    glm::glm
)
