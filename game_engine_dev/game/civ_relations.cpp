//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "civ_relations.h"
#include "map_bit_array_overlay.h"

//================================================================================================================================
//=> - CivRelations -
//================================================================================================================================

static const u8 k_rel_bpv = 3;

CivRelations::CivRelations () :
    m_grid(nullptr) {
}

CivRelations::~CivRelations () {
    reset(0);
}

void CivRelations::reset (u16 civ_n) {
    delete m_grid;
    m_grid = nullptr;
    if (civ_n == 0) {
        return;
    }
    m_grid = new MapBitArrayOverlay(civ_n, civ_n, k_rel_bpv);
    m_grid->fill_all(static_cast<u32>(CivRel::CIV_REL_UNDISCOVERED));
}

CivRel CivRelations::get (CivStaticDataKey c1, CivStaticDataKey c2) const {
    return static_cast<CivRel>(m_grid->get(c1.value(), c2.value()));
}

void CivRelations::set (CivStaticDataKey c1, CivStaticDataKey c2, CivRel rel) {
    m_grid->set(c1.value(), c2.value(), static_cast<u32>(rel));
}

u16 CivRelations::civ_n () const {
    if (m_grid == nullptr) {
        return 0;
    }
    return m_grid->width();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
