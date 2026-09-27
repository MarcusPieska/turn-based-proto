//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "simple_dyn_state_io.h"

#include <cstdio>

#include "bit_array.h"
#include "game_state.h"
#include "map_bit_overlay.h"
#include "tech_age_mng.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static const u8 k_bits_per_byte = 8u;

void SimpleDynStateIO::clr_seat (PlayerState& ps) {
    delete[] ps.m_small_wonder_city;
    ps.m_small_wonder_city = nullptr;
    delete ps.m_explored_overlay;
    ps.m_explored_overlay = nullptr;
    delete ps.m_techs_researched;
    ps.m_techs_researched = nullptr;
    delete ps.m_tech_age;
    ps.m_tech_age = nullptr;
    ps.m_res_ledger.clear();
}

void SimpleDynStateIO::clr_seats (PlayerState* seats, u16 player_n) {
    if (seats == nullptr) {
        return;
    }
    for (u16 i = 0; i < player_n; ++i) {
        clr_seat(seats[i]);
    }
    delete[] seats;
}

bool SimpleDynStateIO::wr_bit_cl (void* fp_raw, const BitArrayCL* ba) {
    std::FILE* fp = static_cast<std::FILE*>(fp_raw);
    if (fp == nullptr) {
        return false;
    }
    u32 n = 0;
    if (ba != nullptr) {
        n = ba->get_count();
    }
    if (std::fwrite(&n, sizeof(n), 1, fp) != 1) {
        return false;
    }
    if (n == 0u) {
        return true;
    }
    const u32 byte_n = (n + static_cast<u32>(k_bits_per_byte) - 1u) / static_cast<u32>(k_bits_per_byte);
    u8* buf = new u8[byte_n];
    if (buf == nullptr) {
        return false;
    }
    for (u32 i = 0; i < byte_n; ++i) {
        buf[i] = 0;
    }
    for (u32 i = 0; i < n; ++i) {
        if (ba->get_bit(i) != 0) {
            buf[i / static_cast<u32>(k_bits_per_byte)] =
                static_cast<u8>(buf[i / static_cast<u32>(k_bits_per_byte)]
                    | static_cast<u8>(1u << (i % static_cast<u32>(k_bits_per_byte))));
        }
    }
    const bool ok = std::fwrite(buf, 1, byte_n, fp) == byte_n;
    delete[] buf;
    return ok;
}

bool SimpleDynStateIO::rd_bit_cl (void* fp_raw, BitArrayCL** out) {
    std::FILE* fp = static_cast<std::FILE*>(fp_raw);
    if (fp == nullptr || out == nullptr) {
        return false;
    }
    u32 n = 0;
    if (std::fread(&n, sizeof(n), 1, fp) != 1) {
        return false;
    }
    delete *out;
    *out = nullptr;
    if (n == 0u) {
        return true;
    }
    const u32 byte_n = (n + static_cast<u32>(k_bits_per_byte) - 1u) / static_cast<u32>(k_bits_per_byte);
    u8* buf = new u8[byte_n];
    if (buf == nullptr) {
        return false;
    }
    if (std::fread(buf, 1, byte_n, fp) != byte_n) {
        delete[] buf;
        return false;
    }
    BitArrayCL* ba = new BitArrayCL(n);
    for (u32 i = 0; i < n; ++i) {
        const u8 b = buf[i / static_cast<u32>(k_bits_per_byte)];
        if ((b & static_cast<u8>(1u << (i % static_cast<u32>(k_bits_per_byte)))) != 0) {
            ba->set_bit(i);
        }
    }
    delete[] buf;
    *out = ba;
    return true;
}

bool SimpleDynStateIO::wr_seat (void* fp_raw, const PlayerState& ps, u16 small_wonder_n) {
    std::FILE* fp = static_cast<std::FILE*>(fp_raw);
    if (fp == nullptr) {
        return false;
    }
    const u8 ai_units = static_cast<u8>(ps.m_ai_units);
    if (std::fwrite(&ps.m_commerce, sizeof(ps.m_commerce), 1, fp) != 1
        || std::fwrite(&ps.m_research, sizeof(ps.m_research), 1, fp) != 1
        || std::fwrite(&ps.m_commerce_from_turn, sizeof(ps.m_commerce_from_turn), 1, fp) != 1
        || std::fwrite(&ps.m_ai_controlled, sizeof(ps.m_ai_controlled), 1, fp) != 1
        || std::fwrite(&ps.m_is_active, sizeof(ps.m_is_active), 1, fp) != 1
        || std::fwrite(&ps.m_civ_index, sizeof(ps.m_civ_index), 1, fp) != 1
        || std::fwrite(&ps.m_current_research_target_idx, sizeof(ps.m_current_research_target_idx), 1, fp) != 1
        || std::fwrite(&ps.m_research_spending_perc, sizeof(ps.m_research_spending_perc), 1, fp) != 1
        || std::fwrite(&ps.m_free_land_unit_support, sizeof(ps.m_free_land_unit_support), 1, fp) != 1
        || std::fwrite(&ps.m_free_naval_unit_support, sizeof(ps.m_free_naval_unit_support), 1, fp) != 1
        || std::fwrite(&ps.m_land_unit_upkeep_needed, sizeof(ps.m_land_unit_upkeep_needed), 1, fp) != 1
        || std::fwrite(&ps.m_naval_unit_upkeep_needed, sizeof(ps.m_naval_unit_upkeep_needed), 1, fp) != 1
        || std::fwrite(&ps.m_target_new_land_unit_support, sizeof(ps.m_target_new_land_unit_support), 1, fp) != 1
        || std::fwrite(&ps.m_target_new_naval_unit_support, sizeof(ps.m_target_new_naval_unit_support), 1, fp) != 1
        || std::fwrite(&ps.m_this_turn_new_land_unit_build_support, sizeof(ps.m_this_turn_new_land_unit_build_support), 1, fp) != 1
        || std::fwrite(&ps.m_this_turn_new_naval_unit_build_support, sizeof(ps.m_this_turn_new_naval_unit_build_support), 1, fp) != 1
        || std::fwrite(&ps.m_last_turn_new_land_unit_build_support, sizeof(ps.m_last_turn_new_land_unit_build_support), 1, fp) != 1
        || std::fwrite(&ps.m_last_turn_new_naval_unit_build_support, sizeof(ps.m_last_turn_new_naval_unit_build_support), 1, fp) != 1
        || std::fwrite(&ps.m_worker_mvt_to_build_perc, sizeof(ps.m_worker_mvt_to_build_perc), 1, fp) != 1
        || std::fwrite(&ps.m_target_settlements, sizeof(ps.m_target_settlements), 1, fp) != 1
        || std::fwrite(ps.m_settler_idx, sizeof(u16), SETTLER_MISSION_SLOTS, fp) != SETTLER_MISSION_SLOTS
        || std::fwrite(ps.m_settle_x, sizeof(u16), SETTLER_MISSION_SLOTS, fp) != SETTLER_MISSION_SLOTS
        || std::fwrite(ps.m_settle_y, sizeof(u16), SETTLER_MISSION_SLOTS, fp) != SETTLER_MISSION_SLOTS
        || std::fwrite(&ps.m_settle_n, sizeof(ps.m_settle_n), 1, fp) != 1
        || std::fwrite(&ps.m_scout_1_idx, sizeof(ps.m_scout_1_idx), 1, fp) != 1
        || std::fwrite(&ps.m_scout_2_idx, sizeof(ps.m_scout_2_idx), 1, fp) != 1
        || std::fwrite(&ps.m_scout_3_idx, sizeof(ps.m_scout_3_idx), 1, fp) != 1
        || std::fwrite(&ps.m_scout_4_idx, sizeof(ps.m_scout_4_idx), 1, fp) != 1
        || std::fwrite(&ps.m_last_turn_settler_count, sizeof(ps.m_last_turn_settler_count), 1, fp) != 1
        || std::fwrite(&ps.m_this_turn_settler_build_n, sizeof(ps.m_this_turn_settler_build_n), 1, fp) != 1
        || std::fwrite(&ps.m_last_turn_settler_build_n, sizeof(ps.m_last_turn_settler_build_n), 1, fp) != 1
        || std::fwrite(&ps.m_defensive_unit_count, sizeof(ps.m_defensive_unit_count), 1, fp) != 1
        || std::fwrite(&ps.m_last_turn_population_count, sizeof(ps.m_last_turn_population_count), 1, fp) != 1
        || std::fwrite(&ps.m_last_turn_city_count, sizeof(ps.m_last_turn_city_count), 1, fp) != 1
        || std::fwrite(&ps.m_this_turn_population_count, sizeof(ps.m_this_turn_population_count), 1, fp) != 1
        || std::fwrite(&ps.m_this_turn_city_count, sizeof(ps.m_this_turn_city_count), 1, fp) != 1
        || std::fwrite(&ps.m_worker_tile_opt_scan, sizeof(ps.m_worker_tile_opt_scan), 1, fp) != 1
        || std::fwrite(&ps.m_worker_tile_opt_reassign, sizeof(ps.m_worker_tile_opt_reassign), 1, fp) != 1
        || std::fwrite(&ps.m_tech_just_researched, sizeof(ps.m_tech_just_researched), 1, fp) != 1
        || std::fwrite(&ps.m_lucky, sizeof(ps.m_lucky), 1, fp) != 1
        || std::fwrite(&ps.m_at_war, sizeof(ps.m_at_war), 1, fp) != 1
        || std::fwrite(&ai_units, sizeof(ai_units), 1, fp) != 1
        || std::fwrite(&ps.m_ai_units_tog, sizeof(ps.m_ai_units_tog), 1, fp) != 1) {
        return false;
    }
    const u8 has_sw = (ps.m_small_wonder_city != nullptr && small_wonder_n != 0u) ? 1u : 0u;
    const u8 has_fog = (ps.m_explored_overlay != nullptr) ? 1u : 0u;
    const u8 has_tech = (ps.m_techs_researched != nullptr) ? 1u : 0u;
    const u8 has_age = (ps.m_tech_age != nullptr) ? 1u : 0u;
    if (std::fwrite(&has_sw, sizeof(has_sw), 1, fp) != 1
        || std::fwrite(&has_fog, sizeof(has_fog), 1, fp) != 1
        || std::fwrite(&has_tech, sizeof(has_tech), 1, fp) != 1
        || std::fwrite(&has_age, sizeof(has_age), 1, fp) != 1) {
        return false;
    }
    if (has_sw != 0u) {
        if (std::fwrite(ps.m_small_wonder_city, sizeof(u16), static_cast<size_t>(small_wonder_n), fp)
            != static_cast<size_t>(small_wonder_n)) {
            return false;
        }
    }
    if (has_fog != 0u && !ps.m_explored_overlay->wr(fp)) {
        return false;
    }
    if (has_tech != 0u && !wr_bit_cl(fp, ps.m_techs_researched)) {
        return false;
    }
    if (has_age != 0u && !ps.m_tech_age->wr(fp)) {
        return false;
    }
    return ps.m_res_ledger.wr(fp);
}

bool SimpleDynStateIO::rd_seat (void* fp_raw, PlayerState& ps, u16 small_wonder_n) {
    std::FILE* fp = static_cast<std::FILE*>(fp_raw);
    if (fp == nullptr) {
        return false;
    }
    clr_seat(ps);
    u8 ai_units = 0;
    if (std::fread(&ps.m_commerce, sizeof(ps.m_commerce), 1, fp) != 1
        || std::fread(&ps.m_research, sizeof(ps.m_research), 1, fp) != 1
        || std::fread(&ps.m_commerce_from_turn, sizeof(ps.m_commerce_from_turn), 1, fp) != 1
        || std::fread(&ps.m_ai_controlled, sizeof(ps.m_ai_controlled), 1, fp) != 1
        || std::fread(&ps.m_is_active, sizeof(ps.m_is_active), 1, fp) != 1
        || std::fread(&ps.m_civ_index, sizeof(ps.m_civ_index), 1, fp) != 1
        || std::fread(&ps.m_current_research_target_idx, sizeof(ps.m_current_research_target_idx), 1, fp) != 1
        || std::fread(&ps.m_research_spending_perc, sizeof(ps.m_research_spending_perc), 1, fp) != 1
        || std::fread(&ps.m_free_land_unit_support, sizeof(ps.m_free_land_unit_support), 1, fp) != 1
        || std::fread(&ps.m_free_naval_unit_support, sizeof(ps.m_free_naval_unit_support), 1, fp) != 1
        || std::fread(&ps.m_land_unit_upkeep_needed, sizeof(ps.m_land_unit_upkeep_needed), 1, fp) != 1
        || std::fread(&ps.m_naval_unit_upkeep_needed, sizeof(ps.m_naval_unit_upkeep_needed), 1, fp) != 1
        || std::fread(&ps.m_target_new_land_unit_support, sizeof(ps.m_target_new_land_unit_support), 1, fp) != 1
        || std::fread(&ps.m_target_new_naval_unit_support, sizeof(ps.m_target_new_naval_unit_support), 1, fp) != 1
        || std::fread(&ps.m_this_turn_new_land_unit_build_support, sizeof(ps.m_this_turn_new_land_unit_build_support), 1, fp) != 1
        || std::fread(&ps.m_this_turn_new_naval_unit_build_support, sizeof(ps.m_this_turn_new_naval_unit_build_support), 1, fp) != 1
        || std::fread(&ps.m_last_turn_new_land_unit_build_support, sizeof(ps.m_last_turn_new_land_unit_build_support), 1, fp) != 1
        || std::fread(&ps.m_last_turn_new_naval_unit_build_support, sizeof(ps.m_last_turn_new_naval_unit_build_support), 1, fp) != 1
        || std::fread(&ps.m_worker_mvt_to_build_perc, sizeof(ps.m_worker_mvt_to_build_perc), 1, fp) != 1
        || std::fread(&ps.m_target_settlements, sizeof(ps.m_target_settlements), 1, fp) != 1
        || std::fread(ps.m_settler_idx, sizeof(u16), SETTLER_MISSION_SLOTS, fp) != SETTLER_MISSION_SLOTS
        || std::fread(ps.m_settle_x, sizeof(u16), SETTLER_MISSION_SLOTS, fp) != SETTLER_MISSION_SLOTS
        || std::fread(ps.m_settle_y, sizeof(u16), SETTLER_MISSION_SLOTS, fp) != SETTLER_MISSION_SLOTS
        || std::fread(&ps.m_settle_n, sizeof(ps.m_settle_n), 1, fp) != 1
        || std::fread(&ps.m_scout_1_idx, sizeof(ps.m_scout_1_idx), 1, fp) != 1
        || std::fread(&ps.m_scout_2_idx, sizeof(ps.m_scout_2_idx), 1, fp) != 1
        || std::fread(&ps.m_scout_3_idx, sizeof(ps.m_scout_3_idx), 1, fp) != 1
        || std::fread(&ps.m_scout_4_idx, sizeof(ps.m_scout_4_idx), 1, fp) != 1
        || std::fread(&ps.m_last_turn_settler_count, sizeof(ps.m_last_turn_settler_count), 1, fp) != 1
        || std::fread(&ps.m_this_turn_settler_build_n, sizeof(ps.m_this_turn_settler_build_n), 1, fp) != 1
        || std::fread(&ps.m_last_turn_settler_build_n, sizeof(ps.m_last_turn_settler_build_n), 1, fp) != 1
        || std::fread(&ps.m_defensive_unit_count, sizeof(ps.m_defensive_unit_count), 1, fp) != 1
        || std::fread(&ps.m_last_turn_population_count, sizeof(ps.m_last_turn_population_count), 1, fp) != 1
        || std::fread(&ps.m_last_turn_city_count, sizeof(ps.m_last_turn_city_count), 1, fp) != 1
        || std::fread(&ps.m_this_turn_population_count, sizeof(ps.m_this_turn_population_count), 1, fp) != 1
        || std::fread(&ps.m_this_turn_city_count, sizeof(ps.m_this_turn_city_count), 1, fp) != 1
        || std::fread(&ps.m_worker_tile_opt_scan, sizeof(ps.m_worker_tile_opt_scan), 1, fp) != 1
        || std::fread(&ps.m_worker_tile_opt_reassign, sizeof(ps.m_worker_tile_opt_reassign), 1, fp) != 1
        || std::fread(&ps.m_tech_just_researched, sizeof(ps.m_tech_just_researched), 1, fp) != 1
        || std::fread(&ps.m_lucky, sizeof(ps.m_lucky), 1, fp) != 1
        || std::fread(&ps.m_at_war, sizeof(ps.m_at_war), 1, fp) != 1
        || std::fread(&ai_units, sizeof(ai_units), 1, fp) != 1
        || std::fread(&ps.m_ai_units_tog, sizeof(ps.m_ai_units_tog), 1, fp) != 1) {
        return false;
    }
    ps.m_ai_units = static_cast<AiUnits>(ai_units);
    u8 has_sw = 0;
    u8 has_fog = 0;
    u8 has_tech = 0;
    u8 has_age = 0;
    if (std::fread(&has_sw, sizeof(has_sw), 1, fp) != 1
        || std::fread(&has_fog, sizeof(has_fog), 1, fp) != 1
        || std::fread(&has_tech, sizeof(has_tech), 1, fp) != 1
        || std::fread(&has_age, sizeof(has_age), 1, fp) != 1) {
        return false;
    }
    if (has_sw != 0u) {
        if (small_wonder_n == 0u) {
            return false;
        }
        ps.m_small_wonder_city = new u16[small_wonder_n];
        if (std::fread(ps.m_small_wonder_city, sizeof(u16), static_cast<size_t>(small_wonder_n), fp)
            != static_cast<size_t>(small_wonder_n)) {
            return false;
        }
    }
    if (has_fog != 0u) {
        ps.m_explored_overlay = new MapBitOverlay(0, 0);
        if (!ps.m_explored_overlay->rd(fp)) {
            return false;
        }
    }
    if (has_tech != 0u && !rd_bit_cl(fp, &ps.m_techs_researched)) {
        return false;
    }
    if (has_age != 0u) {
        if (!TechAgeMng::ready()) {
            return false;
        }
        ps.m_tech_age = new TechAgeMng();
        if (!ps.m_tech_age->rd(fp)) {
            return false;
        }
    }
    return ps.m_res_ledger.rd(fp);
}

//================================================================================================================================
//=> - SimpleDynStateIO -
//================================================================================================================================

bool SimpleDynStateIO::save_players (cstr path, const PlayerState* seats, u16 player_n, u16 small_wonder_n) {
    if (path == nullptr || seats == nullptr || player_n == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    const u32 magic = k_magic;
    const u32 ver = k_ver;
    if (std::fwrite(&magic, sizeof(magic), 1, fp) != 1
        || std::fwrite(&ver, sizeof(ver), 1, fp) != 1
        || std::fwrite(&player_n, sizeof(player_n), 1, fp) != 1
        || std::fwrite(&small_wonder_n, sizeof(small_wonder_n), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    for (u16 p = 0; p < player_n; ++p) {
        if (!wr_seat(fp, seats[p], small_wonder_n)) {
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    return true;
}

bool SimpleDynStateIO::save_players (cstr path, const GameState& state) {
    return save_players(path, state.m_player_states, state.m_player_n, state.m_small_wonder_count);
}

bool SimpleDynStateIO::load_players (cstr path, PlayerState*& seats, u16& player_n) {
    if (path == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u16 n = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&n, sizeof(n), 1, fp) != 1
        || n == 0) {
        std::fclose(fp);
        return false;
    }
    static const u32 k_legacy_magic = 0x53524c50u;
    if (magic == k_legacy_magic && ver >= 2u) {
        clr_seats(seats, player_n);
        seats = nullptr;
        player_n = 0;
        seats = new PlayerState[n];
        player_n = n;
        for (u16 p = 0; p < n; ++p) {
            PlayerState& ps = seats[p];
            if (std::fread(&ps.m_civ_index, sizeof(ps.m_civ_index), 1, fp) != 1
                || std::fread(&ps.m_research_spending_perc, sizeof(ps.m_research_spending_perc), 1, fp) != 1
                || std::fread(&ps.m_current_research_target_idx, sizeof(ps.m_current_research_target_idx), 1, fp) != 1
                || std::fread(&ps.m_commerce, sizeof(ps.m_commerce), 1, fp) != 1
                || std::fread(&ps.m_research, sizeof(ps.m_research), 1, fp) != 1
                || std::fread(&ps.m_commerce_from_turn, sizeof(ps.m_commerce_from_turn), 1, fp) != 1
                || !rd_bit_cl(fp, &ps.m_techs_researched)) {
                clr_seats(seats, player_n);
                seats = nullptr;
                player_n = 0;
                std::fclose(fp);
                return false;
            }
        }
        std::fclose(fp);
        return true;
    }
    u16 sw_n = 0;
    if (std::fread(&sw_n, sizeof(sw_n), 1, fp) != 1
        || magic != k_magic
        || ver != k_ver) {
        std::fclose(fp);
        return false;
    }
    clr_seats(seats, player_n);
    seats = nullptr;
    player_n = 0;
    seats = new PlayerState[n];
    player_n = n;
    for (u16 p = 0; p < n; ++p) {
        if (!rd_seat(fp, seats[p], sw_n)) {
            clr_seats(seats, player_n);
            seats = nullptr;
            player_n = 0;
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    return true;
}

bool SimpleDynStateIO::load_players (cstr path, GameState& state) {
    PlayerState* seats = state.m_player_states;
    u16 n = state.m_player_n;
    if (!load_players(path, seats, n)) {
        return false;
    }
    state.m_player_states = seats;
    state.m_player_n = n;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
