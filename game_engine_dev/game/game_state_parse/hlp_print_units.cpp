//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "hlp_print_units.h"
#include "hlp_civ_nm.h"
#include "hlp_parse_seed.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

static const u32 k_players_magic = 0x53524c50u;
static const u32 k_units_magic = 0x53494e55u;
static const u32 k_io_ver_min = 2u;
static const u8 k_bits_per_byte = 8u;

static const char* G_RT_LIB_A = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA_A = "../../";
static const char* G_RT_LIB_B = "../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA_B = "../";

//================================================================================================================================
//=> - Dump blobs -
//================================================================================================================================

struct SeatCiv {
    u16 m_civ_index;
};

struct UnitDumpRec {
    u16 m_key;
    UnitAddStruct m_u;
};

struct UnitsDump {
    UnitDumpRec* m_recs;
    u32 m_n;
};

//================================================================================================================================
//=> - Load -
//================================================================================================================================

static bool load_seats (cstr path, SeatCiv** out_seats, u16* out_n) {
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
    SeatCiv* seats = new SeatCiv[player_n];
    for (u16 i = 0; i < player_n; ++i) {
        u16 civ = 0;
        u16 slider = 0;
        u16 target = 0;
        u32 commerce = 0;
        u32 research = 0;
        u32 turn_bucket = 0;
        u32 tech_n = 0;
        if (std::fread(&civ, sizeof(civ), 1, fp) != 1
            || std::fread(&slider, sizeof(slider), 1, fp) != 1
            || std::fread(&target, sizeof(target), 1, fp) != 1
            || std::fread(&commerce, sizeof(commerce), 1, fp) != 1
            || std::fread(&research, sizeof(research), 1, fp) != 1
            || std::fread(&turn_bucket, sizeof(turn_bucket), 1, fp) != 1
            || std::fread(&tech_n, sizeof(tech_n), 1, fp) != 1) {
            delete[] seats;
            std::fclose(fp);
            return false;
        }
        if (tech_n > 0) {
            const u32 byte_n = (tech_n + static_cast<u32>(k_bits_per_byte - 1u)) / static_cast<u32>(k_bits_per_byte);
            if (std::fseek(fp, static_cast<long>(byte_n), SEEK_CUR) != 0) {
                delete[] seats;
                std::fclose(fp);
                return false;
            }
        }
        seats[i].m_civ_index = civ;
    }
    std::fclose(fp);
    *out_seats = seats;
    *out_n = player_n;
    return true;
}

static void free_units (UnitsDump* dump) {
    if (dump == nullptr) {
        return;
    }
    delete[] dump->m_recs;
    dump->m_recs = nullptr;
    dump->m_n = 0;
}

static bool load_units (cstr path, UnitsDump* out) {
    if (path == nullptr || out == nullptr) {
        return false;
    }
    out->m_recs = nullptr;
    out->m_n = 0;
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u32 live_n = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&live_n, sizeof(live_n), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    if (magic != k_units_magic || ver < k_io_ver_min) {
        std::fclose(fp);
        return false;
    }
    UnitDumpRec* recs = new UnitDumpRec[live_n == 0 ? 1u : live_n];
    for (u32 i = 0; i < live_n; ++i) {
        if (std::fread(&recs[i].m_key, sizeof(recs[i].m_key), 1, fp) != 1
            || std::fread(&recs[i].m_u, sizeof(recs[i].m_u), 1, fp) != 1) {
            delete[] recs;
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    out->m_recs = recs;
    out->m_n = live_n;
    return true;
}

static cstr unit_nm (const RuntimeStatics& st, u16 typ_idx) {
    if (typ_idx >= st.unit().get_item_count()) {
        return "?";
    }
    cstr nm = st.unit().get_name(UnitStaticDataKey::from_raw(typ_idx));
    return (nm != nullptr) ? nm : "?";
}

static u32 count_for (const UnitsDump& units, u16 player) {
    u32 n = 0;
    for (u32 i = 0; i < units.m_n; ++i) {
        if (units.m_recs[i].m_u.m_player_idx == player) {
            n = n + 1u;
        }
    }
    return n;
}

static void print_pos (u16 x, u16 y) {
    if (x == U16_KEY_NULL || y == U16_KEY_NULL) {
        std::printf(" x=none y=none");
    } else {
        std::printf(" x=%u y=%u", x, y);
    }
}

//================================================================================================================================
//=> - Helper_PrintUnits -
//================================================================================================================================

bool Helper_PrintUnits::run (u32 turn, bool list) {
    if (!Helper_ParseSeed::load()) {
        std::printf("Helper_PrintUnits: parse_seed load failed\n");
        return false;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB_A, G_RT_DATA_A) && !loader.load(G_RT_LIB_B, G_RT_DATA_B)) {
        std::printf("Helper_PrintUnits: statics load failed\n");
        return false;
    }
    const RuntimeStatics& st = loader.statics();
    char players_path[384];
    char units_path[384];
    if (!Helper_ParseSeed::players_path(turn, players_path, sizeof(players_path))
        || !Helper_ParseSeed::units_path(turn, units_path, sizeof(units_path))) {
        return false;
    }
    SeatCiv* seats = nullptr;
    u16 player_n = 0;
    if (!load_seats(players_path, &seats, &player_n)) {
        std::printf("Helper_PrintUnits: players load failed: %s\n", players_path);
        return false;
    }
    UnitsDump units = {};
    if (!load_units(units_path, &units)) {
        std::printf("Helper_PrintUnits: units load failed: %s\n", units_path);
        delete[] seats;
        return false;
    }
    std::printf("seed=%u players=%u turn=%04u path=%s\n",
        Helper_ParseSeed::seed(), Helper_ParseSeed::players(), turn, units_path);
    for (u16 p = 0; p < player_n; ++p) {
        const u32 n = count_for(units, p);
        std::printf("civ=%s units=%u\n", Helper_CivNm::nm(st, seats[p].m_civ_index), n);
        if (!list) {
            continue;
        }
        for (u32 i = 0; i < units.m_n; ++i) {
            const UnitAddStruct& u = units.m_recs[i].m_u;
            if (u.m_player_idx != p) {
                continue;
            }
            std::printf("  idx=%u %s", units.m_recs[i].m_key, unit_nm(st, u.m_unit_typ_idx));
            print_pos(u.m_x, u.m_y);
            std::printf("\n");
        }
    }
    free_units(&units);
    delete[] seats;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
