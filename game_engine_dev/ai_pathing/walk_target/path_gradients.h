//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PATH_GRADIENTS_H
#define PATH_GRADIENTS_H

#include "game_primitives.h"
#include "walk_p2p.h"

class GameState;

//================================================================================================================================
//=> - PathGradients -
//================================================================================================================================
//
//  Fixed bank of WalkP2P path gradients (one slot per army). WalkP2P API with a leading slot index.
//  Slots are lazy pointers: at(i) constructs on first use so unused armies do not hold whiteboards.
//
//================================================================================================================================

class PathGradients {
public:
    static const u16 k_cap = 20u;
    static const u16 k_opt_n = WalkP2P::k_opt_n;

    typedef WalkP2P::StepRes StepRes;

    PathGradients ();
    ~PathGradients ();

    WalkP2P& at (u16 i);
    const WalkP2P& at (u16 i) const;

    bool ok (u16 i) const;
    u16 w (u16 i) const;
    u16 h (u16 i) const;
    u16* turn (u16 i);
    const u16* turn (u16 i) const;
    u16* rem (u16 i);
    const u16* rem (u16 i) const;
    StepRes peek (u16 i, const GameState& s, u16 x, u16 y) const;
    void peek_opts (u16 i, const GameState& s, u16 x, u16 y, u16* ox, u16* oy) const;

private:
    PathGradients (const PathGradients& o) = delete;
    PathGradients (PathGradients&& o) = delete;
    PathGradients& operator= (const PathGradients& o) = delete;
    PathGradients& operator= (PathGradients&& o) = delete;

    bool ens (u16 i);
    WalkP2P* m_g[k_cap];
};

#endif // PATH_GRADIENTS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
