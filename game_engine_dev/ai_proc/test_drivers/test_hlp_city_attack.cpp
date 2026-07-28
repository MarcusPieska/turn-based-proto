//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "test_hlp_city_attack.h"
#include "conduct_campaign.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_snap_cap = 2048u;
static const u16 k_typ_cap = 256u;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

struct SnapUnit {
    u16 m_key;
    u16 m_typ;
    u8 m_hp;
};

static u16 walk_grp (GameState& s, UnitAddKey head, SnapUnit* out, u16 cap) {
    u16 n = 0;
    UnitAddKey cur = head;
    while (cur.is_valid() && n < cap) {
        const UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        out[n].m_key = cur.value();
        out[n].m_typ = u->m_unit_typ_idx;
        out[n].m_hp = u->m_health;
        n++;
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    return n;
}

static void count_by_typ (const SnapUnit* snap, u16 n, u16* ct, u16 typ_n) {
    for (u16 t = 0; t < typ_n && t < k_typ_cap; ++t) {
        ct[t] = 0;
    }
    for (u16 i = 0; i < n; ++i) {
        if (snap[i].m_typ < typ_n && snap[i].m_typ < k_typ_cap) {
            ct[snap[i].m_typ]++;
        }
    }
}

static void print_comp (cstr label, const GameState& s, const u16* ct, u16 typ_n, u16 total) {
    std::printf("%s total=%u\n", label, (u32)total);
    if (s.m_statics == nullptr) {
        return;
    }
    for (u16 t = 0; t < typ_n && t < k_typ_cap; ++t) {
        if (ct[t] == 0u) {
            continue;
        }
        const char* nm = s.m_statics->unit().get_name(UnitStaticDataKey::from_raw(t));
        if (nm == nullptr) {
            nm = "?";
        }
        std::printf("  %s: %u\n", nm, (u32)ct[t]);
    }
}

static bool in_snap (const SnapUnit* snap, u16 n, u16 key) {
    for (u16 i = 0; i < n; ++i) {
        if (snap[i].m_key == key) {
            return true;
        }
    }
    return false;
}

static const SnapUnit* find_snap (const SnapUnit* snap, u16 n, u16 key) {
    for (u16 i = 0; i < n; ++i) {
        if (snap[i].m_key == key) {
            return &snap[i];
        }
    }
    return nullptr;
}

//================================================================================================================================
//=> - TestHlpCityAttack -
//================================================================================================================================

bool TestHlpCityAttack::run (GameState& s, ConductCampaign& camp, u16 city_x, u16 city_y, bool print_log) {
    if (s.m_statics == nullptr) {
        return false;
    }
    const UnitAddKey army = UnitAddKey::from_raw(camp.atk_hd(0));
    if (!army.is_valid()) {
        if (print_log) {
            std::printf("city_attack: no army head\n");
        }
        return false;
    }
    const u16 typ_n = s.m_statics->unit().get_item_count();
    SnapUnit before[k_snap_cap];
    u16 bn = 0;
    if (print_log) {
        bn = walk_grp(s, army, before, k_snap_cap);
        u16 bct[k_typ_cap];
        count_by_typ(before, bn, bct, typ_n);
        std::printf("army before assault:\n");
        print_comp("  ", s, bct, typ_n, bn);
    }

    const auto t0 = std::chrono::steady_clock::now();
    const bool ok = camp.assault_city(city_x, city_y, 0u);
    const auto t1 = std::chrono::steady_clock::now();
    const double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    if (!ok) {
        if (print_log) {
            std::printf("*** FAILED ConductCampaign::assault_city (%.2f us)\n", us);
        }
        return false;
    }
    const UnitAddKey stay = UnitAddKey::from_raw(camp.split_hd(0));
    const UnitAddKey occupy = UnitAddKey::from_raw(camp.atk_hd(0));
    if (!print_log) {
        return true;
    }
    std::printf("assault_us=%.2f stay=%u occupy=%u\n", us, (u32)stay.value(), (u32)occupy.value());

    SnapUnit after[k_snap_cap];
    u16 an = 0;
    if (stay.is_valid()) {
        an = walk_grp(s, stay, after, k_snap_cap);
    }
    if (occupy.is_valid() && an < k_snap_cap) {
        an = static_cast<u16>(an + walk_grp(s, occupy, after + an, static_cast<u16>(k_snap_cap - an)));
    }
    u16 act[k_typ_cap];
    count_by_typ(after, an, act, typ_n);
    std::printf("army after assault:\n");
    print_comp("  ", s, act, typ_n, an);

    u16 lost_n = 0;
    u16 lct[k_typ_cap];
    for (u16 t = 0; t < k_typ_cap; ++t) {
        lct[t] = 0;
    }
    for (u16 i = 0; i < bn; ++i) {
        if (in_snap(after, an, before[i].m_key)) {
            continue;
        }
        lost_n++;
        if (before[i].m_typ < k_typ_cap) {
            lct[before[i].m_typ]++;
        }
    }
    std::printf("army losses:\n");
    print_comp("  ", s, lct, typ_n, lost_n);

    std::printf("reduced health:\n");
    u16 dmg_n = 0;
    for (u16 i = 0; i < an; ++i) {
        const UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(after[i].m_key));
        if (u == nullptr || u->m_health == 0u || u->m_health >= UNIT_HEALTH) {
            continue;
        }
        const SnapUnit* prev = find_snap(before, bn, after[i].m_key);
        const u8 hp0 = (prev != nullptr) ? prev->m_hp : UNIT_HEALTH;
        const char* nm = s.m_statics->unit().get_name(UnitStaticDataKey::from_raw(u->m_unit_typ_idx));
        if (nm == nullptr) {
            nm = "?";
        }
        std::printf("  key=%u %s hp %u -> %u\n", (u32)after[i].m_key, nm, (u32)hp0, (u32)u->m_health);
        dmg_n++;
    }
    if (dmg_n == 0u) {
        std::printf("  (none)\n");
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
