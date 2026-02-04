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



int main() {
    std::signal(SIGINT, SignalHandler);
    if (!InitOverlay()) {
        std::cerr << "[-] Failed to init Overlay (GLFW/Wayland)" << std::endl;
        return 1;
    }

    std::thread input_thread(ConsoleInputThread);
    input_thread.detach();

    ProcessId = FindGamePID();
    uintptr_t BaseAddress = 0x140000000;
    std::vector<RenderEntity> firstEntities;

    std::cout << "[+] Waiting for game..." << std::endl;

    FrameHistory fh(300);

    while (g_Running && !glfwWindowShouldClose(window)) {
        bool isDebugMode = isDebugModeAtomic.load();

        std::vector<RenderEntity> entities;
        Vector3 camPos = {0,0,0};
        FminimalViewInfo viewMatrix {};

        if (ProcessId == 0) continue;

        ptr uworld = GetUWorld(BaseAddress);
        if (!isValidPtr(uworld)) {
            std::cout << "[-] Invalid Uworld" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
            continue;
        }

        // //Find CACHED_VIEW_INFO_PTR + CACHED_POS_PTR offsets (in practice range)
        // std::cout << "Start" << std::endl;
        // for (int i{}; i < 0x300; i += sizeof(ptr)) {
        //     ptr vipCheck = ReadMemory<ptr>(uworld + i);
        //     if (isValidPtr(vipCheck)) continue;
        //     for (int j{}; j < 0x300; j += sizeof(ptr)) {
        //         Vector3 posCheck = ReadMemory<Vector3>(vipCheck + j);
        //         if ((posCheck.x > 50000 && posCheck.x < 250000) &&
        //             (posCheck.y > 50000 && posCheck.y < 250000) &&
        //             (posCheck.z > 50000 && posCheck.z < 250000)) {
        //             std::cout << "Found Possible Offset: " <<
        //                 std::hex << i << " " << j <<  std::dec << std::endl;
        //             posCheck.Print();
        //         }
        //     }
        // }

        ptr viewInfoPtr = ReadMemory<ptr>(uworld + off::CACHED_VIEW_INFO_PTR);
        if (!isValidPtr(viewInfoPtr)) {
            std::cout << "[-] Invalid viewInfoPtr" << std::endl;
            firstEntities.clear();
            std::this_thread::sleep_for(std::chrono::seconds(10));
            continue;
        }

        // Read camera data
        camPos = ReadMemory<Vector3>(viewInfoPtr + off::CACHED_POS_PTR);

        // for (int i{}; i < 0x300; i += sizeof(ptr)) {
        //     ptr presLvlTemp = ReadMemory<ptr>(uworld + i);
        //     if (!isValidPtr(presLvlTemp)) continue;
        //     for (int j{}; j < 0x300; j += sizeof(ptr)) {
        //         ptr arctorsTemp = ReadMemory<ptr>(presLvlTemp + j);
        //         int actorsCountTemp = ReadMemory<ptr>(presLvlTemp + j + 0x8);
        //         if (!(actorsCountTemp > 40 && actorsCountTemp < 100)) continue;
        //
        //         bool goodList = true;
        //
        //         for (int a = 0; a < actorsCountTemp; a++) {
        //             ptr actorTemp = ReadMemory<ptr>(arctorsTemp + (a * 0x8));
        //             if (!isValidPtr(actorTemp)) goodList = false;
        //             //do a check to see if you can find the list of bools
        //
        //         }
        //
        //         if (goodList) {
        //             std::cout << "Found Possible Offset: " <<
        //                 std::hex << i << " " << j << std::dec << std::endl;
        //             std::cout << actorsCountTemp << std::endl;
        //         }
        //     }
        // }


        // Actors Loop
        ptr persistentLevel = ReadMemory<ptr>(uworld + off::PERSISTENT_LEVEL);
        if (!isValidPtr(persistentLevel)) {
            firstEntities.clear();
            std::this_thread::sleep_for(std::chrono::seconds(10));
            continue;
        }



        ptr actors = ReadMemory<ptr>(persistentLevel + off::ACTORS_PTR);
        int actorsCount = ReadMemory<int>(persistentLevel + off::ACTORS_PTR + 0x8);
        if (!(isValidPtr(actors) && actorsCount > 0 && actorsCount < 5000)) {
            firstEntities.clear();
            std::this_thread::sleep_for(std::chrono::seconds(10));
            continue;
        }

        for (int a = 0; a < actorsCount; a++) {
            ptr actor = ReadMemory<ptr>(actors + (a * 0x8));
            if (!isValidPtr(actor)) continue;

            ptr rootComp = ReadMemory<ptr>(actor + off::ROOT_COMPONENT_PTR);
            if (!isValidPtr(rootComp)) continue;

            Vector3 pos = ReadMemory<Vector3>(rootComp + off::POS_PTR);
            if (std::abs(pos.x) < 100) continue;

            if (a < firstEntities.size() && firstEntities.at(a).pos == pos) continue; //filter out spanw points

            double dist = camPos.Dist(pos);
            ptr vt = ReadMemory<ptr>(actor);

            RenderEntity ent;
            ent.pos = pos;
            ent.dist = (float)dist;

            if (camPos.Dist(ent.pos) < 30) { //if LP
                auto vm_temp = ReadMemory<FminimalViewInfo>(actor + 0xc30 + 0x10);
                if (vm_temp.FOV < 120.f && vm_temp.FOV > 20.f) { //just in case
                    viewMatrix = ReadMemory<FminimalViewInfo>(actor + 0xc30 + 0x10);
                    //viewMatrix.Print();
                }
            }


            if (isDebugMode) {
                ent.vt = vt;
                if (dist > 15000) continue; // 150m filter
            } else {
                if (dist < 300.0f) continue;

                if (vt == vtabels::PLAYER) ent.type = Object::PLAYER;
                else if (vt == vtabels::ARC) ent.type = Object::ARC;
                else if (vt == vtabels::PICKUP) ent.type = Object::PICKUP;
                else if (vt == vtabels::SEARCH) ent.type = Object::SEARCH;
                else continue;
            }

            entities.push_back(ent);
        }

        if (firstEntities.empty()) firstEntities = entities;

        for (int a{}; a < entities.size(); a++) {
            if (fh.getOldestPosEnt(a).Dist(entities[a].pos) < 10) entities[a].isDead = true;
        }
        fh.add(entities);


        // Render
        RenderBegin();

        char buf[64];
        sprintf(buf, "Entities: %lu", entities.size());
        DrawTextImGui(10, 10, IM_COL32(255, 0, 0, 255), buf);

        if (enableRadar) DrawRadar(entities, viewMatrix);
        DrawESP(entities, viewMatrix, maxArcDist, maxLootDist, SCREEN_W, SCREEN_H);

        RenderEnd();
    }

    std::cout << "[+] Destructing Window" << std::endl;

    close(STDIN_FILENO);  // This will make cin >> command fail/return
    input_thread.join();

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