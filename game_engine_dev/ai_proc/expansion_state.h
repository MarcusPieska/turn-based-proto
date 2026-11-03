//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EXPANSION_STATE_H
#define EXPANSION_STATE_H

#include "bit_array.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - ExpansionState -
//================================================================================================================================
//
//  Per-seat durable state for iterative sector selectors: visited bitset, full-size backtrack
//  path (u16 per sector for now), and walk cursors. Selectors operate through this interface so
//  a truncated path can be swapped in later without changing call sites.
//
//================================================================================================================================

class ExpansionState {
public:
    ExpansionState ();
    ~ExpansionState ();

    bool begin (u16 sec_n);
    void clr ();
    void reset_walk ();

    bool ok () const;
    u16 sec_n () const;

    bool visited (u16 s) const;
    void mark (u16 s);

    bool path_push (u16 s);
    bool path_pop ();
    u16 path_n () const;
    u16 path_at (u16 i) const;
    u16 path_top () const;

    u16 start () const;
    void set_start (u16 s);

    u16 cur () const;
    void set_cur (u16 s);

    u16 ox () const;
    u16 oy () const;
    void set_origin (u16 x, u16 y);

    u32& rs ();
    bool first () const;
    void set_first (bool v);
    bool have () const;
    void set_have (bool v);

private:
    ExpansionState (const ExpansionState& o) = delete;
    ExpansionState& operator= (const ExpansionState& o) = delete;

    BitArrayCL* m_seen; // Visited sectors
    u16* m_path; // Backtrack stack
    u16 m_sec_n; // Sector count / path capacity
    u16 m_path_n; // Valid path length
    u16 m_start; // Origin sector
    u16 m_cur; // Current sector
    u16 m_ox; // Origin seed x
    u16 m_oy; // Origin seed y
    u32 m_rs; // RNG state
    bool m_first; // Next neighbor pick is random
    bool m_have; // Start not yet emitted
    bool m_ok;
};

#endif // EXPANSION_STATE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
