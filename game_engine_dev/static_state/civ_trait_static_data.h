//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_TRAIT_STATIC_DATA_H
#define CIV_TRAIT_STATIC_DATA_H

#include <string>
#include "civ_trait_static_key.h"
#include "item_reqs.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - CivTraitStaticDataStruct -
//================================================================================================================================

typedef struct CivTraitStaticDataStruct {
    std::string name;
} CivTraitStaticDataStruct;

//================================================================================================================================
//=> - CivTraitStaticData class -
//================================================================================================================================

class CivTraitStaticData {
public:
    static void set_items (CivTraitStaticDataStruct* items, u16 item_count);
    static const CivTraitStaticDataStruct& get_item (CivTraitStaticDataKey key);
    static u16 get_item_count ();

private:
    CivTraitStaticData () = delete;
    CivTraitStaticData (const CivTraitStaticData& other) = delete;
    CivTraitStaticData (CivTraitStaticData&& other) = delete;

    static CivTraitStaticDataStruct* m_item_array;
    static u16 m_item_count;
};

#endif // CIV_TRAIT_STATIC_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
