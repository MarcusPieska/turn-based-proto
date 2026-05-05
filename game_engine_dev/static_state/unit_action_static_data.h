//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_ACTION_STATIC_DATA_H
#define UNIT_ACTION_STATIC_DATA_H

#include <string>
#include "unit_action_static_key.h"
#include "item_reqs.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - UnitActionStaticDataStruct -
//================================================================================================================================

typedef struct UnitActionStaticDataStruct {
    std::string name;
} UnitActionStaticDataStruct;

//================================================================================================================================
//=> - UnitActionStaticData class -
//================================================================================================================================

class UnitActionStaticData {
public:
    static void set_items (UnitActionStaticDataStruct* items, u16 item_count);
    static const UnitActionStaticDataStruct& get_item (UnitActionStaticDataKey key);
    static u16 get_item_count ();

private:
    UnitActionStaticData () = delete;
    UnitActionStaticData (const UnitActionStaticData& other) = delete;
    UnitActionStaticData (UnitActionStaticData&& other) = delete;

    static UnitActionStaticDataStruct* m_item_array;
    static u16 m_item_count;
};

#endif // UNIT_ACTION_STATIC_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
