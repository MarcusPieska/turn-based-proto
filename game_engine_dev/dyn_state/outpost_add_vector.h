//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef OUTPOST_ADD_VECTOR_H
#define OUTPOST_ADD_VECTOR_H

#include "outpost_add_vector_key.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - OutpostAddItem struct -
//================================================================================================================================

typedef struct OutpostAddItem {
    u8 x_pos = 0;
    u8 y_pos = 0;
    u16 area_idx = 0;
    u32 dev_flags = 0;
} OutpostAddItem;

//================================================================================================================================
//=> - OutpostAddVector class -
//================================================================================================================================

class OutpostAddVector {
public:
    OutpostAddVector();
    ~OutpostAddVector();

    OutpostAddItem* get_outpost_add(OutpostAddKey key);
    const OutpostAddItem* get_outpost_add(OutpostAddKey key) const;
    OutpostAddKey get_next_new_outpost_add_key();

    void return_outpost_add(OutpostAddKey key);
    OutpostAddItem* get_page(u16 page_idx);
    const OutpostAddItem* get_page(u16 page_idx) const;

    static const u16 MAX_PAGES = 256;
    static const u16 OUTPOST_ADD_ITEMS_PER_PAGE = 256;

private:
    friend class OutpostAddVectorTester;

    OutpostAddItem* m_pages[MAX_PAGES];
    u8* m_exists_pages[MAX_PAGES];
    u16 m_outpost_add_count;
    u16 m_head_outpost_add_idx;
    u16 m_page_count;

    u16* m_recycled_pages[MAX_PAGES];
    u16 m_recycled_outpost_add_count;
    u16 m_recycled_page_count;
};

#endif // OUTPOST_ADD_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
