cmake_minimum_required(VERSION 3.20) # Changed to 3.20 (4.1 doesn't exist yet)
project(danARC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# --- 1. DEFINE SOURCES FIRST ---
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/imgui")

set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# --- 2. ADD EXECUTABLE WITH IMGUI SOURCES ---
add_executable(danARC
        main.cpp
        memory/memory.cpp
        memory/memory.h
        utils/localUtil.h
        offsets.h
        utils/gameUtil.h
        utils/Overlay.h
        ${IMGUI_SOURCES}
)

# --- 3. INCLUDES AND LINKS ---
target_include_directories(danARC PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

target_link_libraries(danARC PRIVATE glfw OpenGL::GL)