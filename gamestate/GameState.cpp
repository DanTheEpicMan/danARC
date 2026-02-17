#include "GameState.h"

#include <thread>
#include "../memory/memory.h"

GameState::GameState(uintptr_t baseAddr) {
    this->BaseAddr = baseAddr;
}

InfoReturn GameState::GetState() {
    ptr uworld = getUworld();
    if (!isValidPtr(uworld)) {
        std::cout << "[-] Invalid Uworld" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return{};
    }

    //Used on new update
    //scanCamPos(uworld)

    Vector3 camPos = getCamPos(uworld);
    if (camPos.Dist({0, 0, 0}) < 1) {
        fh.setFirstScan({}); //reset because new map
        std::cout << "[-] Invalid camPos" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return{};
    }

    camPos.Print();

    std::vector<RenderEntity> rawEntities = getEntities(uworld);
    if (!rawEntities.size()) {
        fh.setFirstScan({}); //reset because new map
        std::cout << "[-] Invalid rawEntities" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return{};
    }

    std::cout << "Raw Ent Size: " << rawEntities.size() << std::endl;

    FminimalViewInfo viewMatrix = getLPVM(rawEntities, camPos);
    if (viewMatrix.FOV < 1) {
        std::cout << "[-] Invalid ViewMatrix" << std::endl; //Getting this error likely means a bad vtables::PLAYER
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return{};
    }

    viewMatrix.Print();

    fh.add(rawEntities);
    if (fh.getFirstScanSize() == 0) {
        fh.setFirstScan(rawEntities);
    }

    //filter
    std::vector<RenderEntity> filteredEntities = filterEntities(rawEntities, camPos);
    if (isDebugMode) filteredEntities = rawEntities; //dont filter on debug

    std::cout << "Fil Ent Size: " << filteredEntities.size() << std::endl;

    //return
    return {viewMatrix, filteredEntities};
}

/************* Private ************/

ptr GameState::getUworld() {
    ptr uworldPtrBase = ReadMemory<uintptr_t>(this->BaseAddr + 0xDD77B78);

    ptr uworldAddr = ReadMemory<uintptr_t>(uworldPtrBase);

    return uworldAddr;
}

Vector3 GameState::getCamPos(uintptr_t uworld) {
    ptr viewInfoPtr = ReadMemory<ptr>(uworld + off::CACHED_VIEW_INFO_PTR);
    Vector3 camPos = ReadMemory<Vector3>(viewInfoPtr + off::CACHED_POS_PTR);
    return camPos;
}

std::vector<RenderEntity> GameState::filterEntities(std::vector<RenderEntity> entities, Vector3 camPos) {
    std::vector<RenderEntity> filteredEntities; //should not be dupicated or anything
    for (int i{}; i < entities.size(); i++) {
        //Not a player, probobly a spawn point
        std::cout << "Dist From First Scan: " << entities[i].pos.Dist(fh.getFirstScanIndexPos(i)) << std::endl;
        if (entities[i].pos.Dist(fh.getFirstScanIndexPos(i)) < 50) continue;

        // dead/not moving
        std::cout << "Dist From Old Scan: " << entities[i].pos.Dist(fh.getOldestPosEnt(i)) << std::endl;
        if (entities[i].pos.Dist(fh.getOldestPosEnt(i)) < 100) entities[i].isDead = true;

        entities[i].dist = entities[i].pos.Dist(camPos);
        std::cout << "Dist from self: " << entities[i].dist << std::endl;
        if (entities[i].dist < 100 && entities[i].type == Object::PLAYER) continue; //LP

        filteredEntities.push_back(entities[i]);
    }
    return filteredEntities;
}

std::vector<RenderEntity> GameState::getEntities(uintptr_t uworld) {
    std::vector<RenderEntity> entities;
    ptr persistentLevel = ReadMemory<ptr>(uworld + off::PERSISTENT_LEVEL);
    if (!isValidPtr(persistentLevel)) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return{};
    }

    ptr actors = ReadMemory<ptr>(persistentLevel + off::ACTORS_PTR);
    int actorsCount = ReadMemory<int>(persistentLevel + off::ACTORS_PTR + 0x8);
    if (!(isValidPtr(actors) && actorsCount > 0 && actorsCount < 5000)) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return{};
    }

    for (int a = 0; a < actorsCount; a++) {
        ptr actor = ReadMemory<ptr>(actors + (a * 0x8));
        if (!isValidPtr(actor)) continue;

        ptr rootComp = ReadMemory<ptr>(actor + off::ROOT_COMPONENT_PTR);
        if (!isValidPtr(rootComp)) continue;

        Vector3 pos = ReadMemory<Vector3>(rootComp + off::POS_PTR);
        if (std::abs(pos.x) < 100) continue;

        ptr vt = ReadMemory<ptr>(actor);

        FminimalViewInfo vm = ReadMemory<FminimalViewInfo>(actor + 0xc40 + 0x10);

        RenderEntity ent;
        ent.pos = pos;
        ent.vt = vt;
        ent.vm = vm;

        if (!isDebugMode) {
            if (vt == vtabels::PLAYER) ent.type = Object::PLAYER;
            else if (vt == vtabels::ARC) ent.type = Object::ARC;
            else if (vt == vtabels::PICKUP) ent.type = Object::PICKUP;
            else if (vt == vtabels::SEARCH) ent.type = Object::SEARCH;
            else continue;
        }

        entities.push_back(ent);
    }
    return entities;
}

FminimalViewInfo GameState::getLPVM(std::vector<RenderEntity> entities, Vector3 camPos) {
    for (RenderEntity entitie : entities) {
        if (camPos.Dist(entitie.pos) < 500 && (entitie.vm.FOV < 120 && entitie.vm.FOV > 30)) {
            return entitie.vm;
        }
    }
    return{};
}

void GameState::scanCamPos(uintptr_t uworld) {
    // //Find CACHED_VIEW_INFO_PTR + CACHED_POS_PTR offsets (in practice range)
    std::cout << "Start" << std::endl;
    for (int i{}; i < 0x5000; i += sizeof(ptr)) {
        ptr vipCheck = ReadMemory<ptr>(uworld + i);
        if (!isValidPtr(vipCheck)) continue;
        for (int j{}; j < 0x5000; j += sizeof(ptr)) {
            Vector3 posCheck = ReadMemory<Vector3>(vipCheck + j);
            if ((posCheck.x > 50000 && posCheck.x < 250000) &&
                (posCheck.y > 50000 && posCheck.y < 250000) &&
                (posCheck.z > 50000 && posCheck.z < 250000)) {
                std::cout << "Found Possible Offset: " <<
                    std::hex << i << " " << j <<  std::dec << std::endl;
                posCheck.Print();
            }
        }
    }
}
