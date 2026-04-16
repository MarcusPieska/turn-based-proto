//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_FLAG_STATIC_DATA_H
#define CITY_FLAG_STATIC_DATA_H

#include <string>
#include "city_flag_static_key.h"
#include "item_reqs.h"
#include "item_effects.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - CityFlagStaticDataStruct -
//================================================================================================================================

typedef struct CityFlagStaticDataStruct {
    std::string name;
    ItemReqsStruct reqs;
    ItemEffectsStruct effects;
} CityFlagStaticDataStruct;

//================================================================================================================================
//=> - CityFlagStaticData class -
//================================================================================================================================

class CityFlagStaticData {
public:
    static void set_items (CityFlagStaticDataStruct* items, u16 item_count);
    static const CityFlagStaticDataStruct& get_item (CityFlagStaticDataKey key);
    static u16 get_item_count ();

private:
    CityFlagStaticData () = delete;
    CityFlagStaticData (const CityFlagStaticData& other) = delete;
    CityFlagStaticData (CityFlagStaticData&& other) = delete;

    static CityFlagStaticDataStruct* m_item_array;
    static u16 m_item_count;
};

#endif // CITY_FLAG_STATIC_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
