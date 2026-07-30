//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "building_trait_attribution.h"

#include <cstdlib>
#include <cstring>

#include "building_static_data.h"
#include "building_static_key.h"
#include "item_effects.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

u8* BuildingTraitAttribution::m_masks = nullptr;
u16 BuildingTraitAttribution::m_n = 0;

//================================================================================================================================
//=> - Mapping tables -
//================================================================================================================================

static const u16 k_map_tag_max = 4;

struct TraitTagList {
    CivTrait m_tags[k_map_tag_max]; // Trait tags contributed by one source
    u8 m_n; // How many tags are set
};

static u8 trait_bit (CivTrait trait) {
    return static_cast<u8>(1u << static_cast<u16>(trait));
}

static u8 mask_from_tags (const TraitTagList& tags) {
    u8 mask = 0;
    for (u8 i = 0; i < tags.m_n && i < k_map_tag_max; ++i) {
        mask = static_cast<u8>(mask | trait_bit(tags.m_tags[i]));
    }
    return mask;
}

static TraitTagList tags_from_booster (ItemEffectBoosterType tp) {
    TraitTagList out = {};
    switch (tp) {
    case ItemEffectBoosterType::SCIENCE:
        out.m_tags[0] = CivTrait::Scientific;
        out.m_n = 1;
        break;
    case ItemEffectBoosterType::HAPPINESS:
        out.m_tags[0] = CivTrait::Religious;
        out.m_n = 1;
        break;
    case ItemEffectBoosterType::CULTURE:
        out.m_tags[0] = CivTrait::Religious;
        out.m_tags[1] = CivTrait::Scientific;
        out.m_n = 2;
        break;
    case ItemEffectBoosterType::PRODUCTION:
    case ItemEffectBoosterType::POLLUTION:
        out.m_tags[0] = CivTrait::Industrious;
        out.m_n = 1;
        break;
    case ItemEffectBoosterType::AIR_DEFENSE:
    case ItemEffectBoosterType::DEFENSE:
    case ItemEffectBoosterType::NUKE_DEFENSE:
    case ItemEffectBoosterType::SHIP_DEFENSE:
    case ItemEffectBoosterType::SHIP_TRAINING:
    case ItemEffectBoosterType::UNIT_EXP:
    case ItemEffectBoosterType::UPGRADE_COST:
    case ItemEffectBoosterType::WAR_WEAR:
        out.m_tags[0] = CivTrait::Militaristic;
        out.m_n = 1;
        break;
    case ItemEffectBoosterType::COMMERCE:
        out.m_tags[0] = CivTrait::Commercial;
        out.m_n = 1;
        break;
    case ItemEffectBoosterType::SEA_TRADE:
        out.m_tags[0] = CivTrait::Expansionist;
        out.m_tags[1] = CivTrait::Commercial;
        out.m_n = 2;
        break;
    case ItemEffectBoosterType::CORRUPTION:
    case ItemEffectBoosterType::ESPIONAGE:
    case ItemEffectBoosterType::MOVEMENT:
    case ItemEffectBoosterType::SHIP_MOVEMENT:
    case ItemEffectBoosterType::AIR_RANGE:
        out.m_tags[0] = CivTrait::Expansionist;
        out.m_n = 1;
        break;
    case ItemEffectBoosterType::POP_GROWTH:
    case ItemEffectBoosterType::SANITATION:
        out.m_tags[0] = CivTrait::Agricultural;
        out.m_n = 1;
        break;
    case ItemEffectBoosterType::NONE:
    default:
        break;
    }
    return out;
}

static TraitTagList tags_from_produce (ItemProduceYield yld) {
    TraitTagList out = {};
    switch (yld) {
    case ItemProduceYield::FOOD:
        out.m_tags[0] = CivTrait::Agricultural;
        out.m_n = 1;
        break;
    case ItemProduceYield::COMMERCE:
        out.m_tags[0] = CivTrait::Commercial;
        out.m_n = 1;
        break;
    case ItemProduceYield::PRODUCTION:
        out.m_tags[0] = CivTrait::Industrious;
        out.m_n = 1;
        break;
    case ItemProduceYield::SCIENCE:
        out.m_tags[0] = CivTrait::Scientific;
        out.m_n = 1;
        break;
    case ItemProduceYield::NONE:
    default:
        break;
    }
    return out;
}

static u8 tags_from_effects (const ItemEffectsStruct& fx) {
    u8 mask = 0;
    for (u16 i = 0; i < MAX_EFFECT_COUNT; ++i) {
        const ItemEffectStruct& slot = fx.items[i];
        const ItemEffectType tp = static_cast<ItemEffectType>(slot.type);
        if (tp == ItemEffectType::NONE) {
            continue;
        }
        if (tp == ItemEffectType::BOOSTER) {
            mask = static_cast<u8>(mask | mask_from_tags(tags_from_booster(slot.effect.booster.target_id)));
            continue;
        }
        if (tp == ItemEffectType::PRODUCE) {
            const ItemEffectProduce& pr = slot.effect.produce;
            if (pr.kind == ItemProduceKind::YIELD) {
                mask = static_cast<u8>(mask | mask_from_tags(tags_from_produce(static_cast<ItemProduceYield>(pr.target_id))));
            }
        }
    }
    return mask;
}

//================================================================================================================================
//=> - BuildingTraitAttribution -
//================================================================================================================================

bool BuildingTraitAttribution::begin (const BuildingStaticData& blds) {
    clear();
    const u16 n = blds.get_item_count();
    if (n == 0) {
        return false;
    }
    u8* masks = static_cast<u8*>(std::malloc(static_cast<size_t>(n)));
    if (masks == nullptr) {
        return false;
    }
    std::memset(masks, 0, static_cast<size_t>(n));
    for (u16 i = 0; i < n; ++i) {
        const BuildingStaticDataStruct& item = blds.get_item(BuildingStaticDataKey::from_raw(i));
        masks[i] = tags_from_effects(item.effects);
    }
    m_masks = masks;
    m_n = n;
    return true;
}

void BuildingTraitAttribution::clear () {
    if (m_masks != nullptr) {
        std::free(m_masks);
        m_masks = nullptr;
    }
    m_n = 0;
}

bool BuildingTraitAttribution::ready () {
    return m_masks != nullptr && m_n > 0;
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
