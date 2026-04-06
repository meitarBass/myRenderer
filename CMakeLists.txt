cmake_minimum_required(VERSION 3.21)
project(Renderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer")

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/IO
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Math
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Shaders
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

add_executable(Renderer
        src/IO/tgaimage.cpp
        src/IO/ModelLoader.cpp
        src/Core/Rasterizer.cpp
        main.cpp
        tests/RendererUnitTests.h
        src/Core/IShader.h
        src/Shaders/PhongShader.h
        src/Core/ModelInstance.h
        src/Shaders/DepthShader.h
        tests/RendererUnitTests.cpp
        src/Shaders/DepthShader.cpp
        src/Shaders/PhongShader.cpp
        src/Core/IShader.cpp
        src/Renderer/Scene.h
        src/Core/Camera.h
        src/Renderer/Renderer.cpp
        src/Renderer/Renderer.h
        src/Utils/ThreadPool.cpp
        src/Utils/ThreadPool.h
        external/glad/src/glad.c
        src/Core/Application.h
        src/Shaders/ScreenShader.h
        src/Core/Application.cpp
)

target_include_directories(Renderer PRIVATE external/glad/include)

target_link_libraries(Renderer PRIVATE glfw
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
)

find_package(glfw3 3.4 REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(Renderer PRIVATE glfw "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")

