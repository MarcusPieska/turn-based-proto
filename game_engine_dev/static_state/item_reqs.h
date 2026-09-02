//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ITEM_REQS_H
#define ITEM_REQS_H

#include "game_primitives.h"

//================================================================================================================================
//=> - ItemReqsStruct -
//================================================================================================================================

#define MAX_PREREQ_COUNT 5 // Will require 15 bytes in total ((2 + 1) * 5)

typedef enum ItemReqType {
    ITEM_REQ_TYPE_NONE = 0,
    ITEM_REQ_TYPE_TECH = 1,
    ITEM_REQ_TYPE_RESOURCE = 2,
    ITEM_REQ_TYPE_FLAG = 3,
    ITEM_REQ_TYPE_CIV = 4,
    ITEM_REQ_TYPE_BUILDING = 5,
    ITEM_REQ_TYPE_TILE = 6,
    ITEM_REQ_TYPE_CIV_TRAIT = 7
} ItemReqType;

typedef enum TileReqKind {
    TILE_REQ_KIND_NONE = 0,
    TILE_REQ_KIND_TERRAIN = 1,
    TILE_REQ_KIND_CLIMATE = 2,
    TILE_REQ_KIND_OVERLAY = 3,
    TILE_REQ_KIND_ATTRIBUTE = 4
} TileReqKind;

typedef struct ItemReqsStruct {
    u16 indices[MAX_PREREQ_COUNT];
    u8 types[MAX_PREREQ_COUNT];
    u8 added_args[MAX_PREREQ_COUNT];
} ItemReqsStruct;

#define MAX_CIV_TRAIT_COUNT 4

typedef struct CivTraitStruct {
    u16 indices[MAX_CIV_TRAIT_COUNT];
} CivTraitStruct;


#endif // ITEM_REQS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
