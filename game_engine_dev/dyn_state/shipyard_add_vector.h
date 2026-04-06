//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SHIPYARD_ADD_VECTOR_H
#define SHIPYARD_ADD_VECTOR_H

#include "shipyard_add_vector_key.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - ShipyardAddItem struct -
//================================================================================================================================

typedef struct ShipyardAddItem {
    u8 x_pos = 0;
    u8 y_pos = 0;
    u16 area_idx = 0;
    u32 dev_flags = 0;
} ShipyardAddItem;

//================================================================================================================================
//=> - ShipyardAddVector class -
//================================================================================================================================

class ShipyardAddVector {
public:
    ShipyardAddVector();
    ~ShipyardAddVector();

    ShipyardAddItem* get_shipyard_add(ShipyardAddKey key);
    const ShipyardAddItem* get_shipyard_add(ShipyardAddKey key) const;
    ShipyardAddKey get_next_new_shipyard_add_key();

    void return_shipyard_add(ShipyardAddKey key);
    ShipyardAddItem* get_page(u16 page_idx);
    const ShipyardAddItem* get_page(u16 page_idx) const;

    static const u16 MAX_PAGES = 256;
    static const u16 SHIPYARD_ADD_ITEMS_PER_PAGE = 256;

private:
    friend class ShipyardAddVectorTester;

    ShipyardAddItem* m_pages[MAX_PAGES];
    u8* m_exists_pages[MAX_PAGES];
    u16 m_shipyard_add_count;
    u16 m_head_shipyard_add_idx;
    u16 m_page_count;

    u16* m_recycled_pages[MAX_PAGES];
    u16 m_recycled_shipyard_add_count;
    u16 m_recycled_page_count;
};

#endif // SHIPYARD_ADD_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
