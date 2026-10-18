//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>

#include "booster_apply.h"
#include "city_booster_test_shared.h"
#include "dyn_booster_register.h"
#include "effect_ctx.h"
#include "item_effects.h"

typedef std::chrono::high_resolution_clock Clock;

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const u16 k_unit_n = 5u;
static const u16 k_grp_n = 3u;
static const u16 k_feed = 1u;

static f64 ns_between (Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<f64, std::nano>(b - a).count();
}

static void print_training (UnitAddStruct* const* us, u16 n) {
    std::printf("Training: [");
    for (u16 i = 0; i < n; ++i) {
        if (i > 0u) {
            std::printf(",");
        }
        if (us[i] == nullptr) {
            std::printf("?");
        } else {
            std::printf("%u", static_cast<unsigned>(us[i]->m_level));
        }
    }
    std::printf("]");
}

static void reset_levels (UnitAddStruct** us, u16 n) {
    for (u16 i = 0; i < n; ++i) {
        if (us[i] != nullptr) {
            us[i]->m_level = 0;
        }
    }
}

static bool all_same_level (UnitAddStruct* const* us, u16 n, u8* out_lv) {
    if (n == 0u || us[0] == nullptr) {
        return false;
    }
    const u8 lv = us[0]->m_level;
    for (u16 i = 1; i < n; ++i) {
        if (us[i] == nullptr || us[i]->m_level != lv) {
            return false;
        }
    }
    if (out_lv != nullptr) {
        *out_lv = lv;
    }
    return true;
}

static bool levels_match_feed (UnitAddStruct* const* us, u16 n, u16 amount, u8 hi) {
    u16 rem = amount;
    for (u16 i = 0; i < n; ++i) {
        if (us[i] == nullptr) {
            return false;
        }
        u8 want = 0u;
        if (rem > 0u) {
            want = (rem >= static_cast<u16>(hi)) ? hi : static_cast<u8>(rem);
            rem = static_cast<u16>(rem - want);
        }
        if (us[i]->m_level != want) {
            return false;
        }
    }
    return true;
}

static u16 resolve_train_max (CityBoosterTestEnv& env) {
    EffectCtx ctx = {};
    ctx.m_owner = 0;
    ctx.m_city_idx = env.m_city_idx;
    ctx.m_tech = env.m_techs;
    ctx.m_bld_bank = env.m_array.get_bld_bank();
    ctx.m_wonder_city = env.m_wonder_city;
    ctx.m_wonder_n = env.m_st->wonder().get_item_count();
    ctx.m_small_wonder_city = env.m_seat.m_small_wonder_city;
    ctx.m_small_wonder_n = env.m_st->small_wonder().get_item_count();
    ctx.m_emp_bld_fn = nullptr;
    ctx.m_emp_bld_ud = nullptr;

    const DynBoosterRegister& reg = env.m_st->dyn_booster();
    u16 max_lv = 0u;
    max_lv = apply_booster_u16(max_lv, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::LOCAL, ctx));
    max_lv = apply_booster_u16(max_lv, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::CITY, ctx));
    max_lv = apply_booster_u16(max_lv, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::CIV, ctx));
    max_lv = apply_booster_u16(max_lv, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::GLOBAL, ctx));
    const u16 lvl_n = env.m_st->unit_level().get_item_count();
    const u16 cat_hi = (lvl_n == 0u) ? 0u : static_cast<u16>(lvl_n - 1u);
    if (max_lv > cat_hi) {
        max_lv = cat_hi;
    }
    return max_lv;
}

static u16 collect_tile_units (CityBoosterTestEnv& env, u16 x, u16 y, UnitAddStruct** us, UnitAddKey* keys, u16 cap) {
    u16 n = 0u;
    u16 hd = env.m_state.m_map.get_unit_hd(x, y);
    while (hd != U16_KEY_NULL && n < cap) {
        UnitAddStruct* u = env.m_state.m_units.get_unit_add(UnitAddKey::from_raw(hd));
        if (u == nullptr) {
            break;
        }
        const u16 nxt_tile = static_cast<u16>(u->m_next_unit_on_tile);
        u16 g = hd;
        while (g != U16_KEY_NULL && n < cap) {
            UnitAddStruct* gu = env.m_state.m_units.get_unit_add(UnitAddKey::from_raw(g));
            if (gu == nullptr) {
                break;
            }
            const u16 nxt_g = static_cast<u16>(gu->m_next_unit_in_group);
            us[n] = gu;
            if (keys != nullptr) {
                keys[n] = UnitAddKey::from_raw(g);
            }
            n = static_cast<u16>(n + 1u);
            g = nxt_g;
        }
        hd = nxt_tile;
    }
    return n;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    CityBoosterTestEnv env;
    if (!env.bind()) {
        return 1;
    }

    std::printf("=== city unit training tester ===\n");

    const u16 warrior = env.find_unit_idx("Warrior");
    if (warrior == U16_KEY_NULL) {
        std::printf("ERROR: Warrior not found\n");
        return 1;
    }

    City* c = env.city();
    if (c == nullptr) {
        return 1;
    }

    for (u16 i = 0; i < k_unit_n; ++i) {
        if (!env.finish_unit_build(warrior)) {
            std::printf("ERROR: spawn %u failed\n", static_cast<unsigned>(i));
            return 1;
        }
    }

    UnitAddStruct* us[k_unit_n];
    UnitAddKey keys[k_unit_n];
    u16 got = collect_tile_units(env, 5, 5, us, keys, k_unit_n);
    if (got != k_unit_n) {
        std::printf("ERROR: tile stack size %u expect %u\n", static_cast<unsigned>(got), static_cast<unsigned>(k_unit_n));
        return 1;
    }

    UnitAddKey grp_hd = UnitAddKey::from_raw(U16_KEY_NULL);
    if (!UnitMovementMng::form_group_chain(env.m_state, keys, k_grp_n, &grp_hd)) {
        std::printf("ERROR: form_group_chain failed\n");
        return 1;
    }
    got = collect_tile_units(env, 5, 5, us, keys, k_unit_n);
    if (got != k_unit_n) {
        std::printf("ERROR: table walk size %u expect %u\n", static_cast<unsigned>(got), static_cast<unsigned>(k_unit_n));
        return 1;
    }

    const u16 tech_n = env.m_st->tech().get_item_count();
    for (u16 i = 0; i < tech_n; ++i) {
        env.m_techs->set_bit(i);
    }
    const u16 bld_n = env.m_st->building().get_item_count();
    for (u16 i = 0; i < bld_n; ++i) {
        env.m_array.set_building_flag(env.m_city_idx, i);
    }

    const u16 train_max = resolve_train_max(env);
    const u8 hi = static_cast<u8>(train_max);
    std::printf("effector UNIT_TRAINING_MAX=%u\n", static_cast<unsigned>(train_max));

    std::printf("--- feed=1 until max ---\n");
    print_training(us, k_unit_n);
    std::printf("\n");

    f64 tmin = 1e300;
    f64 tmax = 0.0;
    f64 tsum = 0.0;
    u32 steps = 0u;
    const u32 step_cap = 512u;
    while (steps < step_cap) {
        const Clock::time_point t0 = Clock::now();
        const u16 left = c->add_training(env.m_city_idx, k_feed);
        const Clock::time_point t1 = Clock::now();
        const f64 ns = ns_between(t0, t1);
        if (ns < tmin) {
            tmin = ns;
        }
        if (ns > tmax) {
            tmax = ns;
        }
        tsum += ns;
        print_training(us, k_unit_n);
        std::printf("  %.2f ns\n", ns);
        steps += 1u;
        if (left == k_feed) {
            break;
        }
    }

    u8 final_lv = 0u;
    const bool leveled = all_same_level(us, k_unit_n, &final_lv);
    const bool ok1 = leveled && final_lv > 0u && final_lv == hi;
    city_booster_note(ok1, "all city units trained to shared UNIT_TRAINING_MAX");
    std::printf("final_level=%u steps=%u\n", static_cast<unsigned>(final_lv), static_cast<unsigned>(steps));
    if (steps > 0u) {
        std::printf("add_training ns/call: min %.2f  max %.2f  avg %.2f\n",
            tmin, tmax, tsum / static_cast<f64>(steps));
    }

    std::printf("--- feed=1..N with reset (N=%u) ---\n", static_cast<unsigned>(steps));
    reset_levels(us, k_unit_n);
    print_training(us, k_unit_n);
    std::printf("\n");

    f64 tmin2 = 1e300;
    f64 tmax2 = 0.0;
    f64 tsum2 = 0.0;
    bool ok2 = steps > 0u;
    for (u32 i = 1u; i <= steps; ++i) {
        const u16 feed = static_cast<u16>(i);
        const Clock::time_point t0 = Clock::now();
        const u16 left = c->add_training(env.m_city_idx, feed);
        const Clock::time_point t1 = Clock::now();
        const f64 ns = ns_between(t0, t1);
        if (ns < tmin2) {
            tmin2 = ns;
        }
        if (ns > tmax2) {
            tmax2 = ns;
        }
        tsum2 += ns;
        print_training(us, k_unit_n);
        std::printf("  feed=%u left=%u  %.2f ns\n", static_cast<unsigned>(feed), static_cast<unsigned>(left), ns);
        if (!levels_match_feed(us, k_unit_n, feed, hi)) {
            ok2 = false;
        }
        reset_levels(us, k_unit_n);
    }
    city_booster_note(ok2, "feed 1..N fill-first matches after each reset");
    if (steps > 0u) {
        std::printf("add_training ns/call: min %.2f  max %.2f  avg %.2f\n",
            tmin2, tmax2, tsum2 / static_cast<f64>(steps));
    }
    std::printf("done\n");
    return (ok1 && ok2) ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
