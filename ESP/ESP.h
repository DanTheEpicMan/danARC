//
// Created by kub on 1/25/26.
//

#ifndef DANARC_ESP_H
#define DANARC_ESP_H
#include <vector>

#include "../utils/Overlay.h"
#include "../utils/gameUtil.h"
#include "../utils/localUtil.h"
#include "../offsets.h"


void DrawRadar(const std::vector<RenderEntity>& entities, FminimalViewInfo cameraInfo) {
    float radarCenterX = 150;
    float radarCenterY = 150;
    float radarRadius = 100;
    float scale = 200.0f;

    // Background
    DrawCircleFilled(radarCenterX, radarCenterY, radarRadius + 2, IM_COL32(0, 0, 0, 150));
    DrawCircleFilled(radarCenterX, radarCenterY, radarRadius, IM_COL32(0, 0, 0, 150));

    // Crosshairs
    DrawLine(radarCenterX - radarRadius, radarCenterY, radarCenterX + radarRadius, radarCenterY, IM_COL32(255, 255, 255, 50), 1);
    DrawLine(radarCenterX, radarCenterY - radarRadius, radarCenterX, radarCenterY + radarRadius, IM_COL32(255, 255, 255, 50), 1);

    // Convert yaw from degrees to radians
    float yawRad = cameraInfo.Rotation.y * 3.14159265358979323846f / 180.0f;
    float cosA = cos(yawRad);
    float sinA = sin(yawRad);

    for (const auto& ent : entities) {
        if (ent.type == Object::PICKUP || ent.type == Object::SEARCH) continue;
        if (ent.dist > radarRadius * scale) continue;

        float deltaX = ent.pos.x - cameraInfo.Location.x;
        float deltaY = ent.pos.y - cameraInfo.Location.y;

        // Rotate by camera yaw
        float rotatedX = deltaX * cosA + deltaY * sinA;
        float rotatedY = -deltaX * sinA + deltaY * cosA;

        float screenX = radarCenterX + rotatedX / scale;
        float screenY = radarCenterY + rotatedY / scale;

        // Clamp to radar circle
        float dx = screenX - radarCenterX;
        float dy = screenY - radarCenterY;
        float dist = sqrt(dx * dx + dy * dy);
        if (dist > radarRadius - 3) {
            float clampScale = (radarRadius - 3) / dist;
            screenX = radarCenterX + dx * clampScale;
            screenY = radarCenterY + dy * clampScale;
        }

        ImU32 color = IM_COL32(255, 255, 255, 255);
        int radius = 3;
        if (ent.type == Object::PLAYER) {color = IM_COL32(255, 0, 0, 255);}
        else if (ent.type == Object::ARC) {color = IM_COL32(255, 255, 0, 255);}
        else if (ent.type == Object::SEARCH) {color = IM_COL32(50, 200, 50, 255); radius = 1;}
        else if (ent.type == Object::PICKUP) {color = IM_COL32(50, 200, 200, 255); radius = 1;}

        DrawCircleFilled(screenX, screenY, radius, color);
    }

    DrawCircleFilled(radarCenterX, radarCenterY, 4, IM_COL32(0, 255, 0, 255));
}

void DrawESP(const std::vector<RenderEntity>& entities, FminimalViewInfo cameraInfo, double maxArcDist, double maxLootDist, int screenWidth, int screenHeight) {
    bool isDebugMode = isDebugModeAtomic.load();
    for (const auto& ent : entities) {
        Vector2 s;
        ImU32 color{};

        // Call WorldToScreen with view matrix
        s = WorldToScreen(ent.pos, cameraInfo, screenWidth, screenHeight);

        // Check if point is on screen (WorldToScreen returns -9999 if behind camera)
        if (s.x > -9000 && s.y > -9000) {
            float distM = ent.dist / 100.0f;
            if (distM < 1.0f) distM = 1.0f;

            if (isDebugMode) {
                char vtBuf[64];
                sprintf(vtBuf, "0x%lx [%.0fm]", ent.vt, distM);
                DrawTextImGui(s.x, s.y, IM_COL32(255, 255, 255, 255), vtBuf);
                continue;
            }

            //*********** TEST *************
            if (ent.isDead) {
                DrawCircleFilled(s.x, s.y, 10, IM_COL32(255, 50, 50, 255));
            }
            //*********** TEST *************

            if (ent.type == Object::PLAYER) {
                color = IM_COL32(255, 50, 50, 255);

                Vector3 headPos = ent.pos; headPos.z += 90;
                Vector3 feetPos = ent.pos; feetPos.z -= 90;

                Vector2 sHead = WorldToScreen(headPos, cameraInfo, screenWidth, screenHeight);
                Vector2 sFeet = WorldToScreen(feetPos, cameraInfo, screenWidth, screenHeight);

                if (sHead.x > -9000 && sHead.y > -9000 &&
                    sFeet.x > -9000 && sFeet.y > -9000) {
                    float h = sFeet.y - sHead.y;
                    float w = h / 2.0f;

                    DrawBox(sHead.x - w/2, sHead.y, w, h, color);

                    char dBuf[32];
                    sprintf(dBuf, "%.0fm", distM);
                    DrawTextCentered(sHead.x - w/2, sHead.y - 15, IM_COL32(255, 255, 255, 255), dBuf);
                }
            } else {
                int radius = 5;
                if (ent.type == Object::ARC) {
                    if (ent.dist > maxArcDist) continue;
                    color = IM_COL32(200, 200, 50, 255);
                    radius = (1-(ent.dist/maxArcDist)) * 6 + 4;
                } else if (ent.type == Object::SEARCH) {
                    if (ent.dist > maxLootDist) continue;
                    color = IM_COL32(50, 200, 50, 255);
                    radius = (1-(ent.dist/maxLootDist)) * 6 + 4;
                } else if (ent.type == Object::PICKUP) {
                    if (ent.dist > maxLootDist) continue;
                    color = IM_COL32(50, 200, 200, 255);
                    radius = (1-(ent.dist/maxLootDist)) * 6 + 4;
                }

                DrawCircleFilled(s.x, s.y, radius, color);
                char dBuf[32];
                sprintf(dBuf, "%.0fm", distM);
                DrawTextCentered(s.x, s.y+radius+1, IM_COL32(255, 255, 255, 255), dBuf);
            }

        }
    }
}



#endif //DANARC_ESP_H