//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "combat_mods.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "unit_role_static_data.h"
#include "unit_role_static_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool load_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!load_statics()) {
        std::printf("*** FAILED load statics\n");
        return 1;
    }
    CombatMods mods;
    if (!mods.setup(*g_rt_statics)) {
        std::printf("*** FAILED CombatMods::setup\n");
        return 1;
    }
    const UnitRoleStaticData& roles = g_rt_statics->unit_role();
    const u16 n = mods.role_n();
    std::printf("roles=%u\n", (u32)n);
    std::printf("open-field non-zero:\n");
    for (u16 a = 0; a < n; ++a) {
        for (u16 d = 0; d < n; ++d) {
            const CombatBoost b = mods.get(a, d, false);
            if (b.m_atk == 0 && b.m_def == 0) {
                continue;
            }
            std::printf("  %s vs %s: atk=%d def=%d\n",
                roles.get_name(UnitRoleStaticDataKey::from_raw(a)),
                roles.get_name(UnitRoleStaticDataKey::from_raw(d)),
                (int)b.m_atk, (int)b.m_def);
        }
    }
    std::printf("city-defense non-zero:\n");
    for (u16 d = 0; d < n; ++d) {
        const CombatBoost open = mods.get(0, d, false);
        const CombatBoost city = mods.get(0, d, true);
        const i16 delta = static_cast<i16>(city.m_def - open.m_def);
        if (delta == 0) {
            continue;
        }
        std::printf("  defender %s: city=%d\n", roles.get_name(UnitRoleStaticDataKey::from_raw(d)), (int)delta);
    }
    std::printf("*** PASSED combat_mods\n");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
