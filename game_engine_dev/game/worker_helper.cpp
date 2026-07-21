//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "worker_helper.h"

#include "assert_log.h"
#include "unit_add_struct.h"

//================================================================================================================================
//=> - WorkerHelper -
//================================================================================================================================

void WorkerHelper::set_finish_time (UnitAddStruct* u, u16 turns_needed) {
    GAME_EXPECT(u != nullptr, "WorkerHelper::set_finish_time null unit");
    GAME_EXPECT(turns_needed <= 255u, "WorkerHelper::set_finish_time turns exceed u8");
    u->m_health = static_cast<u8>(turns_needed);
}

bool WorkerHelper::is_done (const UnitAddStruct* u) {
    GAME_EXPECT(u != nullptr, "WorkerHelper::is_done null unit");
    return u->m_health == 0u;
}

void WorkerHelper::countdown (UnitAddStruct* u) {
    GAME_EXPECT(u != nullptr, "WorkerHelper::countdown null unit");
    if (u->m_health > 0u) {
        u->m_health = static_cast<u8>(u->m_health - 1u);
    }
}

u16 WorkerHelper::get_data (const UnitAddStruct* u) {
    GAME_EXPECT(u != nullptr, "WorkerHelper::get_data null unit");
    return static_cast<u16>((static_cast<u16>(u->m_misc) << 8) | static_cast<u16>(u->m_level));
}

void WorkerHelper::set_data (UnitAddStruct* u, u16 v) {
    GAME_EXPECT(u != nullptr, "WorkerHelper::set_data null unit");
    u->m_misc = static_cast<u8>((v >> 8) & 0xFFu);
    u->m_level = static_cast<u8>(v & 0xFFu);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
