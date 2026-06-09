//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RUNTIME_TRACE_DBG_H
#define RUNTIME_TRACE_DBG_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Toggles -
//================================================================================================================================

#define RUNTIME_TRACE_DBG
#define ENABLE_FLUSH_AFTER_PRINT

#define ENABLED_TRACE_SETUP
#define ENABLED_TRACE_CITY_FOUNDATION
#define ENABLED_TRACE_NEW_TURN

//================================================================================================================================
//=> - Macro-to-function mappings -
//================================================================================================================================

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_SETUP)
    void trace_setup(cstr label);
#define TRACE_SETUP(args) trace_setup args
#else
    #define TRACE_SETUP(args) ((void)0)
#endif

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_CITY_FOUNDATION)
    void trace_city_foundation(u16 x, u16 y, u16 player);
#define TRACE_CITY_FOUNDATION(args) trace_city_foundation args
#else
    #define TRACE_CITY_FOUNDATION(args) ((void)0)
#endif

#if defined(RUNTIME_TRACE_DBG) && defined(ENABLED_TRACE_NEW_TURN)
    void trace_new_turn(u16 turn);
#define TRACE_NEW_TURN(args) trace_new_turn args
#else
    #define TRACE_NEW_TURN(args) ((void)0)
#endif

#endif // RUNTIME_TRACE_DBG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

