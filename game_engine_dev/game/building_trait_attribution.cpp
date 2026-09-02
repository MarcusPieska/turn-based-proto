//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "building_trait_attribution.h"

#include <cstdlib>
#include <cstring>

#include "building_static_data.h"
#include "building_static_key.h"
#include "item_effect_helpers.h"
#include "item_effects.h"
#include "trait_affinity_map.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

u8* BuildingTraitAttribution::m_masks = nullptr;
u16* BuildingTraitAttribution::m_bases = nullptr;
u16 BuildingTraitAttribution::m_n = 0;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u8 trait_bit (CivTrait trait) {
    return static_cast<u8>(1u << static_cast<u16>(trait));
}

static void apply_token (const TraitAffinityMap& aff, cstr token, u8* mask, u16* base_sum) {
    if (!token || !mask || !base_sum) {
        return;
    }
    const u16 idx = aff.name_to_idx(token);
    if (idx == U16_KEY_NULL) {
        return;
    }
    const TraitAffinityRowStruct& row = aff.get_row(idx);
    *base_sum = static_cast<u16>(*base_sum + row.m_base);
    for (u8 i = 0; i < row.m_trait_n; ++i) {
        *mask = static_cast<u8>(*mask | trait_bit(static_cast<CivTrait>(row.m_traits[i])));
    }
}

static void apply_effects (const TraitAffinityMap& aff, const ItemEffectsStruct& fx, u8* mask, u16* base_sum) {
    for (u16 i = 0; i < MAX_EFFECT_COUNT; ++i) {
        const ItemEffectStruct& slot = fx.items[i];
        const ItemEffectType tp = static_cast<ItemEffectType>(slot.type);
        if (tp == ItemEffectType::NONE) {
            continue;
        }
        if (tp == ItemEffectType::BOOSTER) {
            apply_token(aff, ItemEffectHelper::booster_type_enum_to_str(slot.effect.booster.target_id), mask, base_sum);
            continue;
        }
        if (tp == ItemEffectType::PRODUCE) {
            const ItemEffectProduce& pr = slot.effect.produce;
            if (pr.kind == ItemProduceKind::YIELD) {
                apply_token(
                    aff,
                    ItemEffectHelper::produce_yield_enum_to_str(static_cast<ItemProduceYield>(pr.target_id)),
                    mask,
                    base_sum);
            }
        }
    }
}

//================================================================================================================================
//=> - BuildingTraitAttribution -
//================================================================================================================================

bool BuildingTraitAttribution::begin (const BuildingStaticData& blds, const TraitAffinityMap& aff) {
    clear();
    const u16 n = blds.get_item_count();
    if (n == 0 || aff.get_row_count() == 0) {
        return false;
    }
    u8* masks = static_cast<u8*>(std::malloc(static_cast<size_t>(n)));
    u16* bases = static_cast<u16*>(std::malloc(static_cast<size_t>(n) * sizeof(u16)));
    if (masks == nullptr || bases == nullptr) {
        std::free(masks);
        std::free(bases);
        return false;
    }
    std::memset(masks, 0, static_cast<size_t>(n));
    std::memset(bases, 0, static_cast<size_t>(n) * sizeof(u16));
    for (u16 i = 0; i < n; ++i) {
        const BuildingStaticDataStruct& item = blds.get_item(BuildingStaticDataKey::from_raw(i));
        apply_effects(aff, item.effects, &masks[i], &bases[i]);
    }
    m_masks = masks;
    m_bases = bases;
    m_n = n;
    return true;
}

void BuildingTraitAttribution::clear () {
    if (m_masks != nullptr) {
        std::free(m_masks);
        m_masks = nullptr;
    }
    if (m_bases != nullptr) {
        std::free(m_bases);
        m_bases = nullptr;
    }
    m_n = 0;
}

bool BuildingTraitAttribution::ready () {
    return m_masks != nullptr && m_bases != nullptr && m_n > 0;
}

u16 BuildingTraitAttribution::building_n () {
    return m_n;
}

u8 BuildingTraitAttribution::mask (u16 bld_idx) {
    if (m_masks == nullptr || bld_idx >= m_n) {
        return 0;
    }
    return m_masks[bld_idx];
}

u16 BuildingTraitAttribution::base (u16 bld_idx) {
    if (m_bases == nullptr || bld_idx >= m_n) {
        return 0;
    }
    return m_bases[bld_idx];
}

bool BuildingTraitAttribution::has (u16 bld_idx, CivTrait trait) {
    return (mask(bld_idx) & trait_bit(trait)) != 0;
}

bool BuildingTraitAttribution::add_tag (u16 bld_idx, CivTrait trait) {
    if (m_masks == nullptr || bld_idx >= m_n) {
        return false;
    }
    m_masks[bld_idx] = static_cast<u8>(m_masks[bld_idx] | trait_bit(trait));
    return true;
}

u16 BuildingTraitAttribution::count_for (CivTrait trait) {
    if (m_masks == nullptr) {
        return 0;
    }
    const u8 bit = trait_bit(trait);
    u16 n = 0;
    for (u16 i = 0; i < m_n; ++i) {
        if ((m_masks[i] & bit) != 0) {
            n = static_cast<u16>(n + 1u);
        }
    }
    return n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
