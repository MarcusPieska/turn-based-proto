//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EVAL_BIN_H
#define EVAL_BIN_H

#include "game_primitives.h"

class EvalPaths;
class GameArraySimple;
class UnitAddVector;
class CityArray;
class PlayerState;

//================================================================================================================================
//=> - EvalBin -
//================================================================================================================================
//
//  Full GameIo unpack of turn save quartets into hot-path structs. One snap per requested turn.
//
//================================================================================================================================

class EvalBin {
public:
    EvalBin ();
    ~EvalBin ();

    void clr ();
    bool file_ok (cstr path) const;
    bool load_turn (const EvalPaths& paths, u32 turn);

    u16 snap_n () const;
    u32 turn_at (u16 i) const;
    u16 snap_i (u32 turn) const;

    const GameArraySimple* map (u16 i) const;
    const UnitAddVector* units (u16 i) const;
    const CityArray* cities (u16 i) const;
    const PlayerState* seats (u16 i) const;
    u16 seat_n (u16 i) const;

private:
    EvalBin (const EvalBin&) = delete;
    EvalBin& operator= (const EvalBin&) = delete;

    struct Snap;
    Snap** m_snaps;
    u16 m_n;
    u16 m_cap;
};

#endif // EVAL_BIN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
