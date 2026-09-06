//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SIEGE_SNAPSHOT_H
#define SIEGE_SNAPSHOT_H

#include "game_primitives.h"
#include "unit_add_vector_key.h"

class GameState;

//================================================================================================================================
//=> - SiegeSnapshot -
//================================================================================================================================
//
//  Logical pre-assault fight state for war-driver save / CityAttackManager replay.
//  Serializes army group, city buildings + defense deduction, and defending stack.
//
//================================================================================================================================

class SiegeSnapshot {
public:
    static bool ensure_dir (const char* dir);

    static bool save (
        const GameState& s,
        UnitAddKey army_hd,
        u16 city_x,
        u16 city_y,
        u16 def_seat,
        const char* path);

    static bool apply (
        GameState& s,
        const char* path,
        UnitAddKey* out_army,
        u16* out_cx,
        u16* out_cy,
        u16* out_def);

private:
    SiegeSnapshot () = delete;
};

#endif // SIEGE_SNAPSHOT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
