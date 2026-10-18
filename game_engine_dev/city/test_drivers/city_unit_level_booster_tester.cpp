//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_booster_test_shared.h"

#include "booster_apply.h"
#include "civ_trait_static_key.h"
#include "dyn_booster_register.h"
#include "effect_ctx.h"
#include "item_effects.h"
#include "item_reqs.h"
#include <cstring>

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    CityBoosterTestEnv env;
    if (!env.bind()) {
        return 1;
    }

    std::printf("=== city unit level booster tester ===\n");

    const u16 warrior = env.find_unit_idx("Warrior");
    const u16 grounds = env.find_bld_idx("Training Grounds");
    if (warrior == U16_KEY_NULL || grounds == U16_KEY_NULL) {
        std::printf("ERROR: catalog lookup failed (warrior=%u grounds=%u)\n", warrior, grounds);
        return 1;
    }

    const u8 def_lv = static_cast<u8>(env.m_st->config().get_default_unit_level());
    const u16 lvl_n = env.m_st->unit_level().get_item_count();
    const u8 hi = (lvl_n == 0u) ? 0u : static_cast<u8>(lvl_n - 1u);

    BoosterRegisterResult huge = {};
    huge.m_unit = 20;
    const u8 capped = apply_unit_level_boost(def_lv, huge, hi);
    city_booster_note(capped == hi, "apply_unit_level_boost caps at max level index");
    std::printf("  base=%u boost=+20 -> level=%u (expect hi=%u)\n",
        static_cast<unsigned>(def_lv), static_cast<unsigned>(capped), static_cast<unsigned>(hi));

    if (!env.finish_unit_build(warrior)) {
        std::printf("ERROR: warrior spawn failed\n");
        return 1;
    }
    const UnitAddStruct* plain = env.find_spawned_unit(warrior, 5, 5);
    const bool plain_ok = plain != nullptr && plain->m_level == def_lv;
    city_booster_note(plain_ok, "Warrior without Training Grounds stays at DEFAULT_UNIT_LEVEL");
    if (plain != nullptr) {
        std::printf("  spawned Warrior level=%u (expect default=%u)\n",
            static_cast<unsigned>(plain->m_level), static_cast<unsigned>(def_lv));
    }

    env.m_array.set_building_flag(env.m_city_idx, grounds);
    if (!env.finish_unit_build(warrior)) {
        std::printf("ERROR: training-grounds warrior spawn failed\n");
        return 1;
    }
    const UnitAddStruct* boosted = env.find_spawned_unit(warrior, 5, 5);
    const u8 expect_boost = static_cast<u8>(def_lv + 1u);
    const bool boost_ok = boosted != nullptr && boosted->m_level == expect_boost;
    city_booster_note(boost_ok, "Warrior with Training Grounds gets +1 level index");
    if (boosted != nullptr) {
        std::printf("  spawned Warrior level=%u (expect %u)\n",
            static_cast<unsigned>(boosted->m_level), static_cast<unsigned>(expect_boost));
    }

    const u16 mil = env.m_st->civ_trait().get_item_count();
    u16 mil_idx = U16_KEY_NULL;
    for (u16 i = 0; i < mil; ++i) {
        cstr nm = env.m_st->civ_trait().get_name(CivTraitStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, "Militaristic") == 0) {
            mil_idx = i;
            break;
        }
    }
    bool mil_ok = mil_idx != U16_KEY_NULL;
    if (mil_ok) {
        EffectCtx ctx = {};
        ctx.m_owner = 0;
        ctx.m_city_idx = env.m_city_idx;
        ctx.m_tech = env.m_techs;
        ctx.m_bld_bank = env.m_array.get_bld_bank();
        u16 traits[MAX_CIV_TRAIT_COUNT];
        for (u16 i = 0; i < MAX_CIV_TRAIT_COUNT; ++i) {
            traits[i] = U16_KEY_NULL;
        }
        traits[0] = mil_idx;
        ctx.m_trait_idx = traits;
        ctx.m_trait_n = MAX_CIV_TRAIT_COUNT;
        const DynBoosterRegister& reg = env.m_st->dyn_booster();
        u16 lv = def_lv;
        lv = apply_booster_u16(lv, reg.determine(ItemEffectBoosterType::UNIT_LEVEL_DEFAULT, ItemEffectsScope::CIV, ctx));
        u16 tmax = 0;
        tmax = apply_booster_u16(tmax, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::CIV, ctx));
        mil_ok = (lv == static_cast<u16>(def_lv + 1u)) && (tmax >= 1u);
        city_booster_note(mil_ok, "Militaristic CIV trait boosts UNIT_LEVEL_DEFAULT and UNIT_TRAINING_MAX");
        std::printf("  militaristic level=%u train_max=%u\n",
            static_cast<unsigned>(lv), static_cast<unsigned>(tmax));
    } else {
        city_booster_note(false, "Militaristic trait catalog lookup");
    }

    std::printf("done\n");
    return (plain_ok && boost_ok && mil_ok && capped == hi) ? 0 : 1;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
