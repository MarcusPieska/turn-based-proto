//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_ADD_VECTOR_H
#define UNIT_ADD_VECTOR_H

#include "unit_add_vector_key.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - UnitAddItem struct -
//================================================================================================================================

typedef struct UnitAddItem {
    u8 x_pos = 0;
    u8 y_pos = 0;
    u16 area_idx = 0;

    u16 unit_idx = 0;
    u16 civ_idx = 0;

    u16 next_unit_list_idx = 0;
    u16 health : 10;
    u16 discipline : 6;

    UnitAddItem() : health(0), discipline(0) {}
} UnitAddItem;

//================================================================================================================================
//=> - UnitAddVector class -
//================================================================================================================================

class UnitAddVector {
public:
    UnitAddVector();
    ~UnitAddVector();

    UnitAddItem* get_unit_add(UnitAddKey key);
    const UnitAddItem* get_unit_add(UnitAddKey key) const;
    UnitAddKey get_next_new_unit_add_key();

    void return_unit_add(UnitAddKey key);
    UnitAddItem* get_page(u16 page_idx);
    const UnitAddItem* get_page(u16 page_idx) const;

    static const u16 MAX_PAGES = 256;
    static const u16 UNIT_ADD_ITEMS_PER_PAGE = 256;

private:
    friend class UnitAddVectorTester;

    UnitAddItem* m_pages[MAX_PAGES];
    u8* m_exists_pages[MAX_PAGES];
    u16 m_unit_add_count;
    u16 m_head_unit_add_idx;
    u16 m_page_count;

    u16* m_recycled_pages[MAX_PAGES];
    u16 m_recycled_unit_add_count;
    u16 m_recycled_page_count;
};

#endif // UNIT_ADD_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
