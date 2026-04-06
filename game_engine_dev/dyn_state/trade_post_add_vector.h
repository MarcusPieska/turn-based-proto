//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TRADE_POST_ADD_VECTOR_H
#define TRADE_POST_ADD_VECTOR_H

#include "trade_post_add_vector_key.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - TradePostAddItem struct -
//================================================================================================================================

typedef struct TradePostAddItem {
    u8 x_pos = 0;
    u8 y_pos = 0;
    u16 area_idx = 0;
    u32 dev_flags = 0;
} TradePostAddItem;

//================================================================================================================================
//=> - TradePostAddVector class -
//================================================================================================================================

class TradePostAddVector {
public:
    TradePostAddVector();
    ~TradePostAddVector();

    TradePostAddItem* get_trade_post_add(TradePostAddKey key);
    const TradePostAddItem* get_trade_post_add(TradePostAddKey key) const;
    TradePostAddKey get_next_new_trade_post_add_key();

    void return_trade_post_add(TradePostAddKey key);
    TradePostAddItem* get_page(u16 page_idx);
    const TradePostAddItem* get_page(u16 page_idx) const;

    static const u16 MAX_PAGES = 256;
    static const u16 TRADE_POST_ADD_ITEMS_PER_PAGE = 256;

private:
    friend class TradePostAddVectorTester;

    TradePostAddItem* m_pages[MAX_PAGES];
    u8* m_exists_pages[MAX_PAGES];
    u16 m_trade_post_add_count;
    u16 m_head_trade_post_add_idx;
    u16 m_page_count;

    u16* m_recycled_pages[MAX_PAGES];
    u16 m_recycled_trade_post_add_count;
    u16 m_recycled_page_count;
};

#endif // TRADE_POST_ADD_VECTOR_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
