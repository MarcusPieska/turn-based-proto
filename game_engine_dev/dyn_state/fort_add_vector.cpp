//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "fort_add_vector.h"

//================================================================================================================================
//=> - FortAddVector implementation -
//================================================================================================================================

FortAddVector::FortAddVector() :
    m_fort_add_count(0),
    m_head_fort_add_idx(1),
    m_page_count(0),
    m_recycled_fort_add_count(0),
    m_recycled_page_count(0) {

    for (u16 i = 0; i < MAX_PAGES; ++i) {
        m_pages[i] = nullptr;
        m_exists_pages[i] = nullptr;
        m_recycled_pages[i] = nullptr;
    }
}

FortAddVector::~FortAddVector() {
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

FortAddItem* FortAddVector::get_fort_add(FortAddKey key) {
    if (key.is_null()) {
        return nullptr;
    }
    u16 fort_add_idx = key.value();
    if (fort_add_idx >= m_head_fort_add_idx) {
        return nullptr;
    }
    u16 page = static_cast<u16>(fort_add_idx >> 8);
    u16 slot = static_cast<u16>(fort_add_idx & 0xFF);
    if (m_exists_pages[page][slot] == 0) {
        return nullptr;
    }
    return &m_pages[page][slot];
}

const FortAddItem* FortAddVector::get_fort_add(FortAddKey key) const {
    if (key.is_null()) {
        return nullptr;
    }
    u16 fort_add_idx = key.value();
    if (fort_add_idx >= m_head_fort_add_idx) {
        return nullptr;
    }
    u16 page = static_cast<u16>(fort_add_idx >> 8);
    u16 slot = static_cast<u16>(fort_add_idx & 0xFF);
    if (m_exists_pages[page][slot] == 0) {
        return nullptr;
    }
    return &m_pages[page][slot];
}

FortAddKey FortAddVector::get_next_new_fort_add_key() {
    if (m_recycled_fort_add_count > 0) {
        m_recycled_fort_add_count = static_cast<u16>(m_recycled_fort_add_count - 1);
        u16 recycled_pos = m_recycled_fort_add_count;
        u16 recycled_page = static_cast<u16>(recycled_pos >> 8);
        u16 recycled_slot = static_cast<u16>(recycled_pos & 0xFF);

        if (!m_recycled_pages[recycled_page]) {
            return FortAddKey::None();
        }

        u16 recycled_idx = m_recycled_pages[recycled_page][recycled_slot];
        u16 add_page = static_cast<u16>(recycled_idx >> 8);
        u16 add_slot = static_cast<u16>(recycled_idx & 0xFF);

        if (add_page < MAX_PAGES && m_pages[add_page] && m_exists_pages[add_page]) {
            FortAddItem* add = &m_pages[add_page][add_slot];
            m_exists_pages[add_page][add_slot] = 1;
            *add = FortAddItem{};
            m_fort_add_count = static_cast<u16>(m_fort_add_count + 1);
            return FortAddKey::from_raw(recycled_idx);
        }

        return FortAddKey::None();
    }

    u16 idx = m_head_fort_add_idx;
    u16 page = static_cast<u16>(idx >> 8);
    u16 slot = static_cast<u16>(idx & 0xFF);
    if (!m_pages[page]) {
        m_pages[page] = new FortAddItem[FORT_ADD_ITEMS_PER_PAGE]();
        m_exists_pages[page] = new u8[FORT_ADD_ITEMS_PER_PAGE]();
        m_page_count = static_cast<u16>(m_page_count + 1);
    }
    m_exists_pages[page][slot] = 1;
    m_pages[page][slot] = FortAddItem{};

    m_head_fort_add_idx = static_cast<u16>(m_head_fort_add_idx + 1);
    m_fort_add_count = static_cast<u16>(m_fort_add_count + 1);
    return FortAddKey::from_raw(idx);
}

FortAddItem* FortAddVector::get_page(u16 page_idx) {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

const FortAddItem* FortAddVector::get_page(u16 page_idx) const {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

void FortAddVector::return_fort_add(FortAddKey key) {
    if (key.is_null()) {
        return;
    }
    u16 fort_add_idx = key.value();
    if (fort_add_idx >= m_head_fort_add_idx) {
        return;
    }
    u16 page = static_cast<u16>(fort_add_idx >> 8);
    u16 slot = static_cast<u16>(fort_add_idx & 0xFF);
    if (m_exists_pages[page][slot] == 0) {
        return;
    }

    m_exists_pages[page][slot] = 0;
    m_pages[page][slot] = FortAddItem{};

    u16 push_pos = m_recycled_fort_add_count;
    u16 push_page = static_cast<u16>(push_pos >> 8);
    u16 push_slot = static_cast<u16>(push_pos & 0xFF);
    if (!m_recycled_pages[push_page]) {
        m_recycled_pages[push_page] = new u16[FORT_ADD_ITEMS_PER_PAGE];
        for (u16 i = 0; i < FORT_ADD_ITEMS_PER_PAGE; ++i) {
            m_recycled_pages[push_page][i] = 0;
        }
        m_recycled_page_count = static_cast<u16>(m_recycled_page_count + 1);
    }

    m_recycled_pages[push_page][push_slot] = fort_add_idx;
    m_recycled_fort_add_count = static_cast<u16>(m_recycled_fort_add_count + 1);
    m_fort_add_count = static_cast<u16>(m_fort_add_count - 1);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================