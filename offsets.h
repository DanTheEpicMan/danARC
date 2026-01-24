#ifndef DANARC_OFFSETS_H
#define DANARC_OFFSETS_H

#define p inline uintptr_t
#define ptr uintptr_t

namespace off {
    // UWorld
    p UWORLD_PTR_BASE = 0x0DC77CB8;  // UWorld pointer chain
    p UWORLD_PTR_DEREF = 0xC0;

    // From Uworld
    p LEVELS_PTR = 0x228;

    // Common offsets
    p ROOT_COMPONENT = 0x230;
    p POSITION = 0x248;  // 3x double (x, y, z)
    p YAW = 0x268;
}

#endif //DANARC_OFFSETS_H