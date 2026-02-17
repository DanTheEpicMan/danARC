#include <atomic>
#include <filesystem>
#include <iostream>
#include <vector>
#include <cmath>
#include <csignal>
#include <thread>
#include <variant>

#include "gamestate/GameState.h"
#include "draw/ESP.h"

inline std::atomic<bool> g_Running(true);
void SignalHandler(int signum) {g_Running = false;}

int main() {
    std::signal(SIGINT, SignalHandler);
    if (!InitOverlay()) {
        std::cerr << "[-] Failed to init Overlay (GLFW/Wayland)" << std::endl;
        return 1;
    }

    ProcessId = FindGamePID();
    GameState gs = GameState(0x140000000);
    while (g_Running && !glfwWindowShouldClose(window)) {
        InfoReturn gsInfo = gs.GetState();

        RenderBegin();

        char buf[64];
        sprintf(buf, "Entities: %lu", gsInfo.entities.size());
        DrawTextImGui(10, 10, IM_COL32(255, 0, 0, 255), buf);

        if (enableRadar) DrawRadar(gsInfo.entities, gsInfo.vm);
        DrawESP(gsInfo.entities, gsInfo.vm, maxArcDist, maxLootDist, SCREEN_W, SCREEN_H);

        RenderEnd();
    }

    std::cout << "[+] Destructing Window" << std::endl;

    if (ImGui::GetCurrentContext()) {
        if (ImGui::GetIO().BackendRendererUserData)
            ImGui_ImplOpenGL3_Shutdown();

        if (ImGui::GetIO().BackendPlatformUserData)
            ImGui_ImplGlfw_Shutdown();

        ImGui::DestroyContext();
    }

    if (window) {
        glfwHideWindow(window);
        glfwDestroyWindow(window);
    }

    // causes segmentation fault
    //glfwTerminate();
    return 0;
}
