//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_STATIC_DATA_H
#define BUILDING_STATIC_DATA_H

#include <string>
#include "building_static_key.h"
#include "item_reqs.h"
#include "item_effects.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - BuildingStaticDataStruct -
//================================================================================================================================

typedef struct BuildingStaticDataStruct {
    std::string name;
    ItemReqsStruct reqs;
    u32 cost;
    ItemEffectsStruct effects;
} BuildingStaticDataStruct;

//================================================================================================================================
//=> - BuildingStaticData class -
//================================================================================================================================

class BuildingStaticData {
public:
    static void set_items (BuildingStaticDataStruct* items, u16 item_count);
    static const BuildingStaticDataStruct& get_item (BuildingStaticDataKey key);
    static u16 get_item_count ();

private:
    BuildingStaticData () = delete;
    BuildingStaticData (const BuildingStaticData& other) = delete;
    BuildingStaticData (BuildingStaticData&& other) = delete;

    static BuildingStaticDataStruct* m_item_array;
    static u16 m_item_count;
};

#endif // BUILDING_STATIC_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
