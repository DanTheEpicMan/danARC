#ifndef DANARC_CONFIG_H
#define DANARC_CONFIG_H
#include <cstdint>

//-------------------Config-------------------// btw m stands of meters
inline double maxPlayerDist = 400/*<-Dist in m*/ * 100;
inline double maxArcDist = 200  /*<-Dist in m*/  * 100;
inline double maxLootDist = 70 /*<-Dist in m*/   * 100;
inline bool enableRadar = true;

inline int SCREEN_H = 1440;
inline int SCREEN_W = 2560;

inline bool isDebugMode = false;
//----------------------------------------------

#define pr inline uintptr_t
#define ptr uintptr_t

namespace off {
    // From UWorld : "class World : public Object"
    pr PERSISTENT_LEVEL = 0x130; //0xD0;//0xF8; //same on 1/20 and 1/27
    pr CACHED_VIEW_INFO_PTR = 0x1f0; //0x1A0; //0x220; //was 0x198
    pr CACHED_POS_PTR = 0x0;

    // From PersistentLevel (ULevel)
    // Known as AActor or Actor Array
    pr ACTORS_PTR = 0x100;//0x108; //same on 1/20 and 1/27, 1/28

    // From Actor
    pr ROOT_COMPONENT_PTR = 0x220; //0x228; //0x238; //0x228;

    // From SceneComponent (RootComponent)
    pr POS_PTR = 0x290; //0x258; //0x248;//0x1D0;

}

namespace vtabels {

    pr ARC = 0x14beedbb0;

    pr SEARCH = 0x14bd784a0;
    pr PICKUP = 0x14bd3eb30;

    pr PLAYER = 0x14be8ed00;
}

#endif //DANARC_CONFIG_H