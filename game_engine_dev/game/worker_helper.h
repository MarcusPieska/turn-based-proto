//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WORKER_HELPER_H
#define WORKER_HELPER_H

#include "game_primitives.h"

struct UnitAddStruct;

//================================================================================================================================
//=> - WorkerHelper -
//================================================================================================================================
//
//  Reinterprets UnitAddStruct combat fields for workers: m_health is a work-turn countdown;
//  m_misc|m_level pack a u16 payload (m_misc high byte). Callers must only use on workers.
//
//================================================================================================================================

class WorkerHelper {
public:
    static void set_finish_time (UnitAddStruct* u, u16 turns_needed);
    static bool is_done (const UnitAddStruct* u);
    static void countdown (UnitAddStruct* u);

    static u16 get_data (const UnitAddStruct* u);
    static void set_data (UnitAddStruct* u, u16 v);

private:
    WorkerHelper () = delete;
};

#endif // WORKER_HELPER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
