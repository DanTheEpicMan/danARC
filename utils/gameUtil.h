#ifndef DANARC_GAMEUTIL_H
#define DANARC_GAMEUTIL_H
#include <cmath>

struct Vector3 {
    double x, y, z;

    double Dist(const Vector3& other) const {
        return std::sqrt(std::pow(x - other.x, 2) +
                         std::pow(y - other.y, 2) +
                         std::pow(z - other.z, 2));
    }
};

struct Vector2 {
    double x, y;
};

struct Matrix4x4 {
    double m[4][4];
};

struct ScreenPos {
    float x, y;
};

#endif //DANARC_GAMEUTIL_H