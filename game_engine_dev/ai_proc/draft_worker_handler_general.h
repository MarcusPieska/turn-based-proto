//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DRAFT_WORKER_HANDLER_GENERAL_H
#define DRAFT_WORKER_HANDLER_GENERAL_H

#include "game_primitives.h"

class GameState; 

//================================================================================================================================
//=> - DraftWorkerHandlerGeneral -
//================================================================================================================================
//
//  City-leashed draft-worker AI strategy (general). Implementation body is cpp-included from
//  impl/draft_worker_impl_general_mkNN.cpp via DRAFT_WORKER_GENERAL_IMPL.
//  WorkerHelper::get_data holds the home city index; workers must stay in that city's work disk.
//
//================================================================================================================================

class DraftWorkerHandlerGeneral {
public:
    DraftWorkerHandlerGeneral () = delete;

    static bool begin (GameState& state);
    static void clear ();
    static void handle (GameState& state, u16 unit_idx);
};

#endif // DRAFT_WORKER_HANDLER_GENERAL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
