//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MINE_ADD_VECTOR_H
#define MINE_ADD_VECTOR_H

#include "mine_add_vector_key.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - MineAddItem struct -
//================================================================================================================================

typedef struct MineAddItem {
    u8 x_pos = 0;
    u8 y_pos = 0;
    u16 area_idx = 0;
    u32 dev_flags = 0;
} MineAddItem;

//================================================================================================================================
//=> - MineAddVector class -
//================================================================================================================================

class MineAddVector {
public:
    MineAddVector();
    ~MineAddVector();

    MineAddItem* get_mine_add(MineAddKey key);
    const MineAddItem* get_mine_add(MineAddKey key) const;
    MineAddKey get_next_new_mine_add_key();

    void return_mine_add(MineAddKey key);
    MineAddItem* get_page(u16 page_idx);
    const MineAddItem* get_page(u16 page_idx) const;

    static const u16 MAX_PAGES = 256;
    static const u16 MINE_ADD_ITEMS_PER_PAGE = 256;

private:
    friend class MineAddVectorTester;

    MineAddItem* m_pages[MAX_PAGES];
    u8* m_exists_pages[MAX_PAGES];
    u16 m_mine_add_count;
    u16 m_head_mine_add_idx;
    u16 m_page_count;

    u16* m_recycled_pages[MAX_PAGES];
    u16 m_recycled_mine_add_count;
    u16 m_recycled_page_count;
};

#endif // MINE_ADD_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
