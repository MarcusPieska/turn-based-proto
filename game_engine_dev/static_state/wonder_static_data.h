//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WONDER_STATIC_DATA_H
#define WONDER_STATIC_DATA_H

#include <string>
#include "wonder_static_key.h"
#include "item_reqs.h"
#include "item_effects.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - WonderStaticDataStruct -
//================================================================================================================================

typedef struct WonderStaticDataStruct {
    std::string name;
    ItemReqsStruct reqs;
    u32 cost;
    ItemEffectsStruct effects;
} WonderStaticDataStruct;

//================================================================================================================================
//=> - WonderStaticData class -
//================================================================================================================================

class WonderStaticData {
public:
    static void set_items (WonderStaticDataStruct* items, u16 item_count);
    static const WonderStaticDataStruct& get_item (WonderStaticDataKey key);
    static u16 get_item_count ();

private:
    WonderStaticData () = delete;
    WonderStaticData (const WonderStaticData& other) = delete;
    WonderStaticData (WonderStaticData&& other) = delete;

    static WonderStaticDataStruct* m_item_array;
    static u16 m_item_count;
};

#endif // WONDER_STATIC_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
