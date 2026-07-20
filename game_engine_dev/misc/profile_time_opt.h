//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PROFILE_TIME_OPT_H
#define PROFILE_TIME_OPT_H

#include "game_primitives.h"

//================================================================================================================================
//=> - PtoId -
//================================================================================================================================
//
//  Fixed profile site ids for ProfileTimeOpt. Extend this enum when adding a new profiled site.
//
//================================================================================================================================

enum class PtoId : u16 {
    PTO_CITY_LOOP = 0,
    PTO_UNIT_LOOP = 1,
    PTO_COUNT = 2
};

//================================================================================================================================
//=> - ProfileTimeOpt -
//================================================================================================================================
//
//  Optional O(1) profiler keyed by PtoId. Enable PTO_ENABLE in game_primitives.h. PTO_START/STOP pair on an id;
//  PTO_INIT clears registers; PTO_PRINT dumps totals. Nested starts on the same id are illegal.
//
//================================================================================================================================

class ProfileTimeOpt {
public:
    static void init ();
    static void start (PtoId id);
    static void stop (PtoId id);
    static void print ();

private:
    ProfileTimeOpt () = delete;
};

#ifdef PTO_ENABLE
#define PTO_INIT() ProfileTimeOpt::init()
#define PTO_START(id) ProfileTimeOpt::start(id)
#define PTO_STOP(id) ProfileTimeOpt::stop(id)
#define PTO_PRINT() ProfileTimeOpt::print()
#else
#define PTO_INIT() ((void)0)
#define PTO_START(id) ((void)0)
#define PTO_STOP(id) ((void)0)
#define PTO_PRINT() ((void)0)
#endif

#endif // PROFILE_TIME_OPT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
