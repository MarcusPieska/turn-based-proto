//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "trade_post_add_vector.h"

//================================================================================================================================
//=> - TradePostAddVector implementation -
//================================================================================================================================

TradePostAddVector::TradePostAddVector() :
    m_trade_post_add_count(0),
    m_head_trade_post_add_idx(1),
    m_page_count(0),
    m_recycled_trade_post_add_count(0),
    m_recycled_page_count(0) {

    for (u16 i = 0; i < MAX_PAGES; ++i) {
        m_pages[i] = nullptr;
        m_exists_pages[i] = nullptr;
        m_recycled_pages[i] = nullptr;
    }
}

TradePostAddVector::~TradePostAddVector() {
    for (u16 i = 0; i < MAX_PAGES; ++i) {
        if (m_pages[i] != nullptr) {
            delete[] m_pages[i];
            m_pages[i] = nullptr;
        }
        if (m_exists_pages[i] != nullptr) {
            delete[] m_exists_pages[i];
            m_exists_pages[i] = nullptr;
        }
        if (m_recycled_pages[i] != nullptr) {
            delete[] m_recycled_pages[i];
            m_recycled_pages[i] = nullptr;
        }
    }
}

TradePostAddItem* TradePostAddVector::get_trade_post_add(TradePostAddKey key) {
    if (key.is_null()) {
        return nullptr;
    }
    u16 trade_post_add_idx = key.value();
    if (trade_post_add_idx >= m_head_trade_post_add_idx) {
        return nullptr;
    }
    u16 page = static_cast<u16>(trade_post_add_idx >> 8);
    u16 slot = static_cast<u16>(trade_post_add_idx & 0xFF);
    if (m_exists_pages[page][slot] == 0) {
        return nullptr;
    }
    return &m_pages[page][slot];
}

const TradePostAddItem* TradePostAddVector::get_trade_post_add(TradePostAddKey key) const {
    if (key.is_null()) {
        return nullptr;
    }
    u16 trade_post_add_idx = key.value();
    if (trade_post_add_idx >= m_head_trade_post_add_idx) {
        return nullptr;
    }
    u16 page = static_cast<u16>(trade_post_add_idx >> 8);
    u16 slot = static_cast<u16>(trade_post_add_idx & 0xFF);
    if (m_exists_pages[page][slot] == 0) {
        return nullptr;
    }
    return &m_pages[page][slot];
}

TradePostAddKey TradePostAddVector::get_next_new_trade_post_add_key() {
    if (m_recycled_trade_post_add_count > 0) {
        m_recycled_trade_post_add_count = static_cast<u16>(m_recycled_trade_post_add_count - 1);
        u16 recycled_pos = m_recycled_trade_post_add_count;
        u16 recycled_page = static_cast<u16>(recycled_pos >> 8);
        u16 recycled_slot = static_cast<u16>(recycled_pos & 0xFF);

        if (!m_recycled_pages[recycled_page]) {
            return TradePostAddKey::None();
        }

        u16 recycled_idx = m_recycled_pages[recycled_page][recycled_slot];
        u16 add_page = static_cast<u16>(recycled_idx >> 8);
        u16 add_slot = static_cast<u16>(recycled_idx & 0xFF);

        if (add_page < MAX_PAGES && m_pages[add_page] && m_exists_pages[add_page]) {
            TradePostAddItem* add = &m_pages[add_page][add_slot];
            m_exists_pages[add_page][add_slot] = 1;
            *add = TradePostAddItem{};
            m_trade_post_add_count = static_cast<u16>(m_trade_post_add_count + 1);
            return TradePostAddKey::from_raw(recycled_idx);
        }

        return TradePostAddKey::None();
    }

    u16 idx = m_head_trade_post_add_idx;
    u16 page = static_cast<u16>(idx >> 8);
    u16 slot = static_cast<u16>(idx & 0xFF);
    if (!m_pages[page]) {
        m_pages[page] = new TradePostAddItem[TRADE_POST_ADD_ITEMS_PER_PAGE]();
        m_exists_pages[page] = new u8[TRADE_POST_ADD_ITEMS_PER_PAGE]();
        m_page_count = static_cast<u16>(m_page_count + 1);
    }
    m_exists_pages[page][slot] = 1;
    m_pages[page][slot] = TradePostAddItem{};

    m_head_trade_post_add_idx = static_cast<u16>(m_head_trade_post_add_idx + 1);
    m_trade_post_add_count = static_cast<u16>(m_trade_post_add_count + 1);
    return TradePostAddKey::from_raw(idx);
}

TradePostAddItem* TradePostAddVector::get_page(u16 page_idx) {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

const TradePostAddItem* TradePostAddVector::get_page(u16 page_idx) const {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

void TradePostAddVector::return_trade_post_add(TradePostAddKey key) {
    if (key.is_null()) {
        return;
    }
    u16 trade_post_add_idx = key.value();
    if (trade_post_add_idx >= m_head_trade_post_add_idx) {
        return;
    }
    u16 page = static_cast<u16>(trade_post_add_idx >> 8);
    u16 slot = static_cast<u16>(trade_post_add_idx & 0xFF);
    if (m_exists_pages[page][slot] == 0) {
        return;
    }

    m_exists_pages[page][slot] = 0;
    m_pages[page][slot] = TradePostAddItem{};

    u16 push_pos = m_recycled_trade_post_add_count;
    u16 push_page = static_cast<u16>(push_pos >> 8);
    u16 push_slot = static_cast<u16>(push_pos & 0xFF);
    if (!m_recycled_pages[push_page]) {
        m_recycled_pages[push_page] = new u16[TRADE_POST_ADD_ITEMS_PER_PAGE];
        for (u16 i = 0; i < TRADE_POST_ADD_ITEMS_PER_PAGE; ++i) {
            m_recycled_pages[push_page][i] = 0;
        }
        m_recycled_page_count = static_cast<u16>(m_recycled_page_count + 1);
    }

    m_recycled_pages[push_page][push_slot] = trade_post_add_idx;
    m_recycled_trade_post_add_count = static_cast<u16>(m_recycled_trade_post_add_count + 1);
    m_trade_post_add_count = static_cast<u16>(m_trade_post_add_count - 1);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================