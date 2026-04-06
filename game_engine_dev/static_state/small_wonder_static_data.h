//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SMALL_WONDER_STATIC_DATA_H
#define SMALL_WONDER_STATIC_DATA_H

#include <string>
#include "small_wonder_static_key.h"
#include "item_reqs.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - SmallWonderStaticDataStruct -
//================================================================================================================================

typedef struct SmallWonderStaticDataStruct {
    std::string name;
    ItemReqsStruct reqs;
    u32 cost;
    ItemEffectsStruct effects;
} SmallWonderStaticDataStruct;

//================================================================================================================================
//=> - SmallWonderStaticData class -
//================================================================================================================================

class SmallWonderStaticData {
public:
    static void set_items (SmallWonderStaticDataStruct* items, u16 item_count);
    static const SmallWonderStaticDataStruct& get_item (SmallWonderStaticDataKey key);
    static u16 get_item_count ();

private:
    SmallWonderStaticData () = delete;
    SmallWonderStaticData (const SmallWonderStaticData& other) = delete;
    SmallWonderStaticData (SmallWonderStaticData&& other) = delete;

    static SmallWonderStaticDataStruct* m_item_array;
    static u16 m_item_count;
};

#endif // SMALL_WONDER_STATIC_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
