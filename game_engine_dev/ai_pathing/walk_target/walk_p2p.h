//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WALK_P2P_H
#define WALK_P2P_H

#include "game_primitives.h"
#include "whiteboard_mng.h"

class GameState;

//================================================================================================================================
//=> - WalkP2P -
//================================================================================================================================
//
//  Owns turn and rem-MP whiteboards filled by GenerateDistanceP2P. Steps toward lower turn, breaking
//  ties with higher rem-MP. Boards return to WhiteboardMng on destruction.
//
//================================================================================================================================

class WalkP2P {
public:
    struct StepRes {
        u16 nx;
        u16 ny;
        u16 cost;
        bool have;
    };

    WalkP2P ();
    ~WalkP2P ();

    bool ok () const;
    u16 w () const;
    u16 h () const;
    u16* turn ();
    const u16* turn () const;
    u16* rem ();
    const u16* rem () const;
    StepRes peek (const GameState& s, u16 x, u16 y) const;

private:
    WalkP2P (const WalkP2P& other) = delete;
    WalkP2P (WalkP2P&& other) = delete;
    WalkP2P& operator= (const WalkP2P& other) = delete;
    WalkP2P& operator= (WalkP2P&& other) = delete;

    static u32 tidx (u16 w, u16 x, u16 y);
    static i32 rem_dec (u16 v);
    static bool reach (const u16* turn, u16 w, u16 x, u16 y);
    static bool closer (u16 t, i32 r, u16 nt, i32 nr);
    static bool find_lo (
        u16 t,
        i32 r,
        const u16* turn,
        const u16* rem,
        u16 w,
        u16 h,
        u16 x,
        u16 y,
        u16& ox,
        u16& oy);

    Whiteboard_2B m_turn;
    Whiteboard_2B m_rem;
};

#endif // WALK_P2P_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
