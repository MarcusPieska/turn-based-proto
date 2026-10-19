//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ARMY_GENERAL_H
#define ARMY_GENERAL_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - ArmyGeneral -
//================================================================================================================================
//
 //  Stack-only general slot: commanded group head + stationary bit, packed in one u64. Training and MP boost walk the
 //  group chain from m_hd. Assign stamps has_moved so a newly attached army does not train the same turn.
 //
//================================================================================================================================

class ArmyGeneral {
public:
    ArmyGeneral ();

    void asgn (u16 hd); // Bind group head; stamps has_moved
    void unasgn (); // Clear group head
    u16 grp () const; // Commanded group head; U16_KEY_NULL if empty
    bool moved () const; // True if army moved since last turn-handler clear
    void set_moved (bool on); // War/unit movers stamp true; turn handler clears

    void train (GameState& st); // +1 level to one LAND !NONE unit below UNIT_TRAINING_MAX
    bool full (const GameState& st) const; // True when every LAND !NONE unit in group is at max
    void boost_mvt_pts (GameState& st); // +1 turn MP to every unit in the commanded group
    void boost_mvt_lvl (GameState& st); // +sum(mvt_boost[0..floor(avg lvl)]) to every unit in group

    static void stamp_moved (GameState& st, u16 seat, u16 hd); // Mark general whose group contains hd

private:
    u64 m_hd : 16; // Group head unit slot; U16_KEY_NULL if unassigned
    u64 m_moved : 1; // Stationary bit carried across turns
    u64 m_pad : 47; // Reserved
};

static_assert(sizeof(ArmyGeneral) == 8, "ArmyGeneral must pack into u64");

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

#endif // ARMY_GENERAL_H
