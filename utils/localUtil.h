#ifndef DANARC_LOCALUTIL_H
#define DANARC_LOCALUTIL_H
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "../offsets.h"
#include "../memory/memory.h"
#include "gameUtil.h"
#include "Overlay.h"

bool isValidPtr(ptr addr) {
    if (addr < 0x10000 || addr > 0x800000000000) return false;
    return true;
}


//prob update every game update
inline uintptr_t GetUWorld(ptr BaseAddr) {
    ptr uworldPtrBase = ReadMemory<uintptr_t>(BaseAddr + 0x0DD1EEC8);

    ptr uworldAddr = ReadMemory<uintptr_t>(uworldPtrBase);

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

Vector3 GetCameraLocation(const Matrix4x4& viewMatrix) {
    // Column-major: indices transposed
    double m00 = viewMatrix.m[0][0], m01 = viewMatrix.m[1][0], m02 = viewMatrix.m[2][0];
    double m10 = viewMatrix.m[0][1], m11 = viewMatrix.m[1][1], m12 = viewMatrix.m[2][1];
    double m20 = viewMatrix.m[0][2], m21 = viewMatrix.m[1][2], m22 = viewMatrix.m[2][2];

    // Translation now in row 3 (was column 3)
    double tx = viewMatrix.m[0][3];
    double ty = viewMatrix.m[1][3];
    double tz = viewMatrix.m[2][3];

    return Vector3{
        -(tx * m00 + ty * m01 + tz * m02),
        -(tx * m10 + ty * m11 + tz * m12),
        -(tx * m20 + ty * m21 + tz * m22)
    };
}

// Simple struct for rendering
enum Object {
    ARC,
    SEARCH,
    PICKUP,
    PLAYER,
    NONE
};
struct RenderEntity {
    Vector3 pos;
    float dist;
    ptr vt{};
    enum Object type = Object::NONE;
};

bool WorldToScreen(Vector3 world, Vector2& screen, Matrix4x4 matrix) {

    float w = matrix.m[3][0] * world.x + matrix.m[3][1] * world.y + matrix.m[3][2] * world.z + matrix.m[3][3];

    if (w < 0.01f) return false; // Behind camera

    float x = matrix.m[0][0] * world.x + matrix.m[0][1] * world.y + matrix.m[0][2] * world.z + matrix.m[0][3];
    float y = matrix.m[1][0] * world.x + matrix.m[1][1] * world.y + matrix.m[1][2] * world.z + matrix.m[1][3];

    float invW = 1.0f / w;
    float ndc_x = x * invW;
    float ndc_y = y * invW;

    screen.x = (SCREEN_W / 2.0f) * (1.0f + ndc_x);
    screen.y = (SCREEN_H / 2.0f) * (1.0f - ndc_y);
    return true;
}

#endif //DANARC_LOCALUTIL_H