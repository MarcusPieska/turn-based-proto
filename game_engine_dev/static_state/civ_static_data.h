//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_STATIC_DATA_H
#define CIV_STATIC_DATA_H

#include <string>
#include "civ_static_key.h"
#include "item_reqs.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - CivStaticDataStruct -
//================================================================================================================================

typedef struct CivStaticDataStruct {
    std::string name;
    CivTraitStruct traits;
} CivStaticDataStruct;

//================================================================================================================================
//=> - CivStaticData class -
//================================================================================================================================

class CivStaticData {
public:
    static void set_items (CivStaticDataStruct* items, u16 item_count);
    static const CivStaticDataStruct& get_item (CivStaticDataKey key);
    static u16 get_item_count ();

private:
    CivStaticData () = delete;
    CivStaticData (const CivStaticData& other) = delete;
    CivStaticData (CivStaticData&& other) = delete;

    static CivStaticDataStruct* m_item_array;
    static u16 m_item_count;
};

#endif // CIV_STATIC_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
