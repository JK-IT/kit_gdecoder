#ifndef K_TYPE_DEF_H
#define K_TYPE_DEF_H

#include "kommon.h"

typedef struct kColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

typedef struct CodeEntry {
    kColor color;   //3 bytes
    uint32_t parent;     //4 bytes
};

#endif // K_TYPE_DEF_H