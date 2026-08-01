//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "tech_trait_attribution.h"

#include <cstdlib>
#include <cstring>

#include "assert_log.h"
#include "building_static_data.h"
#include "building_static_key.h"
#include "building_trait_attribution.h"
#include "civ_trait_affinity.h"
#include "item_reqs.h"
#include "tech_static_key.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

const TechStaticDataStruct* TechTraitAttribution::m_items = nullptr;
const BuildingStaticDataStruct* TechTraitAttribution::m_blds = nullptr;
u8* TechTraitAttribution::m_masks = nullptr;
u16 TechTraitAttribution::m_n = 0;
u16 TechTraitAttribution::m_bld_n = 0;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u8 trait_bit (CivTrait trait) {
    return static_cast<u8>(1u << static_cast<u16>(trait));
}

static void walk_up (
    const TechStaticDataStruct* items,
    u16 n,
    u16 tech_idx,
    i32 score,
    i32* scores,
    u16 depth)
{
    GAME_EXPECT(items != nullptr, "TechTraitAttribution walk_up items");
    GAME_EXPECT(scores != nullptr, "TechTraitAttribution walk_up scores");
    GAME_EXPECT(tech_idx < n, "TechTraitAttribution walk_up tech_idx");
    if (score <= 0 || depth > n) {
        return;
    }
    scores[tech_idx] = scores[tech_idx] + score;
    const ItemReqsStruct& reqs = items[tech_idx].reqs;
    const i32 next = score + TECH_TRAIT_SCORE_STEP;
    for (u8 i = 0; i < MAX_PREREQ_COUNT; ++i) {
        if (reqs.types[i] != ITEM_REQ_TYPE_TECH) {
            continue;
        }
        walk_up(items, n, reqs.indices[i], next, scores, static_cast<u16>(depth + 1u));
    }
}

static bool bld_needs_tech (const BuildingStaticDataStruct& bld, u16 tech_idx) {
    for (u8 i = 0; i < MAX_PREREQ_COUNT; ++i) {
        if (bld.reqs.types[i] == ITEM_REQ_TYPE_TECH && bld.reqs.indices[i] == tech_idx) {
            return true;
        }
    }
    return false;
}

//================================================================================================================================
//=> - TechTraitAttribution -
//================================================================================================================================

bool TechTraitAttribution::begin (const TechStaticData& techs, const BuildingStaticData& blds) {
    clear();
    if (!BuildingTraitAttribution::ready()) {
        if (!BuildingTraitAttribution::begin(blds)) {
            return false;
        }
    }
    const u16 tn = techs.get_item_count();
    const u16 bn = blds.get_item_count();
    if (tn == 0 || bn == 0 || bn != BuildingTraitAttribution::building_n()) {
        return false;
    }
    u8* masks = static_cast<u8*>(std::malloc(static_cast<size_t>(tn)));
    if (masks == nullptr) {
        return false;
    }
    std::memset(masks, 0, static_cast<size_t>(tn));
    const BuildingStaticDataStruct* brow = &blds.get_item(BuildingStaticDataKey::from_raw(0));
    for (u16 b = 0; b < bn; ++b) {
        const u8 bm = BuildingTraitAttribution::mask(b);
        if (bm == 0) {
            continue;
        }
        for (u8 i = 0; i < MAX_PREREQ_COUNT; ++i) {
            if (brow[b].reqs.types[i] != ITEM_REQ_TYPE_TECH) {
                continue;
            }
            const u16 t = brow[b].reqs.indices[i];
            GAME_EXPECT(t < tn, "TechTraitAttribution begin tech req");
            masks[t] = static_cast<u8>(masks[t] | bm);
        }
    }
    m_items = &techs.get_item(TechStaticDataKey::from_raw(0));
    m_blds = brow;
    m_masks = masks;
    m_n = tn;
    m_bld_n = bn;
    return true;
}

void TechTraitAttribution::clear () {
    if (m_masks != nullptr) {
        std::free(m_masks);
        m_masks = nullptr;
    }
    m_items = nullptr;
    m_blds = nullptr;
    m_n = 0;
    m_bld_n = 0;
}

bool TechTraitAttribution::ready () {
    return m_items != nullptr && m_masks != nullptr && m_n > 0;
}

u16 TechTraitAttribution::tech_n () {
    return m_n;
}

u8 TechTraitAttribution::mask (u16 tech_idx) {
    GAME_EXPECT(m_masks != nullptr, "TechTraitAttribution mask not ready");
    GAME_EXPECT(tech_idx < m_n, "TechTraitAttribution mask tech_idx");
    return m_masks[tech_idx];
}

bool TechTraitAttribution::has (u16 tech_idx, CivTrait trait) {
    return (mask(tech_idx) & trait_bit(trait)) != 0;
}

u16 TechTraitAttribution::count_for (CivTrait trait) {
    u16 n = 0;
    for (u16 i = 0; i < m_n; ++i) {
        if (has(i, trait)) {
            n = static_cast<u16>(n + 1u);
        }
    }
    return n;
}

i32 TechTraitAttribution::band_score (CivTrait owner, CivTrait tag) {
    const i8 a = CivTraitAffinity::affinity(owner, tag);
    if (a >= 6) {
        return TECH_TRAIT_SCORE_PRIMARY;
    }
    if (a >= 4) {
        return TECH_TRAIT_SCORE_FRIENDLY;
    }
    if (a >= 2) {
        return TECH_TRAIT_SCORE_NEUTRAL;
    }
    return TECH_TRAIT_SCORE_HOSTILE;
}

i32 TechTraitAttribution::local_score (u16 tech_idx, CivTrait primary) {
    GAME_EXPECT(ready(), "TechTraitAttribution local_score not ready");
    GAME_EXPECT(m_blds != nullptr, "TechTraitAttribution local_score blds");
    GAME_EXPECT(tech_idx < m_n, "TechTraitAttribution local_score tech_idx");
    i32 sum = 0;
    for (u16 b = 0; b < m_bld_n; ++b) {
        if (!bld_needs_tech(m_blds[b], tech_idx)) {
            continue;
        }
        const u8 bm = BuildingTraitAttribution::mask(b);
        for (u16 ti = 0; ti < CivTraitAffinity::k_n; ++ti) {
            const CivTrait tag = static_cast<CivTrait>(ti);
            if ((bm & trait_bit(tag)) == 0) {
                continue;
            }
            sum = sum + band_score(primary, tag);
        }
    }
    return sum;
}

void TechTraitAttribution::propagate (u16 tech_idx, i32 score, i32* scores) {
    GAME_EXPECT(ready(), "TechTraitAttribution propagate not ready");
    GAME_EXPECT(scores != nullptr, "TechTraitAttribution propagate scores");
    walk_up(m_items, m_n, tech_idx, score, scores, 0);
}

void TechTraitAttribution::score_tree (CivTrait primary, i32* scores) {
    GAME_EXPECT(ready(), "TechTraitAttribution score_tree not ready");
    GAME_EXPECT(scores != nullptr, "TechTraitAttribution score_tree scores");
    std::memset(scores, 0, sizeof(i32) * static_cast<size_t>(m_n));
    for (u16 t = 0; t < m_n; ++t) {
        const i32 loc = local_score(t, primary);
        if (loc <= 0) {
            continue;
        }
        propagate(t, loc, scores);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
