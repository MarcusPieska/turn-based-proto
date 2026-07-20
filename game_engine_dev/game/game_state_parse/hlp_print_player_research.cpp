//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "hlp_print_player_research.h"
#include "hlp_civ_nm.h"
#include "hlp_parse_seed.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "tech_static_key.h"

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

static const u32 k_players_magic = 0x53524c50u;
static const u32 k_io_ver_min = 2u;
static const u8 k_bits_per_byte = 8u;

static const char* G_RT_LIB_A = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA_A = "../../";
static const char* G_RT_LIB_B = "../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA_B = "../";

//================================================================================================================================
//=> - Seat blob -
//================================================================================================================================

struct SeatResearch {
    u16 m_civ_index;
    u16 m_research_spending_perc;
    u16 m_current_research_target_idx;
    u32 m_commerce;
    u32 m_research;
    u32 m_commerce_from_turn;
    u32 m_tech_n;
    u8* m_tech_bytes;
};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void free_seats (SeatResearch* seats, u16 n) {
    if (seats == nullptr) {
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        delete[] seats[i].m_tech_bytes;
        seats[i].m_tech_bytes = nullptr;
    }
    delete[] seats;
}

static bool rd_tech_bits (std::FILE* fp, SeatResearch* seat) {
    seat->m_tech_n = 0;
    seat->m_tech_bytes = nullptr;
    if (std::fread(&seat->m_tech_n, sizeof(seat->m_tech_n), 1, fp) != 1) {
        return false;
    }
    if (seat->m_tech_n == 0) {
        return true;
    }
    const u32 byte_n = (seat->m_tech_n + static_cast<u32>(k_bits_per_byte - 1u)) / static_cast<u32>(k_bits_per_byte);
    seat->m_tech_bytes = new u8[byte_n];
    if (seat->m_tech_bytes == nullptr) {
        return false;
    }
    if (std::fread(seat->m_tech_bytes, 1, byte_n, fp) != byte_n) {
        delete[] seat->m_tech_bytes;
        seat->m_tech_bytes = nullptr;
        return false;
    }
    return true;
}

static bool load_players (cstr path, SeatResearch** out_seats, u16* out_n) {
    if (path == nullptr || out_seats == nullptr || out_n == nullptr) {
        return false;
    }
    *out_seats = nullptr;
    *out_n = 0;
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u16 player_n = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&player_n, sizeof(player_n), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    if (magic != k_players_magic || ver < k_io_ver_min || player_n == 0) {
        std::fclose(fp);
        return false;
    }
    SeatResearch* seats = new SeatResearch[player_n];
    for (u16 i = 0; i < player_n; ++i) {
        seats[i].m_tech_bytes = nullptr;
        seats[i].m_tech_n = 0;
        if (std::fread(&seats[i].m_civ_index, sizeof(seats[i].m_civ_index), 1, fp) != 1
            || std::fread(&seats[i].m_research_spending_perc, sizeof(seats[i].m_research_spending_perc), 1, fp) != 1
            || std::fread(&seats[i].m_current_research_target_idx, sizeof(seats[i].m_current_research_target_idx), 1, fp) != 1
            || std::fread(&seats[i].m_commerce, sizeof(seats[i].m_commerce), 1, fp) != 1
            || std::fread(&seats[i].m_research, sizeof(seats[i].m_research), 1, fp) != 1
            || std::fread(&seats[i].m_commerce_from_turn, sizeof(seats[i].m_commerce_from_turn), 1, fp) != 1
            || !rd_tech_bits(fp, &seats[i])) {
            free_seats(seats, i + 1);
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    *out_seats = seats;
    *out_n = player_n;
    return true;
}

static bool tech_on (const SeatResearch& seat, u32 idx) {
    if (seat.m_tech_bytes == nullptr || idx >= seat.m_tech_n) {
        return false;
    }
    const u8 byte = seat.m_tech_bytes[idx / static_cast<u32>(k_bits_per_byte)];
    return (byte & static_cast<u8>(1u << (idx % static_cast<u32>(k_bits_per_byte)))) != 0;
}

static cstr tech_nm (const RuntimeStatics& st, u16 idx) {
    if (idx >= st.tech().get_item_count()) {
        return "?";
    }
    cstr nm = st.tech().get_name(TechStaticDataKey::from_raw(idx));
    return (nm != nullptr) ? nm : "?";
}

static void print_seat (u16 p, const SeatResearch& seat, const RuntimeStatics& st, bool list) {
    std::printf("player=%u civ=%s slider=%u research=%u commerce=%u turn_bucket=%u",
        p, Helper_CivNm::nm(st, seat.m_civ_index), seat.m_research_spending_perc,
        seat.m_research, seat.m_commerce, seat.m_commerce_from_turn);
    if (seat.m_current_research_target_idx == U16_KEY_NULL) {
        std::printf(" target=none");
    } else {
        std::printf(" target=%s", tech_nm(st, seat.m_current_research_target_idx));
    }
    u32 owned = 0;
    for (u32 i = 0; i < seat.m_tech_n; ++i) {
        if (tech_on(seat, i)) {
            owned = owned + 1u;
        }
    }
    std::printf(" techs=%u/%u\n", owned, seat.m_tech_n);
    if (!list) {
        return;
    }
    for (u32 i = 0; i < seat.m_tech_n; ++i) {
        if (!tech_on(seat, i)) {
            continue;
        }
        std::printf("  %s\n", tech_nm(st, static_cast<u16>(i)));
    }
}

//================================================================================================================================
//=> - Helper_PrintPlayerResearch -
//================================================================================================================================

bool Helper_PrintPlayerResearch::run (u32 turn, bool list) {
    if (!Helper_ParseSeed::load()) {
        std::printf("Helper_PrintPlayerResearch: parse_seed load failed\n");
        return false;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB_A, G_RT_DATA_A) && !loader.load(G_RT_LIB_B, G_RT_DATA_B)) {
        std::printf("Helper_PrintPlayerResearch: statics load failed\n");
        return false;
    }
    const RuntimeStatics& st = loader.statics();
    char path[384];
    if (!Helper_ParseSeed::players_path(turn, path, sizeof(path))) {
        return false;
    }
    SeatResearch* seats = nullptr;
    u16 player_n = 0;
    if (!load_players(path, &seats, &player_n)) {
        std::printf("Helper_PrintPlayerResearch: load failed: %s\n", path);
        return false;
    }
    std::printf("seed=%u players=%u turn=%04u path=%s\n",
        Helper_ParseSeed::seed(), Helper_ParseSeed::players(), turn, path);
    for (u16 p = 0; p < player_n; ++p) {
        print_seat(p, seats[p], st, list);
    }
    free_seats(seats, player_n);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
