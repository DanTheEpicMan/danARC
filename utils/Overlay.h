#pragma once

// CRITICAL: Define this BEFORE including GLFW
#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <stdlib.h> // for setenv

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>

// Screen globals
int SCREEN_W = 2560;
int SCREEN_H = 1440;
GLFWwindow* window = nullptr;

bool InitOverlay() {
    // 1. FORCE WAYLAND
    // Force GLFW to use the Wayland backend.
    setenv("GLFW_PLATFORM", "wayland", 1);

    if (!glfwInit()) {
        std::cerr << "[-] GLFW Init Failed! Install 'libglfw3-dev' and 'libwayland-dev'." << std::endl;
        return false;
    }

    // 2. Verify we are on Wayland
    // If this fails, we are likely on X11, which makes us visible to the game.
#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 4
    if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND) {
        std::cerr << "[-] FATAL: GLFW initialized into X11 mode! Aborting." << std::endl;
        glfwTerminate();
        return false;
    }
#endif

    // 3. Window Hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);     // Always on top
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);   // No borders
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);     // Create hidden

    // CRITICAL: This is what makes the window "Click-Through" on Wayland
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);

    // Get Resolution
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor) {
        std::cerr << "[-] Failed to get primary monitor." << std::endl;
        return false;
    }
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    SCREEN_W = mode->width;
    SCREEN_H = mode->height;

    // Create Window
    window = glfwCreateWindow(SCREEN_W, SCREEN_H, "Better Discord Overlay", nullptr, nullptr);
    if (!window) return false;

    // 4. Setup Context
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync
    glfwShowWindow(window);

    // 5. Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0);
    style.WindowBorderSize = 0.0f;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    std::cout << "[+] Overlay Initialized (Wayland Native)" << std::endl;
    return true;
}

void RenderBegin() {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(SCREEN_W, SCREEN_H));
    ImGui::Begin("##Overlay", nullptr,
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus
    );
}

void RenderEnd() {
    ImGui::End();
    ImGui::Render();
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

void DrawBox(float x, float y, float w, float h, ImU32 color, float thickness = 1.5f) {
    ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, 0.0f, 0, thickness);
}

void DrawLine(float x1, float y1, float x2, float y2, ImU32 color, float thickness = 1.5f) {
    ImGui::GetBackgroundDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, thickness);
}

void DrawTextImGui(float x, float y, ImU32 color, const char* text) {
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(x, y), color, text);
}