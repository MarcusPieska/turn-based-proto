//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "civic_trait_orderings.h"

#include <cstdlib>
#include <cstring>

#include "bit_array.h"
#include "civic_static_data.h"
#include "civic_trait_attribution.h"
#include "civ_trait_affinity.h"
#include "trait_affinity_map.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

u16* CivicTraitOrderings::m_orders = nullptr;
u16 CivicTraitOrderings::m_n = 0;

//================================================================================================================================
//=> - Score helpers -
//================================================================================================================================

struct OrdRow {
    u16 m_idx;
    i32 m_score;
};

static i32 civic_score (CivTrait owner, u16 c) {
    i32 s = static_cast<i32>(CivicTraitAttribution::base(c));
    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        const CivTrait tag = static_cast<CivTrait>(ti);
        if (!CivicTraitAttribution::has(c, tag)) {
            continue;
        }
        s = s + static_cast<i32>(CivTraitAffinity::affinity(owner, tag));
    }
    return s;
}

static int ord_cmp (const void* a, const void* b) {
    const OrdRow* ra = static_cast<const OrdRow*>(a);
    const OrdRow* rb = static_cast<const OrdRow*>(b);
    if (ra->m_score > rb->m_score) {
        return -1;
    }
    if (ra->m_score < rb->m_score) {
        return 1;
    }
    if (ra->m_idx < rb->m_idx) {
        return -1;
    }
    if (ra->m_idx > rb->m_idx) {
        return 1;
    }
    return 0;
}

//================================================================================================================================
//=> - CivicTraitOrderings -
//================================================================================================================================

bool CivicTraitOrderings::begin (const CivicStaticData& civics, const TraitAffinityMap& aff) {
    clear();
    if (!CivicTraitAttribution::ready()) {
        if (!CivicTraitAttribution::begin(civics, aff)) {
            return false;
        }
    }
    const u16 n = CivicTraitAttribution::civic_n();
    if (n == 0 || n != civics.get_item_count()) {
        return false;
    }
    const u32 bytes = static_cast<u32>(CivTraitAffinity::k_n) * static_cast<u32>(n) * sizeof(u16);
    u16* orders = static_cast<u16*>(std::malloc(bytes));
    OrdRow* rows = static_cast<OrdRow*>(std::malloc(static_cast<size_t>(n) * sizeof(OrdRow)));
    if (orders == nullptr || rows == nullptr) {
        std::free(orders);
        std::free(rows);
        return false;
    }

    m_orders = orders;
    m_n = n;

    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        const CivTrait owner = static_cast<CivTrait>(ti);
        for (u16 c = 0; c < n; ++c) {
            rows[c].m_idx = c;
            rows[c].m_score = civic_score(owner, c);
        }
        std::qsort(rows, static_cast<size_t>(n), sizeof(OrdRow), ord_cmp);
        u16* row = orders + static_cast<u32>(ti) * static_cast<u32>(n);
        for (u16 i = 0; i < n; ++i) {
            row[i] = rows[i].m_idx;
        }
    }

    std::free(rows);
    return true;
}

void CivicTraitOrderings::clear () {
    if (m_orders != nullptr) {
        std::free(m_orders);
        m_orders = nullptr;
    }
    m_n = 0;
}

bool CivicTraitOrderings::ready () {
    return m_orders != nullptr && m_n > 0;
}

u16 CivicTraitOrderings::civic_n () {
    return m_n;
}

u16 CivicTraitOrderings::at (u16 trait_idx, u16 slot) {
    if (m_orders == nullptr || trait_idx >= CivTraitAffinity::k_n || slot >= m_n) {
        return U16_KEY_NULL;
    }
    return m_orders[static_cast<u32>(trait_idx) * static_cast<u32>(m_n) + static_cast<u32>(slot)];
}

u16 CivicTraitOrderings::pick (const BitArrayCL& available, u16 trait_idx, u16 start_slot) {
    if (m_orders == nullptr || trait_idx >= CivTraitAffinity::k_n) {
        return U16_KEY_NULL;
    }
    const u16* row = m_orders + static_cast<u32>(trait_idx) * static_cast<u32>(m_n);
    const u32 avail_n = available.get_count();
    for (u16 s = start_slot; s < m_n; ++s) {
        const u16 c = row[s];
        if (static_cast<u32>(c) >= avail_n) {
            continue;
        }
        if (available.get_bit(c) != 0) {
            return c;
        }
    }
    return U16_KEY_NULL;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
