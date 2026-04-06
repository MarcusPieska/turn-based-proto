//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TECH_STATIC_DATA_H
#define TECH_STATIC_DATA_H

#include <string>
#include "tech_static_key.h"
#include "item_reqs.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - TechStaticDataStruct -
//================================================================================================================================

typedef struct TechStaticDataStruct {
    std::string name;
    ItemReqsStruct reqs;
    u32 cost;
    u16 tier;
} TechStaticDataStruct;

//================================================================================================================================
//=> - TechStaticData class -
//================================================================================================================================

class TechStaticData {
public:
    static void set_items (TechStaticDataStruct* items, u16 item_count);
    static const TechStaticDataStruct& get_item (TechStaticDataKey key);
    static u16 get_item_count ();

private:
    TechStaticData () = delete;
    TechStaticData (const TechStaticData& other) = delete;
    TechStaticData (TechStaticData&& other) = delete;

    static TechStaticDataStruct* m_item_array;
    static u16 m_item_count;
};

#endif // TECH_STATIC_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
