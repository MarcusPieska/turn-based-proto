//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "unit_utility_helper.h"

#include "bit_array.h"
#include "runtime_statics.h"
#include "unit_action_enum.h"
#include "unit_type_action_map.h"

//================================================================================================================================
//= Statics =
//================================================================================================================================

BitArrayCL* UnitUtilityHelper::m_bits = nullptr;
u16 UnitUtilityHelper::m_type_n = 0;

//================================================================================================================================
//= UnitUtilityHelper =
//================================================================================================================================

void UnitUtilityHelper::clear () {
    delete m_bits;
    m_bits = nullptr;
    m_type_n = 0;
}

bool UnitUtilityHelper::setup (const RuntimeStatics& st) {
    clear();
    const u16 type_n = st.unit_type().get_item_count();
    if (type_n == 0u) {
        return false;
    }
    // Single BitArrayCL is enough for ROW_N == 1; switch storage once a second utility row is added.
    m_bits = new BitArrayCL(static_cast<u32>(type_n) * static_cast<u32>(ROW_N));
    m_type_n = type_n;
    const UnitTypeActionMap& am = st.unit_type_action_map();
    const u16 act_embark = static_cast<u16>(UnitAction::canEmbark);
    const u16 act_atk = static_cast<u16>(UnitAction::canAttack);
    const u16 act_br = static_cast<u16>(UnitAction::canBarrage);
    const u16 act_aa = static_cast<u16>(UnitAction::canDoAntiAir);
    const u32 row_base = static_cast<u32>(ROW_IS_LAND_ARMY_UNIT) * static_cast<u32>(type_n);
    for (u16 t = 0; t < type_n; ++t) {
        if (!am.unit_type_can_do(t, act_embark)) {
            continue;
        }
        if (!am.unit_type_can_do(t, act_atk) && !am.unit_type_can_do(t, act_br) && !am.unit_type_can_do(t, act_aa)) {
            continue;
        }
        m_bits->set_bit(row_base + static_cast<u32>(t));
    }
    return true;
}

bool UnitUtilityHelper::is_type_for_land_army (u16 unit_type) {
    if (m_bits == nullptr || unit_type >= m_type_n) {
        return false;
    }
    const u32 bit = static_cast<u32>(ROW_IS_LAND_ARMY_UNIT) * static_cast<u32>(m_type_n) + static_cast<u32>(unit_type);
    return m_bits->get_bit(bit) != 0u;
}

//================================================================================================================================
//= End of file =
//================================================================================================================================
