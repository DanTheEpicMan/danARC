#ifndef DANARC_OFFSETS_H
#define DANARC_OFFSETS_H

#define pr inline uintptr_t
#define ptr uintptr_t

#define VT_FIND_MODE false //use if you need to find VTs (will show offsets on objetcs)

namespace off {
    // From UWorld
    pr PERSISTENT_LEVEL = 0xF8;
    pr CACHED_VIEW_INFO_PTR = 0x198;

    // From PersistentLevel (ULevel)
    pr ACTORS_PTR = 0x108;

    // From Actor
    pr ROOT_COMPONENT_PTR = 0x228;

    // From SceneComponent (RootComponent)
    pr POS_PTR = 0x1D0;

}

namespace vtabels {
    // //All the same, dont know if its  going to stay like that
    // pr roller = 0x14bcd5cb0;
    // pr wasp = 0x14bcd5cb0;
    // pr jumper = 0x14bcd5cb0;

    pr ARC = 0x14bcd5cb0;

    pr SEARCH = 0x14bcc2a30;
    pr PICKUP = 0x14bc8920;

    pr PLAYER = 0x14bde8a30;
}

#endif