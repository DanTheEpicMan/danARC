#include <atomic>
#include <filesystem>
#include <iostream>
#include <vector>
#include <cmath>
#include <csignal>
#include <thread>
#include <variant>

#include "memory/memory.h"
#include "utils/localUtil.h"
#include "utils/Overlay.h"
#include "ESP/ESP.h"

//-------------------Config-------------------// btw m stands of meters
double maxPlayerDist = 400/*<-Dist in m*/ * 100;
double maxArcDist = 200  /*<-Dist in m*/  * 100;
double maxLootDist = 70 /*<-Dist in m*/   * 100;
bool enableRadar = true;
//----------------------------------------------


//Handle Destruction
std::atomic<bool> g_Running(true);
void SignalHandler(int signum) {
    g_Running = false;
}

int main() {
    std::signal(SIGINT, SignalHandler);
    if (!InitOverlay()) {
        std::cerr << "[-] Failed to init Overlay (GLFW/Wayland)" << std::endl;
        return 1;
    }

    ProcessId = FindGamePID();
    uintptr_t BaseAddress = 0x140000000;
    std::vector<RenderEntity> firstEntities;

    std::cout << "[+] Waiting for game..." << std::endl;

    while (g_Running && !glfwWindowShouldClose(window)) {

        std::vector<RenderEntity> entities;
        Matrix4x4 projMatrix = {0}; // Needs to be filled for W2S
        Matrix4x4 viewMatrix{};
        Vector3 camPos = {0,0,0};

        if (ProcessId == 0) {continue;};

        ptr uworld = GetUWorld(BaseAddress);
        if (!isValidPtr(uworld)) {/*std::this_thread::sleep_for(std::chrono::seconds(1));*/ continue;}

        ptr viewInfoPtr = ReadMemory<ptr>(uworld + off::CACHED_VIEW_INFO_PTR);
        if (!isValidPtr(viewInfoPtr)) {firstEntities.clear(); continue;};

        viewMatrix = ReadMemory<Matrix4x4>(viewInfoPtr + 0); //ViewMatrix (Cam Pos)
        projMatrix = ReadMemory<Matrix4x4>(viewInfoPtr + 256); //ViewProjectionMatrix (WorldToScreen)

        camPos = GetCameraLocation(viewMatrix);

        // Actors Loop
        ptr persistentLevel = ReadMemory<ptr>(uworld + off::PERSISTENT_LEVEL);
        if (!isValidPtr(persistentLevel)) {firstEntities.clear(); continue;};

        ptr actors = ReadMemory<ptr>(persistentLevel + off::ACTORS_PTR);
        int actorsCount = ReadMemory<int>(persistentLevel + off::ACTORS_PTR + 0x8);
        if (!(isValidPtr(actors) && actorsCount > 0 && actorsCount < 5000)) {firstEntities.clear(); continue;};

        // Loop
        for (int a = 0; a < actorsCount; a++) {
            ptr actor = ReadMemory<ptr>(actors + (a * 0x8));
            if (!isValidPtr(actor)) continue;

            ptr rootComp = ReadMemory<ptr>(actor + off::ROOT_COMPONENT_PTR);
            if (!isValidPtr(rootComp)) continue;

            Vector3 pos = ReadMemory<Vector3>(rootComp + off::POS_PTR);
            if (std::abs(pos.x) < 100) continue; // Skip 0,0,0

            //never moved, so a spawn point, if out or range of initial list, cant be a spawn point
            if (a < firstEntities.size() && firstEntities.at(a).pos == pos) continue;

            double dist = camPos.Dist(pos);
            ptr vt = ReadMemory<ptr>(actor);

            RenderEntity ent;
            ent.pos = pos;
            ent.dist = (float)dist;

#if VT_FIND_MODE
            ent.vt = vt;
            if (ent.dist > 50/*meters*/ * 100/*conversion*/) continue; //skip to declutter screen
#else
            //validity checks
            if (dist < 300.0f) continue; //is LP

            if (vt == vtabels::PLAYER) {ent.type = Object::PLAYER;}
            else if (vt == vtabels::ARC) {ent.type = Object::ARC;}
            else if (vt == vtabels::PICKUP) {ent.type = Object::PICKUP;}
            else if (vt == vtabels::SEARCH) {ent.type = Object::SEARCH;}
            else {continue;}

#endif
            entities.push_back(ent);
        }
        if (firstEntities.empty()) firstEntities = entities;
        // --- LOGIC END ---

        // --- RENDER START ---
        RenderBegin();


        // Stats
        char buf[64];
        sprintf(buf, "Enemies: %lu", entities.size());
        DrawTextImGui(10, 10, IM_COL32(255, 0, 0, 255), buf);

        if (enableRadar) {
            DrawRadar(camPos, entities, projMatrix);
        }

        DrawESP(projMatrix, entities, maxArcDist, maxLootDist);


        RenderEnd();
    }//while g_running && !shouldclosewindow

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