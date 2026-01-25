#include <filesystem>
#include <iostream>
#include <vector>
#include <cmath>
#include <variant>

#include "memory/memory.h"
#include "utils/localUtil.h"
#include "utils/Overlay.h" // Include our overlay header

//-------------------Config-------------------//
double maxPlayerDist = 400;          //in meters
double maxArcDist = 200;              //in meters
double maxLootDist = 70;              //in meters
//----------------------------------------------

int main() {
    if (!InitOverlay()) {
        std::cerr << "[-] Failed to init Overlay (GLFW/Wayland)" << std::endl;
        return 1;
    }

    ProcessId = FindGamePID();
    uintptr_t BaseAddress = 0x140000000;

    std::cout << "[+] Waiting for game..." << std::endl;

    while (!glfwWindowShouldClose(window)) {

        std::vector<RenderEntity> entities;
        Matrix4x4 projMatrix = {0}; // Needs to be filled for W2S
        bool dataValid = false;
        Vector3 camPos = {0,0,0};

        // Retry PID if game closed
        if (ProcessId == 0) ProcessId = FindGamePID();

        if (ProcessId != 0) {
            ptr uworld = GetUWorld(BaseAddress);

            if (isValidPtr(uworld)) {
                ptr viewInfoPtr = ReadMemory<ptr>(uworld + off::CACHED_VIEW_INFO_PTR);

                if (isValidPtr(viewInfoPtr)) {
                    // Read Matrices
                    // Offset 0 = ViewMatrix (Cam Pos)
                    // Offset 256 = ViewProjectionMatrix (WorldToScreen)
                    Matrix4x4 viewMatrix = ReadMemory<Matrix4x4>(viewInfoPtr + 0); //for pos (for filtering out Local Player)
                    projMatrix = ReadMemory<Matrix4x4>(viewInfoPtr + 256);

                    camPos = GetCameraLocation(viewMatrix);
                    dataValid = true;

                    // Actors Loop
                    ptr persistentLevel = ReadMemory<ptr>(uworld + off::PERSISTENT_LEVEL);
                    if (isValidPtr(persistentLevel)) {
                        ptr actors = ReadMemory<ptr>(persistentLevel + off::ACTORS_PTR);
                        int actorsCount = ReadMemory<int>(persistentLevel + off::ACTORS_PTR + 0x8);

                        if (isValidPtr(actors) && actorsCount > 0 && actorsCount < 5000) {

                            // Loop
                            for (int a = 0; a < actorsCount; a++) {
                                ptr actor = ReadMemory<ptr>(actors + (a * 0x8));
                                if (!isValidPtr(actor)) continue;

                                ptr rootComp = ReadMemory<ptr>(actor + off::ROOT_COMPONENT_PTR);
                                if (!isValidPtr(rootComp)) continue;

                                Vector3 pos = ReadMemory<Vector3>(rootComp + off::POS_PTR);
                                if (std::abs(pos.x) < 100) continue; // Skip 0,0,0

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
                        }
                    }
                }
            }
        }
        // --- LOGIC END ---

        // --- RENDER START ---
        RenderBegin();

        if (dataValid) {
            // Stats
            char buf[64];
            sprintf(buf, "Enemies: %lu", entities.size());
            DrawTextImGui(10, 10, IM_COL32(255, 0, 0, 255), buf);

            for (const auto& ent : entities) {
                Vector2 s;
                if (WorldToScreen(ent.pos, s, projMatrix)) {
                    ImU32 color{};

                    float distM = ent.dist / 100.0f;
                    if (distM < 1.0f) distM = 1.0f;

                    if (ent.type == Object::PLAYER) {
                        color = IM_COL32(255, 50, 50, 255);

                        float headHeight = 180.0f;
                        // Project head and feet to get precise box height
                        Vector3 headPos = ent.pos; headPos.z += 90; // Approx top
                        Vector3 feetPos = ent.pos; feetPos.z -= 90; // Approx bottom

#if VT_FIND_MODE
                        char vtBuf[32];
                        sprintf(vtBuf, "0x%lx", ent.vt);
                        DrawTextImGui(s.x, s.y, IM_COL32(255, 255, 255, 255), vtBuf);
#endif

                        Vector2 sHead, sFeet;
                        if (WorldToScreen(headPos, sHead, projMatrix) && WorldToScreen(feetPos, sFeet, projMatrix)) {
                            float h = sFeet.y - sHead.y;
                            float w = h / 2.0f;

                            DrawBox(sHead.x - w/2, sHead.y, w, h, color);

                            // Distance Text
                            char dBuf[32];
                            sprintf(dBuf, "%.0fm", distM);
                            DrawTextCentered(sHead.x - w/2, sHead.y - 15, IM_COL32(255, 255, 255, 255), dBuf);
                        } //if W2S
                    } else {
                        int radius = 5;
                        if (ent.type == Object::ARC) {
                            if (ent.dist > maxArcDist) continue;
                            color = IM_COL32(200, 200, 50, 255);
                            radius = (ent.dist / maxArcDist) * 9 + 1;
                        } else if (ent.type == Object::PICKUP || ent.type == Object::SEARCH) {
                            if (ent.dist > maxLootDist) continue;
                            color = IM_COL32(50, 200, 50, 255);
                            radius = (ent.dist / maxLootDist) * 9 + 1;
                        }

                        //draw
                        DrawCircleFilled(s.x, s.y, radius, color);
                        char dBuf[32];
                        sprintf(dBuf, "%.0fm", distM);
                        DrawTextCentered(s.x, s.y-radius-2, IM_COL32(255, 255, 255, 255), dBuf);
                    }

                }
            }
        } else {
            DrawTextImGui(10, 10, IM_COL32(255, 255, 0, 255), "Waiting for Game / Memory...");
        }

        RenderEnd();
    }

    // Clean exit
    glfwTerminate();
    return 0;
}