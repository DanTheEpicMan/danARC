#ifndef DANARC_OFFSETS_H
#define DANARC_OFFSETS_H

#define pr inline uintptr_t
#define ptr uintptr_t

#define VT_FIND_MODE true //use if you need to find VTs (will show offsets on objetcs)

namespace off {
    // From UWorld : "class World : public Object"
    pr PERSISTENT_LEVEL = 0xD0;//0xF8; //same on 1/20 and 1/27
    pr CACHED_VIEW_INFO_PTR = 0x1A0; //0x220; //was 0x198
        pr CACHED_POS_PTR = 0x0;

    // From PersistentLevel (ULevel)
    pr ACTORS_PTR = 0x108; //same on 1/20 and 1/27, 1/28

    // From Actor
    pr ROOT_COMPONENT_PTR = 0x228; //0x238; //0x228;

    // From SceneComponent (RootComponent)
    pr POS_PTR = 0x258; //0x248;//0x1D0;

}

namespace vtabels {

    pr ARC = 0x14bd8bc30;

    pr SEARCH = 0x14b784a0;
    pr PICKUP = 0x14bd3eb30;

    pr PLAYER = 0x14be8ed00;
}

#endif