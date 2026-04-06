//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILD_ADD_VECTOR_H
#define BUILD_ADD_VECTOR_H

#include "build_add_vector_key.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - BuildAddItem struct -
//================================================================================================================================

typedef struct BuildAddItem {
    u8 x_pos = 0;
    u8 y_pos = 0;
    u16 area_idx = 0;
    u32 dev_flags = 0;
} BuildAddItem;

//================================================================================================================================
//=> - BuildAddVector class -
//================================================================================================================================

class BuildAddVector {
public:
    BuildAddVector();
    ~BuildAddVector();

    BuildAddItem* get_build_add(BuildAddKey key);
    const BuildAddItem* get_build_add(BuildAddKey key) const;
    BuildAddKey get_next_new_build_add_key();

    void return_build_add(BuildAddKey key);
    BuildAddItem* get_page(u16 page_idx);
    const BuildAddItem* get_page(u16 page_idx) const;

    static const u16 MAX_PAGES = 256;
    static const u16 BUILD_ADD_ITEMS_PER_PAGE = 256;

private:
    friend class BuildAddVectorTester;

    BuildAddItem* m_pages[MAX_PAGES];
    u8* m_exists_pages[MAX_PAGES];
    u16 m_build_add_count;
    u16 m_head_build_add_idx;
    u16 m_page_count;

    u16* m_recycled_pages[MAX_PAGES];
    u16 m_recycled_build_add_count;
    u16 m_recycled_page_count;
};

#endif // BUILD_ADD_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
