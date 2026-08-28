//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_AI_HELPERS_H
#define GEN_AI_HELPERS_H

#include "game_primitives.h"
#include "gen_settlement_order.h"
#include "starting_point_generator.h"

class GameArraySimple;

//================================================================================================================================
//=> - GenAiHelpersRslt -
//================================================================================================================================

struct GenAiHelpersRslt {
    u32 m_city_n; // Planned city sites stamped
    u32 m_ord_n; // Packed settle-order sites across all starts
    u32 m_mtn_pass_n; // Mountain passage tiles stamped
    u32 m_mtn_pass_fort_n; // Passage fort steps stamped
    u32 m_bn_fort_n; // Bottleneck fort centers stamped
};

//================================================================================================================================
//=> - GenAiHelpers -
//================================================================================================================================
//
//  Thin orchestrator for AI pre-compute: city sites, exclusive settle order, mountain passes, bottleneck forts.
//  Stamps m_ai_ov_intent only. m_settler_blocked is city-proximity blocking (GenSettlementTargets / punch), not
//  an AI-helper concern. Order is owned here for the match; m_target_settlements is not consulted (still 0 until
//  GameLoop::arm_settling — leave that filter alone for now). WhiteboardMng::init must run before begin().
//  Save/load must persist settle order plus tile intents (tile dump already has m_ai_ov_intent). Start tiles are
//  unmarked as CITY before order so capitals are not settle targets. Punch / restamp of existing-city blocks
//  still lives in SettlerTurnHandler — revisit whether that moves here.
//
//================================================================================================================================

class GenAiHelpers {
public:
    GenAiHelpers ();
    ~GenAiHelpers ();

    bool begin (GameArraySimple& map);
    bool build (const SpgCoordPair* starts, u32 start_n, GenAiHelpersRslt* out);
    void clr ();
    bool ok () const;
    const GenSettlementOrder& order () const;
    GenSettlementOrder& order ();

private:
    GenAiHelpers (const GenAiHelpers& other) = delete;
    GenAiHelpers& operator= (const GenAiHelpers& other) = delete;
    GenAiHelpers (GenAiHelpers&& other) = delete;
    GenAiHelpers& operator= (GenAiHelpers&& other) = delete;

    GameArraySimple* m_map; // Non-owning map stamped by build
    GenSettlementOrder* m_ord; // Exclusive settle order; allocated in begin
    bool m_ok; // True after begin
};

#endif // GEN_AI_HELPERS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
