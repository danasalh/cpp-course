cmake_minimum_required(VERSION 3.15)
project(StockTracker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/httplib
    ${CMAKE_SOURCE_DIR}/external/json/include
)

# ImGui source files
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

# Project source files
set(SOURCES
    src/main.cpp
    src/StockTracker.cpp
)

# Create executable
add_executable(${PROJECT_NAME} 
    ${SOURCES}
    ${IMGUI_SOURCES}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    OpenGL::GL
    glfw
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# For SSL support (httplib)
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} ssl crypto)
elseif(APPLE)
    find_library(SECURITY_FRAMEWORK Security)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    target_link_libraries(${PROJECT_NAME} 
        ${SECURITY_FRAMEWORK} 
        ${COREFOUNDATION_FRAMEWORK}
    )
endif()

# Copy resources to build directory
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})