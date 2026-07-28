//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "combat_mods.h"
#include "combat_mod.h"
#include "runtime_statics.h"
#include "unit_role_static_data.h"
#include "unit_role_static_key.h"

//================================================================================================================================
//=> - CombatMods -
//================================================================================================================================

CombatMods::CombatMods () :
    m_vs(nullptr),
    m_city(nullptr),
    m_n(0) {
}

CombatMods::~CombatMods () {
    clear();
}

void CombatMods::clear () {
    delete[] m_vs;
    delete[] m_city;
    m_vs = nullptr;
    m_city = nullptr;
    m_n = 0;
}

bool CombatMods::setup (const RuntimeStatics& st) {
    clear();
    const UnitRoleStaticData& roles = st.unit_role();
    const u16 n = roles.get_item_count();
    if (n == 0) {
        return false;
    }
    const u32 cells = static_cast<u32>(n) * static_cast<u32>(n);
    m_vs = new i16[cells];
    m_city = new i16[n];
    std::memset(m_vs, 0, sizeof(i16) * cells);
    std::memset(m_city, 0, sizeof(i16) * n);
    m_n = n;
    for (u16 i = 0; i < n; ++i) {
        const UnitRoleStaticDataStruct& row = roles.get_item(UnitRoleStaticDataKey::from_raw(i));
        for (u8 m = 0; m < row.mods.m_n; ++m) {
            const CombatMod& cm = row.mods.m_mods[m];
            if (cm.m_role == COMBAT_MOD_CITY_DEFENSE) {
                m_city[i] = cm.m_pct;
                continue;
            }
            if (cm.m_role >= n) {
                continue;
            }
            m_vs[static_cast<u32>(i) * n + cm.m_role] = cm.m_pct;
        }
    }
    return true;
}

CombatBoost CombatMods::get (u16 atk, u16 def, bool city) const {
    CombatBoost out = {};
    if (m_vs == nullptr || atk >= m_n || def >= m_n) {
        return out;
    }
    out.m_atk = m_vs[static_cast<u32>(atk) * m_n + def];
    out.m_def = m_vs[static_cast<u32>(def) * m_n + atk];
    if (city && m_city != nullptr) {
        out.m_def = static_cast<i16>(out.m_def + m_city[def]);
    }
    return out;
}

u16 CombatMods::role_n () const {
    return m_n;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
