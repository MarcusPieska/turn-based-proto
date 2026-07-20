//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "game_io.h"
#include "bit_array.h"
#include "city.h"
#include "city_array.h"
#include "game_array_simple.h"
#include "game_state.h"
#include "general_bit_bank.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u32 k_tiles_magic = 0x534c4954u;
static const u32 k_units_magic = 0x53494e55u;
static const u32 k_cities_magic = 0x53495443u;
static const u32 k_players_magic = 0x53524c50u;
static const u32 k_io_ver = 2u;
static const u8 k_bits_per_byte = 8u;
static const u32 k_bank_batches_per_page = 256u;

//================================================================================================================================
//=> - Helpers -
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

static u32 bank_page_byte_n (u16 batch_size) {
    const u32 page_bit_n = static_cast<u32>(batch_size) * k_bank_batches_per_page;
    return (page_bit_n + static_cast<u32>(k_bits_per_byte - 1u)) / static_cast<u32>(k_bits_per_byte);
}

bool GameIo::wr_bit_bank (void* fp_raw, const GeneralBitBank* bank) {
    std::FILE* fp = static_cast<std::FILE*>(fp_raw);
    if (fp == nullptr) {
        return false;
    }
    if (bank == nullptr) {
        const u16 z16 = 0;
        const u8 z8 = 0;
        return std::fwrite(&z16, sizeof(z16), 1, fp) == 1
            && std::fwrite(&z16, sizeof(z16), 1, fp) == 1
            && std::fwrite(&z8, sizeof(z8), 1, fp) == 1;
    }
    if (std::fwrite(&bank->m_batch_size, sizeof(bank->m_batch_size), 1, fp) != 1
        || std::fwrite(&bank->m_claimed_batch_count, sizeof(bank->m_claimed_batch_count), 1, fp) != 1
        || std::fwrite(&bank->m_allocated_page_count, sizeof(bank->m_allocated_page_count), 1, fp) != 1) {
        return false;
    }
    const u32 page_bytes = bank_page_byte_n(bank->m_batch_size);
    for (u8 p = 0; p < bank->m_allocated_page_count; ++p) {
        if (bank->m_pages[p] == nullptr) {
            return false;
        }
        if (std::fwrite(bank->m_pages[p], 1, page_bytes, fp) != page_bytes) {
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - GameIo -
//================================================================================================================================

bool GameIo::save_map_tiles (cstr path, const GameArraySimple& map) {
    if (path == nullptr) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = map.tile_n();
    if (w == 0 || h == 0 || n == 0 || map.m_tiles == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    const u32 magic = k_tiles_magic;
    const u32 ver = k_io_ver;
    if (std::fwrite(&magic, sizeof(magic), 1, fp) != 1
        || std::fwrite(&ver, sizeof(ver), 1, fp) != 1
        || std::fwrite(&w, sizeof(w), 1, fp) != 1
        || std::fwrite(&h, sizeof(h), 1, fp) != 1) 
    {
        std::fclose(fp);
        return false;
    }
    if (std::fwrite(map.m_tiles, sizeof(GameTileSimple), n, fp) != n) {
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    return true;
}

bool GameIo::save_units (cstr path, const UnitAddVector& units) {
    if (path == nullptr) {
        return false;
    }
    const u32 scan_n = static_cast<u32>(units.get_head_unit_add_idx());
    u32 live_n = 0;
    for (u32 idx = 0; idx < scan_n; ++idx) {
        if (units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx))) != nullptr) {
            live_n = live_n + 1u;
        }
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    const u32 magic = k_units_magic;
    const u32 ver = k_io_ver;
    if (std::fwrite(&magic, sizeof(magic), 1, fp) != 1
        || std::fwrite(&ver, sizeof(ver), 1, fp) != 1
        || std::fwrite(&live_n, sizeof(live_n), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddStruct* u = units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr) {
            continue;
        }
        const u16 key = static_cast<u16>(idx);
        if (std::fwrite(&key, sizeof(key), 1, fp) != 1 || std::fwrite(u, sizeof(UnitAddStruct), 1, fp) != 1) {
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    return true;
}

bool GameIo::save_cities (cstr path, const CityArray& cities) {
    if (path == nullptr) {
        return false;
    }
    const u16 cn = cities.get_city_count();
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    const u32 magic = k_cities_magic;
    const u32 ver = k_io_ver;
    if (std::fwrite(&magic, sizeof(magic), 1, fp) != 1
        || std::fwrite(&ver, sizeof(ver), 1, fp) != 1
        || std::fwrite(&cn, sizeof(cn), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    for (u16 i = 0; i < cn; ++i) {
        const City* city = cities.get_city(i);
        CityDumpRec rec = {};
        rec.m_idx = i;
        if (city != nullptr) {
            rec.m_owner = city->get_owner();
            rec.m_x = city->get_x();
            rec.m_y = city->get_y();
            rec.m_pop = city->get_current_population();
            rec.m_food = city->get_current_food_store();
            rec.m_prod = city->get_current_production_store();
            rec.m_culture = city->get_current_culture();
        } else {
            rec.m_owner = U16_KEY_NULL;
            rec.m_x = U16_KEY_NULL;
            rec.m_y = U16_KEY_NULL;
        }
        if (std::fwrite(&rec, sizeof(rec), 1, fp) != 1) {
            std::fclose(fp);
            return false;
        }
    }
    if (!wr_bit_bank(fp, cities.m_flag_bank) || !wr_bit_bank(fp, cities.m_res_bank) || !wr_bit_bank(fp, cities.m_bld_bank)) {
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    return true;
}

bool GameIo::wr_bit_cl (void* fp_raw, const BitArrayCL* ba) {
    std::FILE* fp = static_cast<std::FILE*>(fp_raw);
    if (fp == nullptr) {
        return false;
    }
    const u32 n = (ba != nullptr) ? ba->get_count() : 0u;
    if (std::fwrite(&n, sizeof(n), 1, fp) != 1) {
        return false;
    }
    if (n == 0) {
        return true;
    }
    const u32 byte_n = (n + static_cast<u32>(k_bits_per_byte - 1u)) / static_cast<u32>(k_bits_per_byte);
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

bool GameIo::save_players (cstr path, const GameState& state) {
    if (path == nullptr || state.m_player_states == nullptr || state.m_player_n == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    const u32 magic = k_players_magic;
    const u32 ver = k_io_ver;
    const u16 player_n = state.m_player_n;
    if (std::fwrite(&magic, sizeof(magic), 1, fp) != 1
        || std::fwrite(&ver, sizeof(ver), 1, fp) != 1
        || std::fwrite(&player_n, sizeof(player_n), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    for (u16 p = 0; p < player_n; ++p) {
        const PlayerState& ps = state.m_player_states[p];
        if (std::fwrite(&ps.m_civ_index, sizeof(ps.m_civ_index), 1, fp) != 1
            || std::fwrite(&ps.m_research_spending_perc, sizeof(ps.m_research_spending_perc), 1, fp) != 1
            || std::fwrite(&ps.m_current_research_target_idx, sizeof(ps.m_current_research_target_idx), 1, fp) != 1
            || std::fwrite(&ps.m_commerce, sizeof(ps.m_commerce), 1, fp) != 1
            || std::fwrite(&ps.m_research, sizeof(ps.m_research), 1, fp) != 1
            || std::fwrite(&ps.m_commerce_from_turn, sizeof(ps.m_commerce_from_turn), 1, fp) != 1) {
            std::fclose(fp);
            return false;
        }
        if (!wr_bit_cl(fp, ps.m_techs_researched)) {
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
