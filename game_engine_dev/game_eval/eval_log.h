//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EVAL_LOG_H
#define EVAL_LOG_H

#include "game_primitives.h"
#include "log_dbg/log_need_mask.h"

//================================================================================================================================
//=> - EvalLog -
//================================================================================================================================
//
//  Thin wrapper over log_dbg SO PARSE helpers. Loads a trace file and counts/scans typed lines.
//
//================================================================================================================================

class EvalLog {
public:
    EvalLog ();
    ~EvalLog ();

    void clr ();
    bool load (cstr path);
    bool ok () const;
    u32 line_n () const;
    cstr line (u32 i) const;

    u32 count_i (u16 kind_i) const;
    bool parse_tech_discover (u32 i, u16* player, u16* tech) const;
    bool parse_city_foundation (u32 i, u16* x, u16* y, u16* player) const;
    bool parse_new_turn (u32 i, u16* turn) const;

private:
    EvalLog (const EvalLog&) = delete;
    EvalLog& operator= (const EvalLog&) = delete;

    char** m_lines;
    u32 m_n;
    u32 m_cap;
};

#endif // EVAL_LOG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
