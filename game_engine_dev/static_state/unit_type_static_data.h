//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_TYPE_STATIC_DATA_H
#define UNIT_TYPE_STATIC_DATA_H

#include <string>
#include "unit_type_static_key.h"
#include "item_reqs.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - UnitTypeStaticDataStruct -
//================================================================================================================================

typedef struct UnitTypeStaticDataStruct {
    std::string name;
} UnitTypeStaticDataStruct;

//================================================================================================================================
//=> - UnitTypeStaticData class -
//================================================================================================================================

class UnitTypeStaticData {
public:
    static void set_items (UnitTypeStaticDataStruct* items, u16 item_count);
    static const UnitTypeStaticDataStruct& get_item (UnitTypeStaticDataKey key);
    static u16 get_item_count ();

private:
    UnitTypeStaticData () = delete;
    UnitTypeStaticData (const UnitTypeStaticData& other) = delete;
    UnitTypeStaticData (UnitTypeStaticData&& other) = delete;

    static UnitTypeStaticDataStruct* m_item_array;
    static u16 m_item_count;
};

#endif // UNIT_TYPE_STATIC_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
