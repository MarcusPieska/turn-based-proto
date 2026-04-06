//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "unit_add_vector.h"

//================================================================================================================================
//=> - UnitAddVector implementation -
//================================================================================================================================

UnitAddVector::UnitAddVector() :
    m_unit_add_count(0),
    m_head_unit_add_idx(1),
    m_page_count(0),
    m_recycled_unit_add_count(0),
    m_recycled_page_count(0) {

    for (u16 i = 0; i < MAX_PAGES; ++i) {
        m_pages[i] = nullptr;
        m_exists_pages[i] = nullptr;
        m_recycled_pages[i] = nullptr;
    }
}

UnitAddVector::~UnitAddVector() {
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

UnitAddItem* UnitAddVector::get_unit_add(UnitAddKey key) {
    if (key.is_null()) {
        return nullptr;
    }
    u16 unit_add_idx = key.value();
    if (unit_add_idx >= m_head_unit_add_idx) {
        return nullptr;
    }
    u16 page = static_cast<u16>(unit_add_idx >> 8);
    u16 slot = static_cast<u16>(unit_add_idx & 0xFF);
    if (m_exists_pages[page][slot] == 0) {
        return nullptr;
    }
    return &m_pages[page][slot];
}

const UnitAddItem* UnitAddVector::get_unit_add(UnitAddKey key) const {
    if (key.is_null()) {
        return nullptr;
    }
    u16 unit_add_idx = key.value();
    if (unit_add_idx >= m_head_unit_add_idx) {
        return nullptr;
    }
    u16 page = static_cast<u16>(unit_add_idx >> 8);
    u16 slot = static_cast<u16>(unit_add_idx & 0xFF);
    if (m_exists_pages[page][slot] == 0) {
        return nullptr;
    }
    return &m_pages[page][slot];
}

UnitAddKey UnitAddVector::get_next_new_unit_add_key() {
    if (m_recycled_unit_add_count > 0) {
        m_recycled_unit_add_count = static_cast<u16>(m_recycled_unit_add_count - 1);
        u16 recycled_pos = m_recycled_unit_add_count;
        u16 recycled_page = static_cast<u16>(recycled_pos >> 8);
        u16 recycled_slot = static_cast<u16>(recycled_pos & 0xFF);

        if (!m_recycled_pages[recycled_page]) {
            return UnitAddKey::None();
        }

        u16 recycled_idx = m_recycled_pages[recycled_page][recycled_slot];
        u16 add_page = static_cast<u16>(recycled_idx >> 8);
        u16 add_slot = static_cast<u16>(recycled_idx & 0xFF);

        if (add_page < MAX_PAGES && m_pages[add_page] && m_exists_pages[add_page]) {
            UnitAddItem* add = &m_pages[add_page][add_slot];
            m_exists_pages[add_page][add_slot] = 1;
            *add = UnitAddItem{};
            m_unit_add_count = static_cast<u16>(m_unit_add_count + 1);
            return UnitAddKey::from_raw(recycled_idx);
        }

        return UnitAddKey::None();
    }

    u16 idx = m_head_unit_add_idx;
    u16 page = static_cast<u16>(idx >> 8);
    u16 slot = static_cast<u16>(idx & 0xFF);
    if (!m_pages[page]) {
        m_pages[page] = new UnitAddItem[UNIT_ADD_ITEMS_PER_PAGE]();
        m_exists_pages[page] = new u8[UNIT_ADD_ITEMS_PER_PAGE]();
        m_page_count = static_cast<u16>(m_page_count + 1);
    }
    m_exists_pages[page][slot] = 1;
    m_pages[page][slot] = UnitAddItem{};

    m_head_unit_add_idx = static_cast<u16>(m_head_unit_add_idx + 1);
    m_unit_add_count = static_cast<u16>(m_unit_add_count + 1);
    return UnitAddKey::from_raw(idx);
}

UnitAddItem* UnitAddVector::get_page(u16 page_idx) {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

const UnitAddItem* UnitAddVector::get_page(u16 page_idx) const {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

void UnitAddVector::return_unit_add(UnitAddKey key) {
    if (key.is_null()) {
        return;
    }
    u16 unit_add_idx = key.value();
    if (unit_add_idx >= m_head_unit_add_idx) {
        return;
    }
    u16 page = static_cast<u16>(unit_add_idx >> 8);
    u16 slot = static_cast<u16>(unit_add_idx & 0xFF);
    if (m_exists_pages[page][slot] == 0) {
        return;
    }

    m_exists_pages[page][slot] = 0;
    m_pages[page][slot] = UnitAddItem{};

    u16 push_pos = m_recycled_unit_add_count;
    u16 push_page = static_cast<u16>(push_pos >> 8);
    u16 push_slot = static_cast<u16>(push_pos & 0xFF);
    if (!m_recycled_pages[push_page]) {
        m_recycled_pages[push_page] = new u16[UNIT_ADD_ITEMS_PER_PAGE];
        for (u16 i = 0; i < UNIT_ADD_ITEMS_PER_PAGE; ++i) {
            m_recycled_pages[push_page][i] = 0;
        }
        m_recycled_page_count = static_cast<u16>(m_recycled_page_count + 1);
    }

    m_recycled_pages[push_page][push_slot] = unit_add_idx;
    m_recycled_unit_add_count = static_cast<u16>(m_recycled_unit_add_count + 1);
    m_unit_add_count = static_cast<u16>(m_unit_add_count - 1);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================