//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef [MACRO_PREFIX]_VECTOR_H
#define [MACRO_PREFIX]_VECTOR_H

#include "[MEMBER_TAG]_vector_key.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - [CLASS_NAME_PREFIX]Item struct -
//================================================================================================================================

struct [CLASS_NAME_PREFIX]Item {
    u16 [MEMBER_TAG]_idx;
};

//================================================================================================================================
//=> - [CLASS_NAME_PREFIX]Vector class -
//================================================================================================================================

class [CLASS_NAME_PREFIX]Vector {
public:
    [CLASS_NAME_PREFIX]Vector();
    ~[CLASS_NAME_PREFIX]Vector();

    [CLASS_NAME_PREFIX]Item* get_[MEMBER_TAG]([CLASS_NAME_PREFIX]Key key);
    const [CLASS_NAME_PREFIX]Item* get_[MEMBER_TAG]([CLASS_NAME_PREFIX]Key key) const;
    [CLASS_NAME_PREFIX]Key get_next_new_[MEMBER_TAG]_key();

    void return_[MEMBER_TAG]([CLASS_NAME_PREFIX]Key key);
    [CLASS_NAME_PREFIX]Item* get_page(u16 page_idx);
    const [CLASS_NAME_PREFIX]Item* get_page(u16 page_idx) const;

    static const u16 MAX_PAGES = 256;
    static const u16 [MACRO_PREFIX]_ITEMS_PER_PAGE = 256;

private:
    friend class [CLASS_NAME_PREFIX]VectorTester;

    [CLASS_NAME_PREFIX]Item* m_pages[MAX_PAGES];
    u8* m_exists_pages[MAX_PAGES];
    u16 m_[MEMBER_TAG]_count;
    u16 m_head_[MEMBER_TAG]_idx;
    u16 m_page_count;

    u16* m_recycled_pages[MAX_PAGES];
    u16 m_recycled_[MEMBER_TAG]_count;
    u16 m_recycled_page_count;
};

#endif // [MACRO_PREFIX]_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
