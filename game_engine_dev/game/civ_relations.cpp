//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "civ_relations.h"
#include "map_bit_array_overlay.h"

//================================================================================================================================
//=> - CivRelations -
//================================================================================================================================

static const u8 k_rel_bpv = 4;

CivRelations::CivRelations () :
    m_row(nullptr),
    m_n(0u) {
}

CivRelations::~CivRelations () {
    reset(0);
}

void CivRelations::clr_rows () {
    if (m_row == nullptr) {
        return;
    }
    for (u16 i = 0; i < m_n; ++i) {
        delete m_row[i];
        m_row[i] = nullptr;
    }
    delete[] m_row;
    m_row = nullptr;
}

void CivRelations::reset (u16 civ_n) {
    clr_rows ();
    m_n = 0u;
    if (civ_n == 0u) {
        return;
    }
    m_row = new MapBitArrayOverlay*[civ_n];
    if (m_row == nullptr) {
        return;
    }
    for (u16 i = 0; i < civ_n; ++i) {
        m_row[i] = nullptr;
    }
    m_n = civ_n;
}

bool CivRelations::add_civ (u16 i) {
    if (m_row == nullptr || i >= m_n || m_row[i] != nullptr) {
        return false;
    }
    m_row[i] = new MapBitArrayOverlay(m_n, 1u, k_rel_bpv);
    if (m_row[i] == nullptr) {
        return false;
    }
    m_row[i]->fill_all(static_cast<u32>(CivRel::CIV_REL_UNDISCOVERED));
    return true;
}

void CivRelations::remove_civ (u16 i) {
    if (m_row == nullptr || i >= m_n) {
        return;
    }
    delete m_row[i];
    m_row[i] = nullptr;
    for (u16 j = 0; j < m_n; ++j) {
        if (m_row[j] == nullptr) {
            continue;
        }
        m_row[j]->set(i, 0u, static_cast<u32>(CivRel::CIV_REL_UNDISCOVERED));
    }
}

CivRel CivRelations::get (CivStaticDataKey c1, CivStaticDataKey c2) const {
    const u16 a = c1.value();
    const u16 b = c2.value();
    if (m_row == nullptr || a >= m_n || b >= m_n || m_row[a] == nullptr) {
        return CivRel::CIV_REL_UNDISCOVERED;
    }
    return static_cast<CivRel>(m_row[a]->get(b, 0u));
}

void CivRelations::set (CivStaticDataKey c1, CivStaticDataKey c2, CivRel rel) {
    const u16 a = c1.value();
    const u16 b = c2.value();
    if (m_row == nullptr || a >= m_n || b >= m_n) {
        return;
    }
    const u32 v = static_cast<u32>(rel);
    if (m_row[a] != nullptr) {
        m_row[a]->set(b, 0u, v);
    }
    if (m_row[b] != nullptr) {
        m_row[b]->set(a, 0u, v);
    }
}

u16 CivRelations::civ_n () const {
    return m_n;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
