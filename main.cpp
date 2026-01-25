#include <filesystem>
#include <iostream>
#include "memory/memory.h"
#include "utils/localUtil.h"
#include <vector>
#include <thread>
#include <cfloat>

#define DBGPrint true

#ifdef DBGPrint
#define DBG(x) std::cout << x << std::endl;
#else
#define DBG(x)
#endif

int main() {
    ProcessId = FindGamePID();
    BaseAddress = 0x140000000;

    if (ProcessId == 0) {
        DBG("[-] Game not found!")
        return 1;
    }

    DBG("[+] PID: " << ProcessId)

    ptr localPlayerPtr = 0;

    while (true) {
        ptr uworld = GetUWorld(BaseAddress);
        if (!isValidPtr(uworld)) {
            DBG("[-] UWorld is Invalid! " << std::hex << uworld << std::dec)
            localPlayerPtr = 0;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // Get view matrix
        ptr viewInfoPtr = ReadMemory<ptr>(uworld + off::CACHED_VIEW_INFO_PTR);
        if (!isValidPtr(viewInfoPtr)) {
            DBG("[-] Failed to get View Info")
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        Matrix4x4 viewMatrix = ReadMemory<Matrix4x4>(viewInfoPtr + 0);
        Vector3 camPos = GetCameraLocation(viewMatrix);

        // Get PersistentLevel
        ptr persistentLevel = ReadMemory<ptr>(uworld + off::PERSISTENT_LEVEL);
        if (!isValidPtr(persistentLevel)) {
            DBG("[-] PersistentLevel invalid")
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        ptr actors = ReadMemory<ptr>(persistentLevel + off::ACTORS_PTR);
        int actorsCount = ReadMemory<int>(persistentLevel + off::ACTORS_PTR + 0x8);

        if (!isValidPtr(actors) || actorsCount < 1 || actorsCount > 5000) {
            DBG("[-] Actors invalid")
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        double closestDist = DBL_MAX;
        std::vector<Player> players{};

        for (int a = 0; a < actorsCount; a++) {
            ptr actor = ReadMemory<ptr>(actors + (a * 0x8));
            if (!isValidPtr(actor)) continue;

            ptr rootComp = ReadMemory<ptr>(actor + off::ROOT_COMPONENT_PTR);
            if (!isValidPtr(rootComp)) continue;

            Vector3 pos = ReadMemory<Vector3>(rootComp + off::POS_PTR);
            if (std::abs(pos.x) < 100 || std::abs(pos.x) > 1e8) continue;

            double dist = camPos.Dist(pos);

            // Find local player (closest to camera)
            if (dist < closestDist) {
                closestDist = dist;
                localPlayerPtr = actor;
            }

            // Check if this is a player by VTable
            ptr vt = ReadMemory<ptr>(actor);
            if (vt != off::VT_PLAYER) continue;

            // Skip local player
            if (actor == localPlayerPtr) continue;

            Player pl{};
            pl.pos = pos;
            players.push_back(pl);
        }

        DBG("[+] Camera: " << camPos.x << ", " << camPos.y << ", " << camPos.z)
        DBG("[+] LocalPlayer: 0x" << std::hex << localPlayerPtr << std::dec << " dist: " << closestDist)
        DBG("[+] Players found: " << players.size())

        for (const auto& p : players) {
            double d = camPos.Dist(p.pos);
            DBG("    Player at: " << p.pos.x << ", " << p.pos.y << ", " << p.pos.z << " (dist: " << d << ")")
        }

        DBG("---")
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}