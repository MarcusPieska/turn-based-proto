//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_job_trait_attribution.h"

#include <cstdlib>
#include <cstring>

#include "city_job_static_data.h"
#include "city_job_static_key.h"
#include "item_effect_helpers.h"
#include "item_effects.h"
#include "trait_affinity_map.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

u8* CityJobTraitAttribution::m_masks = nullptr;
u16* CityJobTraitAttribution::m_bases = nullptr;
u16 CityJobTraitAttribution::m_n = 0;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u8 trait_bit (CivTrait trait) {
    return static_cast<u8>(1u << static_cast<u16>(trait));
}

static void apply_token (const TraitAffinityMap& aff, cstr token, u16 weight, u8* mask, u16* base_sum) {
    if (!token || weight == 0 || !mask || !base_sum) {
        return;
    }
    const u16 idx = aff.name_to_idx(token);
    if (idx == U16_KEY_NULL) {
        return;
    }
    const TraitAffinityRowStruct& row = aff.get_row(idx);
    u16 add = static_cast<u16>(row.m_base * weight);
    if (add == 0) {
        add = weight;
    }
    *base_sum = static_cast<u16>(*base_sum + add);
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
            apply_token(aff, ItemEffectHelper::booster_type_enum_to_str(slot.effect.booster.target_id), 1u, mask, base_sum);
            continue;
        }
        if (tp == ItemEffectType::PRODUCE) {
            const ItemEffectProduce& pr = slot.effect.produce;
            if (pr.kind == ItemProduceKind::YIELD) {
                apply_token(
                    aff,
                    ItemEffectHelper::produce_yield_enum_to_str(static_cast<ItemProduceYield>(pr.target_id)),
                    1u,
                    mask,
                    base_sum);
            }
        }
    }
}

static void apply_yields (const TraitAffinityMap& aff, const CityJobStaticDataStruct& job, u8* mask, u16* base_sum) {
    if (job.food > 0) {
        apply_token(aff, "FOOD", static_cast<u16>(job.food), mask, base_sum);
    }
    if (job.production > 0) {
        apply_token(aff, "PRODUCTION", job.production, mask, base_sum);
    }
    if (job.commerce > 0) {
        apply_token(aff, "COMMERCE", job.commerce, mask, base_sum);
    }
    if (job.culture > 0) {
        apply_token(aff, "CULTURE", job.culture, mask, base_sum);
    }
    if (job.science > 0) {
        apply_token(aff, "SCIENCE", job.science, mask, base_sum);
    }
    if (job.religion > 0) {
        apply_token(aff, "RELIGION", job.religion, mask, base_sum);
    }
}

//================================================================================================================================
//=> - CityJobTraitAttribution -
//================================================================================================================================

bool CityJobTraitAttribution::begin (const CityJobStaticData& jobs, const TraitAffinityMap& aff) {
    clear();
    const u16 n = jobs.get_item_count();
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
        const CityJobStaticDataStruct& item = jobs.get_item(CityJobStaticDataKey::from_raw(i));
        apply_yields(aff, item, &masks[i], &bases[i]);
        apply_effects(aff, item.effects, &masks[i], &bases[i]);
    }
    m_masks = masks;
    m_bases = bases;
    m_n = n;
    return true;
}

void CityJobTraitAttribution::clear () {
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

bool CityJobTraitAttribution::ready () {
    return m_masks != nullptr && m_bases != nullptr && m_n > 0;
}

u16 CityJobTraitAttribution::job_n () {
    return m_n;
}

u8 CityJobTraitAttribution::mask (u16 job_idx) {
    if (m_masks == nullptr || job_idx >= m_n) {
        return 0;
    }
    return m_masks[job_idx];
}

u16 CityJobTraitAttribution::base (u16 job_idx) {
    if (m_bases == nullptr || job_idx >= m_n) {
        return 0;
    }
    return m_bases[job_idx];
}

bool CityJobTraitAttribution::has (u16 job_idx, CivTrait trait) {
    return (mask(job_idx) & trait_bit(trait)) != 0;
}

bool CityJobTraitAttribution::add_tag (u16 job_idx, CivTrait trait) {
    if (m_masks == nullptr || job_idx >= m_n) {
        return false;
    }
    m_masks[job_idx] = static_cast<u8>(m_masks[job_idx] | trait_bit(trait));
    return true;
}

u16 CityJobTraitAttribution::count_for (CivTrait trait) {
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
