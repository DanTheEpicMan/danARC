#ifndef DANARC_GAMESTATE_H
#define DANARC_GAMESTATE_H
#include <vector>
#include <deque>

#include "../config.h"
#include "../utils/localUtil.h"


//everything passed back to main
struct InfoReturn {
    FminimalViewInfo vm;
    std::vector<RenderEntity> entities;
};

class FrameHistory {
private:
    std::deque<std::vector<RenderEntity>> history;
    size_t maxFrames;
    std::vector<RenderEntity> firstScan; //should be wiped every map

public:
    FrameHistory(int size) : maxFrames(size) {}

    void add(std::vector<RenderEntity> frameData) {
        history.push_back(frameData);
        // If we exceed x frames, remove the oldest one
        if (history.size() > maxFrames) {
            history.pop_front();
        }
    }

    // Access the oldest frame directly
    const std::vector<RenderEntity>& getOldest() {
        return history.front();
    }

    Vector3 getOldestPosEnt(int index) {
        if (!history.empty() && index < history.front().size()) {
            return history.front()[index].pos;
        }
        return Vector3(0, 0, 0);
    }

    //Return by error of Vector3{}
    Vector3 getFirstScanIndexPos(int index) {
        if (firstScan.size() > index) {
            return firstScan[index].pos;
        }
        return Vector3{};
    }

    //should happen when new map
    void setFirstScan(std::vector<RenderEntity> fs) {
        firstScan = fs;
    }

    int getFirstScanSize() {return firstScan.size();}
};

class FrameHistory; //forward delcaration

class GameState {
private:
    ptr BaseAddr = 0x140000000;
    friend class FrameHistory;
    FrameHistory fh = FrameHistory(1000);

public:
    GameState(ptr baseAddr);

    InfoReturn GetState();

private:
    //needs updating every game update
    ptr getUworld();

    Vector3 getCamPos(ptr uworld);

    std::vector<RenderEntity> getEntities(ptr uworld);

    FminimalViewInfo getLPVM(std::vector<RenderEntity> entities, Vector3 camPos);

    //Math for stand still, dead, and LP
    std::vector<RenderEntity> filterEntities(std::vector<RenderEntity> entities, Vector3 camPos);

private:
    void scanCamPos(ptr uworld);

};


#endif //DANARC_GAMESTATE_H