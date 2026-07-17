//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_array.h"
#include "city.h"

//================================================================================================================================
//=> - City -
//================================================================================================================================

City::City () :
    m_owner(U16_KEY_NULL),
    m_x(U16_KEY_NULL),
    m_y(U16_KEY_NULL) {
    clr_net();
}

City::~City () {
}

void City::init (u16 owner, u16 x, u16 y) {
    m_owner = owner;
    m_x = x;
    m_y = y;
    clr_net();
}

u16 City::get_owner () const {
    return m_owner;
}

u16 City::get_x () const {
    return m_x;
}

u16 City::get_y () const {
    return m_y;
}

CityNetLinks& City::links () {
    return m_links;
}

const CityNetLinks& City::links () const {
    return m_links;
}

void City::clr_net () {
    m_links.m_ne = U16_KEY_NULL;
    m_links.m_nw = U16_KEY_NULL;
    m_links.m_se = U16_KEY_NULL;
    m_links.m_sw = U16_KEY_NULL;
}

//================================================================================================================================
//=> - CityArray -
//================================================================================================================================

CityArray::CityArray () :
    m_city_count(0),
    m_page_count(0) {
    for (u16 i = 0; i < MAX_PAGES; ++i) {
        m_pages[i] = nullptr;
    }
}

CityArray::~CityArray () {
    for (u16 i = 0; i < MAX_PAGES; ++i) {
        if (m_pages[i] != nullptr) {
            delete[] m_pages[i];
            m_pages[i] = nullptr;
        }
    }
}

City* CityArray::get_city (u16 city_idx) {
    const u16 page = static_cast<u16>(city_idx >> 8);
    const u16 slot = static_cast<u16>(city_idx & 0xFF);
    if (page >= MAX_PAGES || m_pages[page] == nullptr) {
        return nullptr;
    }
    return &m_pages[page][slot];
}

const City* CityArray::get_city (u16 city_idx) const {
    const u16 page = static_cast<u16>(city_idx >> 8);
    const u16 slot = static_cast<u16>(city_idx & 0xFF);
    if (page >= MAX_PAGES || m_pages[page] == nullptr) {
        return nullptr;
    }
    return &m_pages[page][slot];
}

u16 CityArray::get_next_new_city_idx () {
    const u16 idx = m_city_count;
    const u16 page = static_cast<u16>(idx >> 8);
    if (page >= MAX_PAGES) {
        return U16_KEY_NULL;
    }
    if (m_pages[page] == nullptr) {
        m_pages[page] = new City[CITIES_PER_PAGE];
        m_page_count = static_cast<u16>(m_page_count + 1);
    }
    m_city_count = static_cast<u16>(m_city_count + 1);
    return idx;
}

u16 CityArray::get_page_count () const {
    return m_page_count;
}

u16 CityArray::get_city_count () const {
    return m_city_count;
}

City* CityArray::get_page (u16 page_idx) {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

const City* CityArray::get_page (u16 page_idx) const {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
