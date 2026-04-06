//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef FORT_ADD_VECTOR_H
#define FORT_ADD_VECTOR_H

#include "fort_add_vector_key.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - FortAddItem struct -
//================================================================================================================================

typedef struct FortAddItem {
    u8 x_pos = 0;
    u8 y_pos = 0;
    u16 area_idx = 0;
    u32 dev_flags = 0;
} FortAddItem;

//================================================================================================================================
//=> - FortAddVector class -
//================================================================================================================================

class FortAddVector {
public:
    FortAddVector();
    ~FortAddVector();

    FortAddItem* get_fort_add(FortAddKey key);
    const FortAddItem* get_fort_add(FortAddKey key) const;
    FortAddKey get_next_new_fort_add_key();

    void return_fort_add(FortAddKey key);
    FortAddItem* get_page(u16 page_idx);
    const FortAddItem* get_page(u16 page_idx) const;

    static const u16 MAX_PAGES = 256;
    static const u16 FORT_ADD_ITEMS_PER_PAGE = 256;

private:
    friend class FortAddVectorTester;

    FortAddItem* m_pages[MAX_PAGES];
    u8* m_exists_pages[MAX_PAGES];
    u16 m_fort_add_count;
    u16 m_head_fort_add_idx;
    u16 m_page_count;

    u16* m_recycled_pages[MAX_PAGES];
    u16 m_recycled_fort_add_count;
    u16 m_recycled_page_count;
};

#endif // FORT_ADD_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
