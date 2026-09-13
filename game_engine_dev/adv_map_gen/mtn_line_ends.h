//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MTN_LINE_ENDS_H
#define MTN_LINE_ENDS_H

#include "game_primitives.h"
#include "p1_map_size.h"
#include "p1_wb_util.h"
#include "wb_que_xy.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - MtnLineEnds -
//================================================================================================================================
//
//  Streams farthest-endpoint pairs (A then B via two 8-adj BFS passes) one connected line component at a time.
//  next() returns false and writes the nil sentinel when no components remain. No heap: whiteboards + stack only.
//
//================================================================================================================================

//================================================================================================================================
//=> - MtnLineEndPair -
//================================================================================================================================

static const u16 MTN_LINE_END_NIL = 0xffffu;

struct MtnLineEndPair {
    u16 m_sx; // First-flood seed x
    u16 m_sy; // First-flood seed y
    u16 m_ax; // Farthest from seed (A)
    u16 m_ay;
    u16 m_bx; // Farthest from A (B)
    u16 m_by;
};

static inline MtnLineEndPair mtn_line_end_pair_nil () {
    MtnLineEndPair p;
    p.m_sx = MTN_LINE_END_NIL;
    p.m_sy = MTN_LINE_END_NIL;
    p.m_ax = MTN_LINE_END_NIL;
    p.m_ay = MTN_LINE_END_NIL;
    p.m_bx = MTN_LINE_END_NIL;
    p.m_by = MTN_LINE_END_NIL;
    return p;
}

static inline bool mtn_line_end_pair_is_nil (const MtnLineEndPair& p) {
    return p.m_ax == MTN_LINE_END_NIL || p.m_bx == MTN_LINE_END_NIL;
}

//================================================================================================================================
//=> - MtnLineEnds -
//================================================================================================================================

class MtnLineEnds {
public:
    explicit MtnLineEnds (u32 seed);
    ~MtnLineEnds ();

    bool begin (const u16* line_ov, u16 w, u16 h);
    bool next (MtnLineEndPair* out);
    bool ok () const;
    u32 line_n () const;
    u32 comp_n () const;

private:
    MtnLineEnds (const MtnLineEnds& other) = delete;
    MtnLineEnds (MtnLineEnds&& other) = delete;

    bool flood_pass (u16 sx, u16 sy, const u8* in_comp, u8* mark, u16* step, WB_QueXY* visit, u16* far_x, u16* far_y);
    bool flood_comp (u16 sx, u16 sy, MtnLineEndPair* out);

    u32 m_seed;
    bool m_ok;
    bool m_open;
    const u16* m_ov; // Active line overlay
    u16 m_w;
    u16 m_h;
    u32 m_scan; // Global scan index
    u32 m_line_n; // Tiles flooded so far
    u32 m_comp_n; // Pairs produced so far

    Whiteboard_1B m_done; // Component membership / re-flood guard
    Whiteboard_1B m_mk2;  // Pass-2 visit marks
    Whiteboard_2B m_s1;   // Pass-1 steps
    Whiteboard_2B m_s2;   // Pass-2 steps
    WB_QueXY m_bfs;
    WB_QueXY m_visit;
};

#endif // MTN_LINE_ENDS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
