//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MONASTERY_ADD_VECTOR_H
#define MONASTERY_ADD_VECTOR_H

#include "monastery_add_vector_key.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - MonasteryAddItem struct -
//================================================================================================================================

typedef struct MonasteryAddItem {
    u8 x_pos = 0;
    u8 y_pos = 0;
    u16 area_idx = 0;
    u32 dev_flags = 0;
} MonasteryAddItem;
    
//================================================================================================================================
//=> - MonasteryAddVector class -
//================================================================================================================================

class MonasteryAddVector {
public:
    MonasteryAddVector();
    ~MonasteryAddVector();

    MonasteryAddItem* get_monastery_add(MonasteryAddKey key);
    const MonasteryAddItem* get_monastery_add(MonasteryAddKey key) const;
    MonasteryAddKey get_next_new_monastery_add_key();

    void return_monastery_add(MonasteryAddKey key);
    MonasteryAddItem* get_page(u16 page_idx);
    const MonasteryAddItem* get_page(u16 page_idx) const;

    static const u16 MAX_PAGES = 256;
    static const u16 MONASTERY_ADD_ITEMS_PER_PAGE = 256;

private:
    friend class MonasteryAddVectorTester;

    MonasteryAddItem* m_pages[MAX_PAGES];
    u8* m_exists_pages[MAX_PAGES];
    u16 m_monastery_add_count;
    u16 m_head_monastery_add_idx;
    u16 m_page_count;

    u16* m_recycled_pages[MAX_PAGES];
    u16 m_recycled_monastery_add_count;
    u16 m_recycled_page_count;
};

#endif // MONASTERY_ADD_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
