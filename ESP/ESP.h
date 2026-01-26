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


void DrawRadar(Vector3 camPos, const std::vector<RenderEntity>& entities, const Matrix4x4& viewMatrix) {
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

    // Extract camera yaw
    float rightX = viewMatrix.m[0][0];
    float rightY = viewMatrix.m[1][0];
    float camYaw = atan2(rightY, rightX);

    float cosA = cos(camYaw);
    float sinA = sin(camYaw);

    for (const auto& ent : entities) {
        if (ent.type == Object::PICKUP || ent.type == Object::SEARCH) continue;
        if (ent.dist > radarRadius * scale) continue;

        float deltaX = ent.pos.x - camPos.x;
        float deltaY = ent.pos.y - camPos.y;

        // Rotate by camera yaw
        float rotatedX = deltaX * cosA + deltaY * sinA;
        float rotatedY = -deltaX * sinA + deltaY * cosA;

        // FLIPPED: changed signs here
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
        if (ent.type == Object::PLAYER) color = IM_COL32(255, 0, 0, 255);
        else if (ent.type == Object::ARC) color = IM_COL32(255, 255, 0, 255);

        DrawCircleFilled(screenX, screenY, 3, color);
    }

    DrawCircleFilled(radarCenterX, radarCenterY, 4, IM_COL32(0, 255, 0, 255));
}

void DrawESP(Matrix4x4 projMatrix, const std::vector<RenderEntity>& entities, double maxArcDist, double maxLootDist) {
    for (const auto& ent : entities) {
        Vector2 s;
        ImU32 color{};
        if (WorldToScreen(ent.pos, s, projMatrix)) {


            float distM = ent.dist / 100.0f;
            if (distM < 1.0f) distM = 1.0f;

            if (ent.type == Object::PLAYER) {
                color = IM_COL32(255, 50, 50, 255);

                float headHeight = 180.0f;
                // Project head and feet to get precise box height
                Vector3 headPos = ent.pos; headPos.z += 120; // Approx top
                Vector3 feetPos = ent.pos; feetPos.z -= 60; // Approx bottom

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
            } else { //if player
                int radius = 5;
                if (ent.type == Object::ARC) {
                    if (ent.dist > maxArcDist) continue;
                    color = IM_COL32(200, 200, 50, 255);
                    radius = (1-(ent.dist/maxArcDist)) * 6 + 4;
                } else if (ent.type == Object::PICKUP || ent.type == Object::SEARCH) {
                    if (ent.dist > maxLootDist) continue;
                    color = IM_COL32(50, 200, 50, 255);
                    radius = (1-(ent.dist/maxLootDist)) * 6 + 4;
                }

                //draw
                DrawCircleFilled(s.x, s.y, radius, color);
                char dBuf[32];
                sprintf(dBuf, "%.0fm", distM);
                DrawTextCentered(s.x, s.y+radius+1, IM_COL32(255, 255, 255, 255), dBuf);
            }// else player
        }//if on screen
    } //for entities
}



#endif //DANARC_ESP_H