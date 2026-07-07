//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "unit_array.h"

//================================================================================================================================
//=> - UnitArray implementation -
//================================================================================================================================

UnitArray::UnitArray() : 
    m_unit_count(0), 
    m_head_unit_idx(0),
    m_page_count(0),
    m_recycled_unit_count(0),
    m_recycled_page_count(0) {
        
    for (u16 i = 0; i < MAX_PAGES; ++i) {
        m_pages[i] = nullptr;
        m_recycled_pages[i] = nullptr;
    }
}

UnitArray::~UnitArray() {
    for (u16 i = 0; i < MAX_PAGES; ++i) {
        if (m_pages[i] != nullptr) {
            delete[] m_pages[i];
            m_pages[i] = nullptr;
        }
        if (m_recycled_pages[i] != nullptr) {
            delete[] m_recycled_pages[i];
            m_recycled_pages[i] = nullptr;
        }
    }
}

Unit* UnitArray::get_unit(u16 unit_idx) {
    if (unit_idx >= m_head_unit_idx) {
        return nullptr;
    }

    u16 page = static_cast<u16>(unit_idx >> 8);
    u16 slot = static_cast<u16>(unit_idx & 0xFF);
    if (page >= MAX_PAGES) {
        return nullptr;
    }
    Unit* page_ptr = m_pages[page];
    if (!page_ptr) {
        return nullptr;
    }
    Unit* unit = &page_ptr[slot];
    if (unit->exists == 0) {
        return nullptr;
    }
    unit->exists = 1;
    return unit;
}

const Unit* UnitArray::get_unit(u16 unit_idx) const {
    if (unit_idx >= m_head_unit_idx) {
        return nullptr;
    }

    u16 page = static_cast<u16>(unit_idx >> 8);
    u16 slot = static_cast<u16>(unit_idx & 0xFF);
    if (page >= MAX_PAGES) {
        return nullptr;
    }
    Unit* page_ptr = m_pages[page];
    if (!page_ptr) {
        return nullptr;
    }
    const Unit* unit = &page_ptr[slot];
    if (unit->exists == 0) {
        return nullptr;
    }
    return unit;
}

u16 UnitArray::get_next_new_unit_idx() {
    if (m_recycled_unit_count > 0) {
        m_recycled_unit_count = static_cast<u16>(m_recycled_unit_count - 1);
        u16 recycled_pos = m_recycled_unit_count;
        u16 recycled_page = static_cast<u16>(recycled_pos >> 8);
        u16 recycled_slot = static_cast<u16>(recycled_pos & 0xFF);

        if (!m_recycled_pages[recycled_page]) {
            return 0;
        }

        u16 recycled_idx = m_recycled_pages[recycled_page][recycled_slot];
        u16 unit_page = static_cast<u16>(recycled_idx >> 8);
        u16 unit_slot = static_cast<u16>(recycled_idx & 0xFF);

        if (unit_page < MAX_PAGES && m_pages[unit_page]) {
            Unit* unit = &m_pages[unit_page][unit_slot];
            unit->exists = 1;
            unit->id = 0;
            m_unit_count = static_cast<u16>(m_unit_count + 1);
            return recycled_idx;
        }

        return 0;
    }

    u16 idx = m_head_unit_idx;
    u16 page = static_cast<u16>(idx >> 8);
    u16 slot = static_cast<u16>(idx & 0xFF);
    if (page >= MAX_PAGES) {
        return 0;
    }
    if (!m_pages[page]) {
        m_pages[page] = new Unit[UNITS_PER_PAGE];
        for (u16 i = 0; i < UNITS_PER_PAGE; ++i) {
            m_pages[page][i].exists = 0;
            m_pages[page][i].id = 0;
            m_pages[page][i].idx = static_cast<u16>((page << 8) | i);
        }
        m_page_count = static_cast<u16>(m_page_count + 1);
    }
    m_pages[page][slot].exists = 1;
    m_pages[page][slot].id = 0;

    m_head_unit_idx = static_cast<u16>(m_head_unit_idx + 1);
    m_unit_count = static_cast<u16>(m_unit_count + 1);
    return idx;
}

u16 UnitArray::get_page_count() const {
    return m_page_count;
}

u16 UnitArray::get_unit_count() const {
    return m_unit_count;
}

u16 UnitArray::get_head_unit_count() const {
    return m_head_unit_idx;
}

Unit* UnitArray::get_page(u16 page_idx) {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

const Unit* UnitArray::get_page(u16 page_idx) const {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

void UnitArray::return_unit(Unit* unit) {
    if (!unit) {
        return;
    }

    if (!unit->exists) {
        return;
    }

    unit->exists = 0;
    unit->id = 0;
    const u16 recycled_idx = unit->idx;

    u16 push_pos = m_recycled_unit_count;
    u16 push_page = static_cast<u16>(push_pos >> 8);
    u16 push_slot = static_cast<u16>(push_pos & 0xFF);
    if (push_page >= MAX_PAGES) {
        return;
    }

    if (!m_recycled_pages[push_page]) {
        m_recycled_pages[push_page] = new u16[UNITS_PER_PAGE];
        for (u16 i = 0; i < UNITS_PER_PAGE; ++i) {
            m_recycled_pages[push_page][i] = 0;
        }
        m_recycled_page_count = static_cast<u16>(m_recycled_page_count + 1);
    }

    m_recycled_pages[push_page][push_slot] = recycled_idx;
    m_recycled_unit_count = static_cast<u16>(m_recycled_unit_count + 1);
    m_unit_count = static_cast<u16>(m_unit_count - 1);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================