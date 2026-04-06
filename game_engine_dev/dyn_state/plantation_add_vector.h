//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PLANTATION_ADD_VECTOR_H
#define PLANTATION_ADD_VECTOR_H

#include "plantation_add_vector_key.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - PlantationAddItem struct -
//================================================================================================================================

typedef struct PlantationAddItem {
    u8 x_pos = 0;
    u8 y_pos = 0;
    u16 area_idx = 0;
    u32 dev_flags = 0;
} PlantationAddItem;

//================================================================================================================================
//=> - PlantationAddVector class -
//================================================================================================================================

class PlantationAddVector {
public:
    PlantationAddVector();
    ~PlantationAddVector();

    PlantationAddItem* get_plantation_add(PlantationAddKey key);
    const PlantationAddItem* get_plantation_add(PlantationAddKey key) const;
    PlantationAddKey get_next_new_plantation_add_key();

    void return_plantation_add(PlantationAddKey key);
    PlantationAddItem* get_page(u16 page_idx);
    const PlantationAddItem* get_page(u16 page_idx) const;

    static const u16 MAX_PAGES = 256;
    static const u16 PLANTATION_ADD_ITEMS_PER_PAGE = 256;

private:
    friend class PlantationAddVectorTester;

    PlantationAddItem* m_pages[MAX_PAGES];
    u8* m_exists_pages[MAX_PAGES];
    u16 m_plantation_add_count;
    u16 m_head_plantation_add_idx;
    u16 m_page_count;

    u16* m_recycled_pages[MAX_PAGES];
    u16 m_recycled_plantation_add_count;
    u16 m_recycled_page_count;
};

#endif // PLANTATION_ADD_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
