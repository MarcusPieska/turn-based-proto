//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ASSERT_LOG_H
#define ASSERT_LOG_H

#include "game_primitives.h"

//================================================================================================================================
//=> - AssertLog -
//================================================================================================================================
//
//  Optional expect logger/exiter; enable GAME_ASSERT_ENABLE in game_primitives.h.
//  When disabled, GAME_EXPECT* macros compile out for zero hot-path cost.
//  When enabled, a failed expect logs to stderr and exits the process.
//
//================================================================================================================================

class AssertLog {
public:
    static void log (cstr msg);

private:
    AssertLog () = delete;
};

#if defined(GAME_ASSERT_ENABLE)
#define GAME_EXPECT(cond, msg) \
    do { if (!(cond)) { AssertLog::log(msg); } } while (0)
#define GAME_EXPECT_RET(cond, ret, msg) \
    do { if (!(cond)) { AssertLog::log(msg); return (ret); } } while (0)
#else
#define GAME_EXPECT(cond, msg) ((void)0)
#define GAME_EXPECT_RET(cond, ret, msg) ((void)0)
#endif

#endif // ASSERT_LOG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
