#ifndef DANARC_LOCALUTIL_H
#define DANARC_LOCALUTIL_H
#include <cstdint>
#include <fstream>

#include "../offsets.h"
#include "../memory/memory.h"
#include "gameUtil.h"

bool isValidPtr(ptr addr) {
    if (addr < 0x10000 || addr > 0x100000000000) return false;
    return true;
}

inline uintptr_t GetUWorld(ptr BaseAddr) {
    // Double pointer dereference: BASE + offset -> intermediate ptr -> UWorld
    ptr uworldPtrBase = ReadMemory<uintptr_t>(BaseAddr + off::UWORLD_PTR_BASE);

    ptr uworldAddr = ReadMemory<uintptr_t>(uworldPtrBase + off::UWORLD_PTR_DEREF);

    return uworldAddr;
}

namespace fs = std::filesystem;

pid_t FindGamePID() {
    for (const auto& entry : fs::directory_iterator("/proc")) {
        if (!entry.is_directory()) continue;

        std::string pid_str = entry.path().filename().string();

        // Check if directory name is a number (PID)
        if (pid_str.empty() || !isdigit(pid_str[0])) continue;

        try {
            pid_t pid = std::stoi(pid_str);

            // Count threads in /proc/PID/task
            std::string task_path = "/proc/" + pid_str + "/task";
            int thread_count = 0;

            if (fs::exists(task_path)) {
                for (const auto& task : fs::directory_iterator(task_path)) {
                    thread_count++;
                }
            }

            // Check if process has >100 threads (Unreal Engine indicator)
            if (thread_count > 100) {
                // Check process name
                std::string comm_path = "/proc/" + pid_str + "/comm";
                std::ifstream comm_file(comm_path);
                std::string comm_name;

                if (comm_file >> comm_name) {
                    if (comm_name.find("GameThread") != std::string::npos) {
                        std::cout << "[+] Found game process: PID " << pid
                                  << " (" << comm_name << ") with "
                                  << thread_count << " threads" << std::endl;
                        return pid;
                    }
                }
            }
        } catch (...) {
            continue;
        }
    }

    std::cerr << "[-] Game process not found!" << std::endl;
    return 0;
}

bool WorldToScreen(Vector3 worldPos, Matrix4x4 matrix, int screenW, int screenH, ScreenPos& out) {
    float clipX = worldPos.x * matrix.m[0][0] + worldPos.y * matrix.m[1][0] + worldPos.z * matrix.m[2][0] + matrix.m[3][0];
    float clipY = worldPos.x * matrix.m[0][1] + worldPos.y * matrix.m[1][1] + worldPos.z * matrix.m[2][1] + matrix.m[3][1];
    float clipW = worldPos.x * matrix.m[0][3] + worldPos.y * matrix.m[1][3] + worldPos.z * matrix.m[2][3] + matrix.m[3][3];

    if (clipW < 0.01f) return false;

    float ndcX = clipX / clipW;
    float ndcY = clipY / clipW;

    // Standard NDC to Screen conversion
    out.x = (screenW / 2.0f) + (ndcX * (screenW / 2.0f));
    out.y = (screenH / 2.0f) - (ndcY * (screenH / 2.0f));

    return true;
}

#endif //DANARC_LOCALUTIL_H