//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ARMY_REASSEMBLER_H
#define ARMY_REASSEMBLER_H

#include "game_primitives.h"
#include "walk_p2p.h"

class GameState;

//================================================================================================================================
//=> - ArmyReassembler -
//================================================================================================================================
//
//  Walks standing (in_army) group heads to a rally tile via a muster-style distance field.
//  begin builds the field; tick steps heads; at_goal reports whether all listed heads are on target.
//
//================================================================================================================================

class ArmyReassembler {
public:
    static const u16 k_hd_cap = 32u;

    ArmyReassembler ();
    ~ArmyReassembler ();

    bool begin (GameState& st, u16 seat, u16 tx, u16 ty);
    bool tick (GameState& st, u16 seat, const u16* heads, u16 n);
    bool at_goal (GameState& st, const u16* heads, u16 n) const;
    void clr ();

    bool ok () const;
    u16 tx () const;
    u16 ty () const;

private:
    ArmyReassembler (const ArmyReassembler& o) = delete;
    ArmyReassembler& operator= (const ArmyReassembler& o) = delete;

    WalkP2P m_mob;
    u16 m_tx;
    u16 m_ty;
    bool m_ok;
};

#endif // ARMY_REASSEMBLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
