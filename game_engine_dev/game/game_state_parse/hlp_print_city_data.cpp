//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "hlp_print_city_data.h"
#include "hlp_civ_nm.h"
#include "hlp_parse_seed.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

static const u32 k_players_magic = 0x53524c50u;
static const u32 k_cities_magic = 0x53495443u;
static const u32 k_io_ver_min = 2u;
static const u8 k_bits_per_byte = 8u;
static const u32 k_bank_batches_per_page = 256u;

static const char* G_RT_LIB_A = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA_A = "../../";
static const char* G_RT_LIB_B = "../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA_B = "../";

//================================================================================================================================
//=> - Dump blobs -
//================================================================================================================================

struct CityDumpRec {
    u16 m_idx;
    u16 m_owner;
    u16 m_x;
    u16 m_y;
    u16 m_pop;
    u16 m_food;
    u16 m_prod;
    u16 m_culture;
};

struct SeatCiv {
    u16 m_civ_index;
};

struct BankDump {
    u16 m_batch_size;
    u16 m_claimed;
    u8 m_page_n;
    u8** m_pages;
};

struct CitiesDump {
    CityDumpRec* m_recs;
    u16 m_cn;
    BankDump m_bld;
};

//================================================================================================================================
//=> - Bank helpers -
//================================================================================================================================

static u32 bank_page_byte_n (u16 batch_size) {
    const u32 page_bit_n = static_cast<u32>(batch_size) * k_bank_batches_per_page;
    return (page_bit_n + static_cast<u32>(k_bits_per_byte - 1u)) / static_cast<u32>(k_bits_per_byte);
}

static void free_bank (BankDump* bank) {
    if (bank == nullptr) {
        return;
    }
    if (bank->m_pages != nullptr) {
        for (u8 p = 0; p < bank->m_page_n; ++p) {
            delete[] bank->m_pages[p];
            bank->m_pages[p] = nullptr;
        }
        delete[] bank->m_pages;
        bank->m_pages = nullptr;
    }
    bank->m_page_n = 0;
    bank->m_batch_size = 0;
    bank->m_claimed = 0;
}

static bool rd_bank (std::FILE* fp, BankDump* out) {
    if (fp == nullptr || out == nullptr) {
        return false;
    }
    out->m_batch_size = 0;
    out->m_claimed = 0;
    out->m_page_n = 0;
    out->m_pages = nullptr;
    if (std::fread(&out->m_batch_size, sizeof(out->m_batch_size), 1, fp) != 1
        || std::fread(&out->m_claimed, sizeof(out->m_claimed), 1, fp) != 1
        || std::fread(&out->m_page_n, sizeof(out->m_page_n), 1, fp) != 1) {
        return false;
    }
    if (out->m_page_n == 0) {
        return true;
    }
    const u32 page_bytes = bank_page_byte_n(out->m_batch_size);
    out->m_pages = new u8*[out->m_page_n];
    for (u8 p = 0; p < out->m_page_n; ++p) {
        out->m_pages[p] = nullptr;
    }
    for (u8 p = 0; p < out->m_page_n; ++p) {
        out->m_pages[p] = new u8[page_bytes];
        if (std::fread(out->m_pages[p], 1, page_bytes, fp) != page_bytes) {
            free_bank(out);
            return false;
        }
    }
    return true;
}

static bool skip_bank (std::FILE* fp) {
    BankDump tmp = {};
    if (!rd_bank(fp, &tmp)) {
        return false;
    }
    free_bank(&tmp);
    return true;
}

static bool bank_on (const BankDump& bank, u16 city_idx, u16 flag_idx) {
    if (bank.m_pages == nullptr || city_idx >= bank.m_claimed || flag_idx >= bank.m_batch_size) {
        return false;
    }
    const u16 page_idx = static_cast<u16>(city_idx / static_cast<u16>(k_bank_batches_per_page));
    if (page_idx >= static_cast<u16>(bank.m_page_n) || bank.m_pages[page_idx] == nullptr) {
        return false;
    }
    const u32 batch_in_page = static_cast<u32>(city_idx % static_cast<u16>(k_bank_batches_per_page));
    const u32 bit_offset = (batch_in_page * static_cast<u32>(bank.m_batch_size)) + static_cast<u32>(flag_idx);
    const u32 byte_offset = bit_offset >> 3;
    const u8 bit_idx = static_cast<u8>(bit_offset & 0x7u);
    return (bank.m_pages[page_idx][byte_offset] & static_cast<u8>(1u << bit_idx)) != 0;
}

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

static void free_cities (CitiesDump* dump) {
    if (dump == nullptr) {
        return;
    }
    delete[] dump->m_recs;
    dump->m_recs = nullptr;
    dump->m_cn = 0;
    free_bank(&dump->m_bld);
}

static bool load_cities (cstr path, CitiesDump* out) {
    if (path == nullptr || out == nullptr) {
        return false;
    }
    out->m_recs = nullptr;
    out->m_cn = 0;
    out->m_bld = {};
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u16 cn = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&cn, sizeof(cn), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    if (magic != k_cities_magic || ver < k_io_ver_min) {
        std::fclose(fp);
        return false;
    }
    CityDumpRec* recs = new CityDumpRec[cn == 0 ? 1u : cn];
    for (u16 i = 0; i < cn; ++i) {
        if (std::fread(&recs[i], sizeof(recs[i]), 1, fp) != 1) {
            delete[] recs;
            std::fclose(fp);
            return false;
        }
    }
    if (!skip_bank(fp) || !skip_bank(fp) || !rd_bank(fp, &out->m_bld)) {
        delete[] recs;
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    out->m_recs = recs;
    out->m_cn = cn;
    return true;
}

static u32 count_blds (const BankDump& bank, u16 city_idx, u16 bld_n) {
    u32 n = 0;
    for (u16 i = 0; i < bld_n; ++i) {
        if (bank_on(bank, city_idx, i)) {
            n = n + 1u;
        }
    }
    return n;
}

//================================================================================================================================
//=> - Helper_PrintCityData -
//================================================================================================================================

bool Helper_PrintCityData::run (u32 turn) {
    if (!Helper_ParseSeed::load()) {
        std::printf("Helper_PrintCityData: parse_seed load failed\n");
        return false;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB_A, G_RT_DATA_A) && !loader.load(G_RT_LIB_B, G_RT_DATA_B)) {
        std::printf("Helper_PrintCityData: statics load failed\n");
        return false;
    }
    const RuntimeStatics& st = loader.statics();
    char players_path[384];
    char cities_path[384];
    if (!Helper_ParseSeed::players_path(turn, players_path, sizeof(players_path))
        || !Helper_ParseSeed::cities_path(turn, cities_path, sizeof(cities_path))) {
        return false;
    }
    SeatCiv* seats = nullptr;
    u16 player_n = 0;
    if (!load_seats(players_path, &seats, &player_n)) {
        std::printf("Helper_PrintCityData: players load failed: %s\n", players_path);
        return false;
    }
    CitiesDump cities = {};
    if (!load_cities(cities_path, &cities)) {
        std::printf("Helper_PrintCityData: cities load failed: %s\n", cities_path);
        delete[] seats;
        return false;
    }
    const u16 bld_n = st.building().get_item_count();
    std::printf("seed=%u players=%u turn=%04u path=%s\n",
        Helper_ParseSeed::seed(), Helper_ParseSeed::players(), turn, cities_path);
    for (u16 p = 0; p < player_n; ++p) {
        std::printf("civ=%s\n", Helper_CivNm::nm(st, seats[p].m_civ_index));
        for (u16 i = 0; i < cities.m_cn; ++i) {
            const CityDumpRec& rec = cities.m_recs[i];
            if (rec.m_owner != p) {
                continue;
            }
            const u32 bn = count_blds(cities.m_bld, rec.m_idx, bld_n);
            std::printf(" city=%u blds=%u culture=%u pop=%u\n", rec.m_idx, bn, rec.m_culture, rec.m_pop);
        }
    }
    free_cities(&cities);
    delete[] seats;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
