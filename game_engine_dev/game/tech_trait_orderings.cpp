//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "tech_trait_orderings.h"

#include <cstdlib>
#include <cstring>

#include "assert_log.h"
#include "bit_array.h"
#include "building_static_data.h"
#include "civ_trait_affinity.h"
#include "tech_static_data.h"
#include "tech_trait_attribution.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

u16* TechTraitOrderings::m_orders = nullptr;
u16 TechTraitOrderings::m_n = 0;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

struct OrdRow {
    u16 m_idx;
    i32 m_score;
};

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
//=> - TechTraitOrderings -
//================================================================================================================================

bool TechTraitOrderings::begin (const TechStaticData& techs, const BuildingStaticData& blds) {
    clear();
    if (!TechTraitAttribution::ready()) {
        if (!TechTraitAttribution::begin(techs, blds)) {
            return false;
        }
    }
    const u16 n = TechTraitAttribution::tech_n();
    if (n == 0 || n != techs.get_item_count()) {
        return false;
    }
    const u32 bytes = static_cast<u32>(CivTraitAffinity::k_n) * static_cast<u32>(n) * sizeof(u16);
    u16* orders = static_cast<u16*>(std::malloc(bytes));
    if (orders == nullptr) {
        return false;
    }
    i32* scores = static_cast<i32*>(std::malloc(sizeof(i32) * static_cast<size_t>(n)));
    OrdRow* rows = static_cast<OrdRow*>(std::malloc(sizeof(OrdRow) * static_cast<size_t>(n)));
    if (scores == nullptr || rows == nullptr) {
        std::free(orders);
        std::free(scores);
        std::free(rows);
        return false;
    }

    m_orders = orders;
    m_n = n;
    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        const CivTrait owner = static_cast<CivTrait>(ti);
        TechTraitAttribution::score_tree(owner, scores);
        for (u16 i = 0; i < n; ++i) {
            rows[i].m_idx = i;
            rows[i].m_score = scores[i];
        }
        std::qsort(rows, n, sizeof(OrdRow), ord_cmp);
        u16* row = orders + static_cast<u32>(ti) * static_cast<u32>(n);
        for (u16 i = 0; i < n; ++i) {
            row[i] = rows[i].m_idx;
        }
    }

    std::free(scores);
    std::free(rows);
    return true;
}

void TechTraitOrderings::clear () {
    if (m_orders != nullptr) {
        std::free(m_orders);
        m_orders = nullptr;
    }
    m_n = 0;
}

bool TechTraitOrderings::ready () {
    return m_orders != nullptr && m_n > 0;
}

u16 TechTraitOrderings::tech_n () {
    return m_n;
}

u16 TechTraitOrderings::at (u16 trait_idx, u16 slot) {
    GAME_EXPECT(m_orders != nullptr, "TechTraitOrderings at not ready");
    GAME_EXPECT(trait_idx < CivTraitAffinity::k_n, "TechTraitOrderings at trait");
    GAME_EXPECT(slot < m_n, "TechTraitOrderings at slot");
    return m_orders[static_cast<u32>(trait_idx) * static_cast<u32>(m_n) + static_cast<u32>(slot)];
}

u16 TechTraitOrderings::pick (const BitArrayCL& available, u16 trait_idx) {
    GAME_EXPECT(m_orders != nullptr, "TechTraitOrderings pick not ready");
    GAME_EXPECT(trait_idx < CivTraitAffinity::k_n, "TechTraitOrderings pick trait");
    GAME_EXPECT(available.get_count() == m_n, "TechTraitOrderings pick avail size");
    const u16* row = m_orders + static_cast<u32>(trait_idx) * static_cast<u32>(m_n);
    for (u16 s = 0; s < m_n; ++s) {
        const u16 t = row[s];
        if (available.get_bit(t) != 0) {
            return t;
        }
    }
    return U16_KEY_NULL;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
