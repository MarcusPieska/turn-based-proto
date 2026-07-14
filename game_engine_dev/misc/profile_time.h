//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PROFILE_TIME_H
#define PROFILE_TIME_H

#include "game_primitives.h"

//================================================================================================================================
//=> - ProfileTime -
//================================================================================================================================
//
//  Optional function profiler; enable PTIME_ENABLE in game_primitives.h. PTIME_START/STOP pair on a cstr key (typically __func__).
//  PTIME_TRACE_ENABLE there adds >> / << trace lines on start/stop (handled in profile_time.cpp).
//  Each name keeps last-entry time, call count, and accumulated total. Nested calls across different names are fine;
//  a second START on the same name before STOP is treated as illegal recursion and exits.
//
//================================================================================================================================

class ProfileTime {
public:
    static void init ();
    static void start (cstr name);
    static void stop (cstr name);
    static void print ();

private:
    ProfileTime () = delete;
};

#if defined(PTIME_ENABLE) || defined(PTIME_TRACE_ENABLE)
#define PTIME_START(name) ProfileTime::start(name)
#define PTIME_STOP(name) ProfileTime::stop(name)
#else
#define PTIME_START(name) ((void)0)
#define PTIME_STOP(name) ((void)0)
#endif

#ifdef PTIME_ENABLE
#define PTIME_INIT() ProfileTime::init()
#define PTIME_PRINT() ProfileTime::print()
#else
#define PTIME_INIT() ((void)0)
#define PTIME_PRINT() ((void)0)
#endif

#endif // PROFILE_TIME_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
