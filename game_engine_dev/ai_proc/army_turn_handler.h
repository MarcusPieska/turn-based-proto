//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef ARMY_TURN_HANDLER_H
#define ARMY_TURN_HANDLER_H

#include "game_primitives.h"

class GameState;
class PathGradients;

//================================================================================================================================
//=> - WarAssault -
//================================================================================================================================
//
//  Result of assault: Ok captured (and claimed), Stall out of assault MP with fight left, Fail hard stop.
//
//================================================================================================================================

enum class WarAssault : u8 {
    Ok = 0,
    Stall = 1,
    Fail = 2
};

//================================================================================================================================
//=> - ArmyPhase / ArmyTurnRslt -
//================================================================================================================================
//
//  Self-drive state for one army vs its current target. handle advances one phase step per call.
//  Busy still working; NeedTarget captured/rejoined/healed and ready for a new city; Stop hard end.
//
//================================================================================================================================

enum class ArmyPhase : u8 {
    Idle = 0,
    March = 1,
    Assault = 2,
    Rejoin = 3
};

enum class ArmyTurnRslt : u8 {
    Busy = 0,
    NeedTarget = 1,
    Stop = 2
};

//================================================================================================================================
//=> - WarArmy -
//================================================================================================================================
//
//  Per-army in/out for ArmyTurnHandler. WarTurnHandler owns the slot; head/split/target/paths/phase
//  are inputs (updated in place); stall/barrage/assault are outputs from the last army op.
//
//================================================================================================================================

struct WarArmy {
    u16 m_hd; // Group head UnitAddKey raw; U16_KEY_NULL if empty
    u16 m_split; // Remainder outside city after split; U16_KEY_NULL if none
    u16 m_tx; // Active target city x
    u16 m_ty; // Active target city y
    u16 m_idx; // Slot index for logs
    u16 m_path_i; // Index into m_paths
    PathGradients* m_paths; // Non-owning path bank; null if unset
    ArmyPhase m_phase; // Self-drive phase for handle
    bool m_stall; // True when last assault returned Stall
    u32 m_br_tot; // Last barrage pass total HP removed
    u32 m_br_last; // Last barrage pass final-shot HP removed
    WarAssault m_assault; // Last assault result
};

//================================================================================================================================
//=> - ArmyTurnHandler -
//================================================================================================================================
//
//  Single-army self-driver: handle walks, assaults (claims on Ok), rejoins, and heals one target cycle.
//  Step helpers are private; ArmyTurnTestHlp (test drivers only) is friended for step-level tests.
//  Campaign/target pick stays in WarTurnHandler; no multi-army management here.
//
//================================================================================================================================

class ArmyTurnTestHlp;

class ArmyTurnHandler {
public:
    ArmyTurnHandler () = delete;

    static ArmyTurnRslt handle (GameState& s, u16 seat, u16 enemy, WarArmy& army);

private:
    friend class ArmyTurnTestHlp;

    static bool walk (GameState& s, WarArmy& army);
    static WarAssault assault (GameState& s, u16 seat, WarArmy& army, u16 city_x, u16 city_y);
    static bool rejoin_move (GameState& s, u16 seat, u16 enemy, WarArmy& army);
    static bool rejoin_link (GameState& s, WarArmy& army);
    static u16 rest_heal (GameState& s, WarArmy& army);
    static bool can_fight (const GameState& s, const WarArmy& army);
    static bool can_cont (const GameState& s, const WarArmy& army);
    static void refill_grp (GameState& s, u16 head_idx);
    static void claim_city (GameState& s, u16 seat, u16 x, u16 y);
};

#endif // ARMY_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
