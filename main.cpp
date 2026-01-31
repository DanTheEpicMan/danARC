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
        if (!isValidPtr(uworld)) {std::cout << "[-] Invalid Uworld " << uworld << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10)); continue;}

        ptr viewInfoPtr = ReadMemory<ptr>(uworld + off::CACHED_VIEW_INFO_PTR);
        if (!isValidPtr(viewInfoPtr)) {std::cout << "[-] Invalid viewInfoPtr " << viewInfoPtr << std::endl;
            firstEntities.clear(); std::this_thread::sleep_for(std::chrono::seconds(10)); continue;};

        viewMatrix = ReadMemory<Matrix4x4>(viewInfoPtr + 0); //ViewMatrix (Cam Pos)
        projMatrix = ReadMemory<Matrix4x4>(viewInfoPtr + 256); //ViewProjectionMatrix (WorldToScreen)
        camPos = ReadMemory<Vector3>(viewInfoPtr + 0x1F0);

        //camPos = GetCameraLocation(viewMatrix);

        std::cout << camPos.x << " " << camPos.y << " " << camPos.z << std::endl;

        // Actors Loop
        ptr persistentLevel = ReadMemory<ptr>(uworld + off::PERSISTENT_LEVEL);
        if (!isValidPtr(persistentLevel)) {std::cout << "[-] Invalid persistentLevel " << persistentLevel << std::endl;
            firstEntities.clear(); std::this_thread::sleep_for(std::chrono::seconds(10)); continue;};

        ptr actors = ReadMemory<ptr>(persistentLevel + off::ACTORS_PTR);
        int actorsCount = ReadMemory<int>(persistentLevel + off::ACTORS_PTR + 0x8);
        if (!(isValidPtr(actors) && actorsCount > 0 && actorsCount < 5000)) {std::cout << "[-] Invalid actors array " << actors << std::endl;
            firstEntities.clear(); std::this_thread::sleep_for(std::chrono::seconds(10)); continue;};

        ptr firstActor = ReadMemory<ptr>(actors);
        ptr firstVt = ReadMemory<ptr>(firstActor);
        std::cout << "First actor: 0x" << std::hex << firstActor << " vt: 0x" << firstVt << std::endl;

        std::cout << "Actor Count: " << actorsCount << std::endl;

        auto DumpMatrix = [](const char* name, const Matrix4x4& m) {
            std::cout << "=== " << name << " ===" << std::endl;
            for (int r = 0; r < 4; r++) {
                std::cout << std::fixed << std::setprecision(4);
                for (int c = 0; c < 4; c++) {
                    std::cout << std::setw(12) << m.m[r][c] << " ";
                }
                std::cout << std::endl;
            }
        };

        DumpMatrix("ViewMatrix", viewMatrix);
        DumpMatrix("ProjMatrix", projMatrix);

        // Scan viewInfoPtr for raw camera position
        std::cout << "\n=== Scanning viewInfoPtr for camera position ===" << std::endl;
        for (int i = 0; i < 0x300; i += 0x8) {
            Vector3 pos = ReadMemory<Vector3>(viewInfoPtr + i);

            // Look for values in similar range as actor positions
            if (std::abs(pos.x) > 100000 && std::abs(pos.x) < 1000000 &&
                std::abs(pos.y) > 100000 && std::abs(pos.y) < 1000000 &&
                std::abs(pos.z) > 1000 && std::abs(pos.z) < 200000) {
                std::cout << "  +0x" << std::hex << i << ": ("
                          << std::dec << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
                }
        }

        // Also dump the first 0x100 bytes as doubles to see structure
        std::cout << "\n=== Raw viewInfoPtr structure ===" << std::endl;
        for (int i = 0; i < 0x100; i += 0x18) {  // 3 doubles = 0x18 bytes
            double x = ReadMemory<double>(viewInfoPtr + i);
            double y = ReadMemory<double>(viewInfoPtr + i + 0x8);
            double z = ReadMemory<double>(viewInfoPtr + i + 0x10);
            std::cout << "+0x" << std::hex << i << ": ("
                      << std::dec << std::fixed << std::setprecision(2)
                      << x << ", " << y << ", " << z << ")" << std::endl;
        }

        int invalidActor = 0, invalidRoot = 0, zeroPos = 0, notMoved = 0, tooClose = 0, unknownVt = 0, passed = 0;

        for (int a = 0; a < actorsCount; a++) {
            ptr actor = ReadMemory<ptr>(actors + (a * 0x8));
            if (!isValidPtr(actor)) { invalidActor++; continue; }

            ptr rootComp = ReadMemory<ptr>(actor + off::ROOT_COMPONENT_PTR);
            if (!isValidPtr(rootComp)) { invalidRoot++; continue; }

            Vector3 pos = ReadMemory<Vector3>(rootComp + off::POS_PTR);
            if (std::abs(pos.x) < 100) { zeroPos++; continue; }

            if (a < firstEntities.size() && firstEntities.at(a).pos == pos) { notMoved++; continue; }

            double dist = camPos.Dist(pos);
            ptr vt = ReadMemory<ptr>(actor);

            RenderEntity ent;
            ent.pos = pos;
            ent.dist = (float)dist;

#if VT_FIND_MODE
            ent.vt = vt;
            //if (ent.dist > 50/*meters*/ * 100/*conversion*/) continue; //skip to declutter screen
#else
            //validity checks
            if (dist < 300.0f) continue; //is LP

            if (vt == vtabels::PLAYER) {ent.type = Object::PLAYER;}
            else if (vt == vtabels::ARC) {ent.type = Object::ARC;}
            else if (vt == vtabels::PICKUP) {ent.type = Object::PICKUP;}
            else if (vt == vtabels::SEARCH) {ent.type = Object::SEARCH;}
            else {continue;}

#endif
            passed++;
            entities.push_back(ent);
        }
        std::cout << "Filter stats: invalidActor=" << invalidActor
          << " invalidRoot=" << invalidRoot
          << " zeroPos=" << zeroPos
          << " notMoved=" << notMoved
          << " tooClose=" << tooClose
          << " unknownVt=" << unknownVt
          << " passed=" << passed << std::endl;

        // Also dump first few actor vtables to find new values
        std::cout << "First 10 actor vtables:" << std::endl;
        for (int a = 0; a < actorsCount; a++) {
            ptr actor = ReadMemory<ptr>(actors + (a * 0x8));
            if (!isValidPtr(actor)) continue;
            ptr vt = ReadMemory<ptr>(actor);
            ptr rootComp = ReadMemory<ptr>(actor + off::ROOT_COMPONENT_PTR);
            Vector3 pos = {0,0,0};
            if (isValidPtr(rootComp)) {
                pos = ReadMemory<Vector3>(rootComp + off::POS_PTR);
            }
            std::cout << "  [" << a << "] vt=0x" << std::hex << vt
                      << " pos=(" << std::dec << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
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