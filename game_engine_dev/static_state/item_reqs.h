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
    ITEM_REQ_TYPE_BUILDING = 5
} ItemReqType;

typedef struct ItemReqsStruct {
    u16 indices[MAX_PREREQ_COUNT];
    u8 types[MAX_PREREQ_COUNT];
    u8 added_args[MAX_PREREQ_COUNT];
} ItemReqsStruct;

#define MAX_CIV_TRAIT_COUNT 4

typedef struct CivTraitStruct {
    u8 indices[MAX_CIV_TRAIT_COUNT];
} CivTraitStruct;


#endif // ITEM_REQS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
