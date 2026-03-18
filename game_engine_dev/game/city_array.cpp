//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "city_array.h"

//================================================================================================================================
//=> - CityArray implementation -
//================================================================================================================================

CityArray::CityArray() : 
    m_city_count(0), 
    m_page_count(0) {
        
    for (u16 i = 0; i < MAX_PAGES; ++i) {
        m_pages[i] = nullptr;
    }
}

CityArray::~CityArray() {
    for (u16 i = 0; i < MAX_PAGES; ++i) {
        if (m_pages[i] != nullptr) {
            delete[] m_pages[i];
            m_pages[i] = nullptr;
        }
    }
}

City* CityArray::get_city(u16 city_idx) {
    u16 page = static_cast<u16>(city_idx >> 8);
    u16 slot = static_cast<u16>(city_idx & 0xFF);
    if (page >= MAX_PAGES) {
        return nullptr;
    }
    City* page_ptr = m_pages[page];
    if (!page_ptr) {
        return nullptr;
    }
    return &page_ptr[slot];
}

const City* CityArray::get_city(u16 city_idx) const {
    u16 page = static_cast<u16>(city_idx >> 8);
    u16 slot = static_cast<u16>(city_idx & 0xFF);
    if (page >= MAX_PAGES) {
        return nullptr;
    }
    City* page_ptr = m_pages[page];
    if (!page_ptr) {
        return nullptr;
    }
    return &page_ptr[slot];
}

u16 CityArray::get_next_new_city_idx() {
    u16 idx = m_city_count;
    u16 page = static_cast<u16>(idx >> 8);
    u16 slot = static_cast<u16>(idx & 0xFF);
    if (page >= MAX_PAGES) {
        return 0;
    }
    if (!m_pages[page]) {
        m_pages[page] = new City[CITIES_PER_PAGE];
        m_page_count = static_cast<u16>(m_page_count + 1);
    }
    m_city_count = static_cast<u16>(m_city_count + 1);
    return idx;
}

u16 CityArray::get_page_count() const {
    return m_page_count;
}

u16 CityArray::get_city_count() const {
    return m_city_count;
}

City* CityArray::get_page(u16 page_idx) {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

const City* CityArray::get_page(u16 page_idx) const {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

//================================================================================================================================
//=> - End -
//================================================================================================================================