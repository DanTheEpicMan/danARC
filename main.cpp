#include <filesystem>
#include <iostream>
#include <vector>
#include <cmath>
#include "memory/memory.h"
#include "utils/localUtil.h"
#include "utils/Overlay.h" // Include our overlay header



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
#else
                                //validity checks
                                if (vt != off::VT_PLAYER) continue; //is not player
                                if (dist < 100.0f) continue; //is LP
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


                    ImU32 color = IM_COL32(255, 0, 0, 255);

                    // Simple Box Calculation
                    // 100 unreal units ~= 1 meter. Assuming player height ~1.8m (180 units)
                    float distM = ent.dist / 100.0f;
                    if (distM < 1.0f) distM = 1.0f;

                    float headHeight = 180.0f;
                    // Project head and feet to get precise box height
                    Vector3 headPos = ent.pos; headPos.z += 90; // Approx top
                    Vector3 feetPos = ent.pos; feetPos.z -= 90; // Approx bottom

#if VT_FIND_MODE
                    Vector2 sBase{};
                    if (WorldToScreen(feetPos, sBase, projMatrix)) {
                        char vtBuf[32];
                        sprintf(vtBuf, "0x%lx", ent.vt);
                        DrawTextImGui(sBase.x, sBase.y, IM_COL32(255, 255, 255, 255), vtBuf);
                    }
#endif

                    Vector2 sHead, sFeet;
                    if (WorldToScreen(headPos, sHead, projMatrix) && WorldToScreen(feetPos, sFeet, projMatrix)) {
                        float h = sFeet.y - sHead.y;
                        float w = h / 2.0f;

                        DrawBox(sHead.x - w/2, sHead.y, w, h, color);

                        // Distance Text
                        char dBuf[32];
                        sprintf(dBuf, "%.0fm", distM);
                        DrawTextImGui(sHead.x - w/2, sHead.y - 15, IM_COL32(255, 255, 255, 255), dBuf);
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