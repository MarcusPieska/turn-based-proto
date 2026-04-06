//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_STATIC_DATA_H
#define UNIT_STATIC_DATA_H

#include <string>
#include "unit_static_key.h"
#include "item_reqs.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - UnitStaticDataStruct -
//================================================================================================================================

typedef struct UnitStaticDataStruct {
    std::string name;
    ItemReqsStruct reqs;
    u32 cost;
    u16 type;
    u16 attack;
    u16 defense;
    u16 mvt_pts;
} UnitStaticDataStruct;

//================================================================================================================================
//=> - UnitStaticData class -
//================================================================================================================================

class UnitStaticData {
public:
    static void set_items (UnitStaticDataStruct* items, u16 item_count);
    static const UnitStaticDataStruct& get_item (UnitStaticDataKey key);
    static u16 get_item_count ();

private:
    UnitStaticData () = delete;
    UnitStaticData (const UnitStaticData& other) = delete;
    UnitStaticData (UnitStaticData&& other) = delete;

    static UnitStaticDataStruct* m_item_array;
    static u16 m_item_count;
};

#endif // UNIT_STATIC_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
