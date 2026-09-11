//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MOCK_MUSTER_SIEGE_H
#define MOCK_MUSTER_SIEGE_H

#include "game_primitives.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"

class GameState;

//================================================================================================================================
//=> - MockSiegeRslt -
//================================================================================================================================
//
//  Per-city mock assault outcome: taken flag, live/dead counts, wall microsec timing.
//
//================================================================================================================================

struct MockSiegeRslt {
    bool m_taken; // True if mock defenders wiped
    u16 m_army_live; // Attacker copies with health > 0 after
    u16 m_army_n0; // Attacker copies at siege start
    u16 m_def_live; // Defender copies with health > 0 after
    u16 m_def_n0; // Defender copies at siege start
    u64 m_us; // Wall time for this siege in microseconds
};

//================================================================================================================================
//=> - MockMusterSiege -
//================================================================================================================================
//
//  Dry muster via UnitGroupManagement collect APIs, copy UnitAddStruct combat fields, then mock-siege
//  enemy cities with CombatMng on copies. Siege writes battle damage back into the army so a campaign
//  can retain losses across successive cities until the force is depleted.
//
//================================================================================================================================

class MockMusterSiege {
public:
    static const u16 k_cap = 2048u;

    MockMusterSiege ();

    bool collect (GameState& s, u16 seat); // Scan own cities; leave-one then leave-five; fill army copies
    u16 army_n () const;
    u16 army_live () const;
    u16 seat () const;
    bool ok () const;

    bool siege (GameState& s, u16 city_x, u16 city_y, MockSiegeRslt* out); // One timed mock assault; keeps damage
    bool campaign (GameState& s, u16 foe_seat, u16* out_taken, u16* out_foe_n, u16* out_turns); // Nearest-first; out_turns = sieges fought

private:
    MockMusterSiege (const MockMusterSiege& o) = delete;
    MockMusterSiege& operator= (const MockMusterSiege& o) = delete;

    bool copy_tile_foes (GameState& s, u16 x, u16 y, u16 atk_seat, UnitAddStruct* dst, u16 dst_cap, u16* out_n);
    void refill_mp (GameState& s, UnitAddStruct* u, u16 n);
    void prune_dead ();
    bool can_barrage (GameState& s, u16 typ) const;
    bool can_attack (GameState& s, u16 typ) const;
    u16 atk_stat (GameState& s, u16 typ) const;
    u16 def_stat (GameState& s, u16 typ) const;
    bool mock_barrage (GameState& s, UnitAddStruct* atk, u16 an, UnitAddStruct* def, u16 dn, u16 cx, u16 cy, u16* ded_io);
    bool mock_melee (GameState& s, UnitAddStruct* atk, u16 an, UnitAddStruct* def, u16 dn, u16 cx, u16 cy, bool* out_taken);

    UnitAddStruct m_army[k_cap];
    u16 m_army_n;
    u16 m_seat;
    bool m_ok;
};

#endif // MOCK_MUSTER_SIEGE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
