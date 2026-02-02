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
    ptr uworldPtrBase = ReadMemory<uintptr_t>(BaseAddr + 0xDD0FA88);

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



inline Vector2 WorldToScreen(Vector3 WorldLocation, FminimalViewInfo CameraInfo, int Width, int Height) {
    Vector3 Delta = WorldLocation - CameraInfo.Location;

    double Pi = 3.14159265358979323846;
    double Yaw = CameraInfo.Rotation.y * Pi / 180.0;      // Changed Y to y
    double Pitch = CameraInfo.Rotation.x * Pi / 180.0;    // Changed X to x

    Vector3 Forward = Vector3(
        cos(Pitch) * cos(Yaw),
        cos(Pitch) * sin(Yaw),
        sin(Pitch)
    );

    Vector3 Right = Vector3(
        -sin(Yaw),
        cos(Yaw),
        0.0
    );

    Vector3 Up = Vector3(
        -sin(Pitch) * cos(Yaw),
        -sin(Pitch) * sin(Yaw),
        cos(Pitch)
    );

    double CamX = Delta.Dot(Right);
    double CamY = Delta.Dot(Up);
    double CamZ = Delta.Dot(Forward);

    if (CamZ < 1.0) return Vector2(-9999.0, -9999.0);

    double AspectRatio = (double)Width / (double)Height;
    double FovRad = CameraInfo.FOV * Pi / 180.0;  // Changed FieldOfView to FOV
    double TanHalfFov = tan(FovRad / 2.0);

    double ScreenX = (Width / 2.0) + (CamX / (CamZ * TanHalfFov * AspectRatio)) * (Width / 2.0);
    double ScreenY = (Height / 2.0) - (CamY / (CamZ * TanHalfFov)) * (Height / 2.0);

    return Vector2(ScreenX, ScreenY);
}

#endif //DANARC_LOCALUTIL_H