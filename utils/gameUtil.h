/** Contains game struct info
 * Used by all files (memory, gameState, ESP)
 * Should remain constant between updates
 */

#ifndef DANARC_GAMEUTIL_H
#define DANARC_GAMEUTIL_H
#include <cmath>
#include <iostream>
#include <iomanip>

struct Vector3 {
    double x, y, z;

    // Constructor
    Vector3(double _x = 0.0, double _y = 0.0, double _z = 0.0)
        : x(_x), y(_y), z(_z) {}

    // Dot product
    double Dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    // Subtraction operator
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    double Dist(const Vector3& other) const {
        return std::sqrt(std::pow(x - other.x, 2) +
                         std::pow(y - other.y, 2) +
                         std::pow(z - other.z, 2));
    }

    bool operator==(const Vector3& other) const {
        return (x == other.x && y == other.y && z == other.z);
    }

    void Print() {
        std::cout << x << " " << y << " " << z << std::endl;
    }
};

struct FminimalViewInfo {
    Vector3 Location{};   // +0x00 (24 bytes)
    Vector3 Rotation{};   // +0x18 (24 bytes) - pitch, yaw, roll in DEGREES
    float FOV;            // +0x30

    void Print() const {
        std::cout << "FminimalViewInfo:" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  Location: (" << Location.x << ", " << Location.y << ", " << Location.z << ")" << std::endl;
        std::cout << "  Rotation: (P: " << Rotation.x << ", Y: " << Rotation.y << ", R: " << Rotation.z << ")" << std::endl;
        std::cout << "  FOV:      " << FOV << std::endl;
    }
};

struct Vector2 {
    double x, y;

    // Constructor
    Vector2(double _x = 0.0, double _y = 0.0)
        : x(_x), y(_y) {}
};

struct Matrix4x4 {
    double m[4][4];

    void Print() const {
        std::cout << "Matrix4x4:" << std::endl;
        for (int i = 0; i < 4; ++i) {
            std::cout << "  [ ";
            for (int j = 0; j < 4; ++j) {
                std::cout << std::fixed << std::setprecision(2) << m[i][j] << (j < 3 ? ", " : "");
            }
            std::cout << " ]" << std::endl;
        }
    }
};

struct ScreenPos {
    float x, y;
};

#endif //DANARC_GAMEUTIL_H