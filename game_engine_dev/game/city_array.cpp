//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "city_array.h"
#include "city.h"
#include "general_bit_bank.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - CityArray implementation -
//================================================================================================================================

CityArray::CityArray() :
    m_city_count(0),
    m_page_count(0),
    m_flag_bank(nullptr),
    m_res_bank(nullptr),
    m_bld_bank(nullptr) {
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
    clear_banks();
}

void CityArray::clear_banks () {
    City::clear_assess_scratch();
    delete m_flag_bank;
    delete m_res_bank;
    delete m_bld_bank;
    m_flag_bank = nullptr;
    m_res_bank = nullptr;
    m_bld_bank = nullptr;
}

bool CityArray::bind_statics (const RuntimeStatics& st) {
    clear_banks();
    m_flag_bank = new GeneralBitBank(st.city_flag().get_item_count());
    m_res_bank = new GeneralBitBank(st.resource().get_item_count());
    m_bld_bank = new GeneralBitBank(st.building().get_item_count());
    City::bind_statics(st);
    City::bind_banks(m_flag_bank, m_res_bank, m_bld_bank);
    return true;
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
    if (page >= MAX_PAGES) {
        return 0;
    }
    if (!m_pages[page]) {
        m_pages[page] = new City[CITIES_PER_PAGE];
        m_page_count = static_cast<u16>(m_page_count + 1);
    }
    if (m_flag_bank != nullptr) {
        m_flag_bank->claim_batch();
        m_res_bank->claim_batch();
        m_bld_bank->claim_batch();
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

GeneralBitBank* CityArray::get_flag_bank () {
    return m_flag_bank;
}

const GeneralBitBank* CityArray::get_flag_bank () const {
    return m_flag_bank;
}

GeneralBitBank* CityArray::get_res_bank () {
    return m_res_bank;
}

const GeneralBitBank* CityArray::get_res_bank () const {
    return m_res_bank;
}

GeneralBitBank* CityArray::get_bld_bank () {
    return m_bld_bank;
}

const GeneralBitBank* CityArray::get_bld_bank () const {
    return m_bld_bank;
}

void CityArray::set_building_flag (u16 city_idx, u16 bld_idx) {
    if (m_bld_bank == nullptr) {
        return;
    }
    m_bld_bank->set_flag(city_idx, bld_idx);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
