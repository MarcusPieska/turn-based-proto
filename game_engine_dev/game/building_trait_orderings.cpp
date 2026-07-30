//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "building_trait_orderings.h"

#include <cstdlib>
#include <cstring>

#include "bit_array.h"
#include "building_static_data.h"
#include "building_trait_attribution.h"
#include "civ_trait_affinity.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

u16* BuildingTraitOrderings::m_orders = nullptr;
u16 BuildingTraitOrderings::m_n = 0;

//================================================================================================================================
//=> - BuildingTraitOrderings -
//================================================================================================================================

bool BuildingTraitOrderings::begin (const BuildingStaticData& blds) {
    clear();
    if (!BuildingTraitAttribution::ready()) {
        if (!BuildingTraitAttribution::begin(blds)) {
            return false;
        }
    }
    const u16 n = BuildingTraitAttribution::building_n();
    if (n == 0 || n != blds.get_item_count()) {
        return false;
    }
    const u32 bytes = static_cast<u32>(CivTraitAffinity::k_n) * static_cast<u32>(n) * sizeof(u16);
    u16* orders = static_cast<u16*>(std::malloc(bytes));
    if (orders == nullptr) {
        return false;
    }
    u8* claimed = static_cast<u8*>(std::malloc(static_cast<size_t>(n)));
    if (claimed == nullptr) {
        std::free(orders);
        return false;
    }

    m_orders = orders;
    m_n = n;

    for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
        const CivTrait owner = static_cast<CivTrait>(ti);
        std::memset(claimed, 0, static_cast<size_t>(n));
        u16* row = orders + static_cast<u32>(ti) * static_cast<u32>(n);
        u16 w = 0;
        for (u16 rank = 0; rank < CivTraitAffinity::k_n; ++rank) {
            const CivTrait tag = CivTraitAffinity::pref(owner, rank);
            for (u16 b = 0; b < n; ++b) {
                if (claimed[b] != 0) {
                    continue;
                }
                if (!BuildingTraitAttribution::has(b, tag)) {
                    continue;
                }
                row[w] = b;
                w = static_cast<u16>(w + 1u);
                claimed[b] = 1;
            }
        }
        for (u16 b = 0; b < n; ++b) {
            if (claimed[b] != 0) {
                continue;
            }
            row[w] = b;
            w = static_cast<u16>(w + 1u);
            claimed[b] = 1;
        }
    }

    std::free(claimed);
    return true;
}

void BuildingTraitOrderings::clear () {
    if (m_orders != nullptr) {
        std::free(m_orders);
        m_orders = nullptr;
    }
    m_n = 0;
}

bool BuildingTraitOrderings::ready () {
    return m_orders != nullptr && m_n > 0;
}

u16 BuildingTraitOrderings::building_n () {
    return m_n;
}

u16 BuildingTraitOrderings::at (u16 trait_idx, u16 slot) {
    if (m_orders == nullptr || trait_idx >= CivTraitAffinity::k_n || slot >= m_n) {
        return U16_KEY_NULL;
    }
    return m_orders[static_cast<u32>(trait_idx) * static_cast<u32>(m_n) + static_cast<u32>(slot)];
}

u16 BuildingTraitOrderings::pick (const BitArrayCL& available, u16 trait_idx) {
    if (m_orders == nullptr || trait_idx >= CivTraitAffinity::k_n) {
        return U16_KEY_NULL;
    }
    const u16* row = m_orders + static_cast<u32>(trait_idx) * static_cast<u32>(m_n);
    const u32 avail_n = available.get_count();
    for (u16 s = 0; s < m_n; ++s) {
        const u16 b = row[s];
        if (static_cast<u32>(b) >= avail_n) {
            continue;
        }
        if (available.get_bit(b) != 0) {
            return b;
        }
    }
    return U16_KEY_NULL;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
