//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCE_STATIC_DATA_H
#define RESOURCE_STATIC_DATA_H

#include <string>
#include "resource_static_key.h"
#include "item_reqs.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - ResourceStaticDataStruct -
//================================================================================================================================

typedef struct ResourceStaticDataStruct {
    std::string name;
    ItemReqsStruct reqs;
    u16 food;
    u16 shields;
    u16 commerce;
} ResourceStaticDataStruct;

//================================================================================================================================
//=> - ResourceStaticData class -
//================================================================================================================================

class ResourceStaticData {
public:
    static void set_items (ResourceStaticDataStruct* items, u16 item_count);
    static const ResourceStaticDataStruct& get_item (ResourceStaticDataKey key);
    static u16 get_item_count ();

private:
    ResourceStaticData () = delete;
    ResourceStaticData (const ResourceStaticData& other) = delete;
    ResourceStaticData (ResourceStaticData&& other) = delete;

    static ResourceStaticDataStruct* m_item_array;
    static u16 m_item_count;
};

#endif // RESOURCE_STATIC_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
