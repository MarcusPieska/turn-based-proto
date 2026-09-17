//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TRACE_SINK_H
#define TRACE_SINK_H

#include "game_primitives.h"

//================================================================================================================================
//=> - TraceSink -
//================================================================================================================================
//
//  Thin shared file sink for runtime_trace_dbg and log_dbg. open binds a path; printf writes to the file.
//  If the sink is closed, printf opens the usual game_loop.trace path. Re-open of the same path is a no-op.
//
//================================================================================================================================

class TraceSink {
public:
    static bool open (cstr path);
    static bool ensure_open (cstr path);
    static void close ();
    static bool ok ();
    static void set_auto_flush (bool on);
    static void printf (cstr fmt, ...);
    static void flush ();

private:
    TraceSink () = delete;
};

#endif // TRACE_SINK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
