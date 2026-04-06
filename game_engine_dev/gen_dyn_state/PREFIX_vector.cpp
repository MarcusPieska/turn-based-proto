//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "[MEMBER_TAG]_vector.h"

//================================================================================================================================
//=> - [CLASS_NAME_PREFIX]Vector implementation -
//================================================================================================================================

[CLASS_NAME_PREFIX]Vector::[CLASS_NAME_PREFIX]Vector() :
    m_[MEMBER_TAG]_count(0),
    m_head_[MEMBER_TAG]_idx(1),
    m_page_count(0),
    m_recycled_[MEMBER_TAG]_count(0),
    m_recycled_page_count(0) {

    for (u16 i = 0; i < MAX_PAGES; ++i) {
        m_pages[i] = nullptr;
        m_exists_pages[i] = nullptr;
        m_recycled_pages[i] = nullptr;
    }
}

[CLASS_NAME_PREFIX]Vector::~[CLASS_NAME_PREFIX]Vector() {
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

[CLASS_NAME_PREFIX]Item* [CLASS_NAME_PREFIX]Vector::get_[MEMBER_TAG]([CLASS_NAME_PREFIX]Key key) {
    if (key.is_null()) {
        return nullptr;
    }
    u16 [MEMBER_TAG]_idx = key.value();
    if ([MEMBER_TAG]_idx >= m_head_[MEMBER_TAG]_idx) {
        return nullptr;
    }
    u16 page = static_cast<u16>([MEMBER_TAG]_idx >> 8);
    u16 slot = static_cast<u16>([MEMBER_TAG]_idx & 0xFF);
    if (m_exists_pages[page][slot] == 0) {
        return nullptr;
    }
    return &m_pages[page][slot];
}

const [CLASS_NAME_PREFIX]Item* [CLASS_NAME_PREFIX]Vector::get_[MEMBER_TAG]([CLASS_NAME_PREFIX]Key key) const {
    if (key.is_null()) {
        return nullptr;
    }
    u16 [MEMBER_TAG]_idx = key.value();
    if ([MEMBER_TAG]_idx >= m_head_[MEMBER_TAG]_idx) {
        return nullptr;
    }
    u16 page = static_cast<u16>([MEMBER_TAG]_idx >> 8);
    u16 slot = static_cast<u16>([MEMBER_TAG]_idx & 0xFF);
    if (m_exists_pages[page][slot] == 0) {
        return nullptr;
    }
    return &m_pages[page][slot];
}

[CLASS_NAME_PREFIX]Key [CLASS_NAME_PREFIX]Vector::get_next_new_[MEMBER_TAG]_key() {
    if (m_recycled_[MEMBER_TAG]_count > 0) {
        m_recycled_[MEMBER_TAG]_count = static_cast<u16>(m_recycled_[MEMBER_TAG]_count - 1);
        u16 recycled_pos = m_recycled_[MEMBER_TAG]_count;
        u16 recycled_page = static_cast<u16>(recycled_pos >> 8);
        u16 recycled_slot = static_cast<u16>(recycled_pos & 0xFF);

        if (!m_recycled_pages[recycled_page]) {
            return [CLASS_NAME_PREFIX]Key::None();
        }

        u16 recycled_idx = m_recycled_pages[recycled_page][recycled_slot];
        u16 add_page = static_cast<u16>(recycled_idx >> 8);
        u16 add_slot = static_cast<u16>(recycled_idx & 0xFF);

        if (add_page < MAX_PAGES && m_pages[add_page] && m_exists_pages[add_page]) {
            [CLASS_NAME_PREFIX]Item* add = &m_pages[add_page][add_slot];
            m_exists_pages[add_page][add_slot] = 1;
            *add = [CLASS_NAME_PREFIX]Item{};
            m_[MEMBER_TAG]_count = static_cast<u16>(m_[MEMBER_TAG]_count + 1);
            return [CLASS_NAME_PREFIX]Key::from_raw(recycled_idx);
        }

        return [CLASS_NAME_PREFIX]Key::None();
    }

    u16 idx = m_head_[MEMBER_TAG]_idx;
    u16 page = static_cast<u16>(idx >> 8);
    u16 slot = static_cast<u16>(idx & 0xFF);
    if (!m_pages[page]) {
        m_pages[page] = new [CLASS_NAME_PREFIX]Item[[MACRO_PREFIX]_ITEMS_PER_PAGE]();
        m_exists_pages[page] = new u8[[MACRO_PREFIX]_ITEMS_PER_PAGE]();
        m_page_count = static_cast<u16>(m_page_count + 1);
    }
    m_exists_pages[page][slot] = 1;
    m_pages[page][slot] = [CLASS_NAME_PREFIX]Item{};

    m_head_[MEMBER_TAG]_idx = static_cast<u16>(m_head_[MEMBER_TAG]_idx + 1);
    m_[MEMBER_TAG]_count = static_cast<u16>(m_[MEMBER_TAG]_count + 1);
    return [CLASS_NAME_PREFIX]Key::from_raw(idx);
}

[CLASS_NAME_PREFIX]Item* [CLASS_NAME_PREFIX]Vector::get_page(u16 page_idx) {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

const [CLASS_NAME_PREFIX]Item* [CLASS_NAME_PREFIX]Vector::get_page(u16 page_idx) const {
    if (page_idx >= MAX_PAGES) {
        return nullptr;
    }
    return m_pages[page_idx];
}

void [CLASS_NAME_PREFIX]Vector::return_[MEMBER_TAG]([CLASS_NAME_PREFIX]Key key) {
    if (key.is_null()) {
        return;
    }
    u16 [MEMBER_TAG]_idx = key.value();
    if ([MEMBER_TAG]_idx >= m_head_[MEMBER_TAG]_idx) {
        return;
    }
    u16 page = static_cast<u16>([MEMBER_TAG]_idx >> 8);
    u16 slot = static_cast<u16>([MEMBER_TAG]_idx & 0xFF);
    if (m_exists_pages[page][slot] == 0) {
        return;
    }

    m_exists_pages[page][slot] = 0;
    m_pages[page][slot] = [CLASS_NAME_PREFIX]Item{};

    u16 push_pos = m_recycled_[MEMBER_TAG]_count;
    u16 push_page = static_cast<u16>(push_pos >> 8);
    u16 push_slot = static_cast<u16>(push_pos & 0xFF);
    if (!m_recycled_pages[push_page]) {
        m_recycled_pages[push_page] = new u16[[MACRO_PREFIX]_ITEMS_PER_PAGE];
        for (u16 i = 0; i < [MACRO_PREFIX]_ITEMS_PER_PAGE; ++i) {
            m_recycled_pages[push_page][i] = 0;
        }
        m_recycled_page_count = static_cast<u16>(m_recycled_page_count + 1);
    }

    m_recycled_pages[push_page][push_slot] = [MEMBER_TAG]_idx;
    m_recycled_[MEMBER_TAG]_count = static_cast<u16>(m_recycled_[MEMBER_TAG]_count + 1);
    m_[MEMBER_TAG]_count = static_cast<u16>(m_[MEMBER_TAG]_count - 1);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================