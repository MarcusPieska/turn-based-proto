//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TILE_ENTRY_RESOLVER_H
#define TILE_ENTRY_RESOLVER_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - TileEntryRes -
//================================================================================================================================
//
//  Result of TileEntryResolver::resolve. Callers only need BlockRemains vs cleared.
//
//================================================================================================================================

enum class TileEntryRes : u8 {
    BlockRemains = 0, // Tile still blocked; caller may path around
    Cleared = 1 // Block removed (capture for now)
};

//================================================================================================================================
//=> - TileEntryResolver -
//================================================================================================================================
//
//  Resolves foreign units blocking a step (CanStepFail::Entry). Prefer capture of capturable
//  stacks; attack and expel are stubs. bind_state wires the active GameState.
//
//================================================================================================================================

class TileEntryResolver {
public:
    static void bind_state (GameState* state);
    static TileEntryRes resolve (u16 unit_idx, u16 x, u16 y);

private:
    TileEntryResolver () = delete;

    static bool stack_all_capturable (u16 x, u16 y, u16 foe_seat);
    static void stack_flip_owner (u16 x, u16 y, u16 self_seat);
    static TileEntryRes try_capture (u16 self_seat, u16 unit_idx, u16 x, u16 y, u16 foe_seat);
    static TileEntryRes try_attack (u16 self_seat, u16 unit_idx, u16 x, u16 y, u16 foe_seat);
    static TileEntryRes try_expel (u16 self_seat, u16 unit_idx, u16 x, u16 y, u16 foe_seat);

    static GameState* m_st; // Bound match state; null when idle
};

#endif // TILE_ENTRY_RESOLVER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
