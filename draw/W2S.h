#ifndef DANARC_W2S_H
#define DANARC_W2S_H
#include "../utils/gameUtil.h"

inline Vector2 WorldToScreen(Vector3 WorldLocation, FminimalViewInfo CameraInfo, int Width, int Height) {
    Vector3 Delta = WorldLocation - CameraInfo.Location;

    double Pi = 3.1416;
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

#endif //DANARC_W2S_H