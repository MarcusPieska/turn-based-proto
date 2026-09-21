//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "player_ledger.h"
#include "game_state.h"
#include "assert_log.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

static GameState* s_state = nullptr;
static PlayerState* s_seats = nullptr;
static u16 s_seat_n = 0;
static const RuntimeStatics* s_statics = nullptr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static PlayerState* seat_ptr (u16 player) {
    if (s_state != nullptr) {
        GAME_EXPECT_RET(s_state->m_player_states != nullptr, nullptr, "PlayerLedger player states");
        GAME_EXPECT_RET(player < s_state->m_player_n, nullptr, "PlayerLedger seat");
        return &s_state->m_player_states[player];
    }
    GAME_EXPECT_RET(s_seats != nullptr, nullptr, "PlayerLedger seats");
    GAME_EXPECT_RET(player < s_seat_n, nullptr, "PlayerLedger seat");
    return &s_seats[player];
}

static const RuntimeStatics* statics_ptr () {
    if (s_state != nullptr) {
        return s_state->m_statics;
    }
    return s_statics;
}

//================================================================================================================================
//=> - PlayerLedger -
//================================================================================================================================

void PlayerLedger::bind_state (GameState* state) {
    s_state = state;
    s_seats = nullptr;
    s_seat_n = 0;
    s_statics = nullptr;
}

void PlayerLedger::bind_seats (PlayerState* seats, u16 n, const RuntimeStatics* statics) {
    s_state = nullptr;
    s_seats = seats;
    s_seat_n = n;
    s_statics = statics;
}

bool PlayerLedger::add_commerce (u16 player, u16 amount) {
    PlayerState* ps = seat_ptr(player);
    GAME_EXPECT_RET(ps != nullptr, false, "PlayerLedger add_commerce");
    ps->m_commerce_from_turn += static_cast<u32>(amount);
    return true;
}

bool PlayerLedger::add_research (u16 player, u16 amount) {
    PlayerState* ps = seat_ptr(player);
    GAME_EXPECT_RET(ps != nullptr, false, "PlayerLedger add_research");
    ps->m_research += static_cast<u32>(amount);
    return true;
}

u32 PlayerLedger::commerce (u16 player) {
    PlayerState* ps = seat_ptr(player);
    GAME_EXPECT_RET(ps != nullptr, 0u, "PlayerLedger commerce");
    return ps->m_commerce;
}

bool PlayerLedger::spend_commerce (u16 player, u32 amount) {
    PlayerState* ps = seat_ptr(player);
    GAME_EXPECT_RET(ps != nullptr, false, "PlayerLedger spend_commerce");
    if (ps->m_commerce < amount) {
        return false;
    }
    ps->m_commerce = ps->m_commerce - amount;
    return true;
}

bool PlayerLedger::upgrade_rates (u16* out_prod, u16* out_stat) {
    GAME_EXPECT_RET(out_prod != nullptr, false, "PlayerLedger upgrade_rates null prod");
    GAME_EXPECT_RET(out_stat != nullptr, false, "PlayerLedger upgrade_rates null stat");
    const RuntimeStatics* st = statics_ptr();
    GAME_EXPECT_RET(st != nullptr, false, "PlayerLedger statics");
    *out_prod = st->config().get_upgrade_cost_per_prod();
    *out_stat = st->config().get_upgrade_cost_per_stat_pt();
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
