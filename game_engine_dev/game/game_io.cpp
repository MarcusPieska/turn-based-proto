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

bool GameIo::save_players (cstr path, const PlayerState* seats, u16 player_n) {
    if (path == nullptr || seats == nullptr || player_n == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    const u32 magic = k_players_magic;
    const u32 ver = k_io_ver;
    if (std::fwrite(&magic, sizeof(magic), 1, fp) != 1
        || std::fwrite(&ver, sizeof(ver), 1, fp) != 1
        || std::fwrite(&player_n, sizeof(player_n), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    for (u16 p = 0; p < player_n; ++p) {
        const PlayerState& ps = seats[p];
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

bool GameIo::save_players (cstr path, const GameState& state) {
    return save_players(path, state.m_player_states, state.m_player_n);
}

//================================================================================================================================
//=> - Load helpers -
//================================================================================================================================

void GameIo::clr_units (UnitAddVector& units) {
    for (u16 i = 0; i < UnitAddVector::MAX_PAGES; ++i) {
        delete[] units.m_pages[i];
        units.m_pages[i] = nullptr;
        delete[] units.m_exists_pages[i];
        units.m_exists_pages[i] = nullptr;
        delete[] units.m_recycled_pages[i];
        units.m_recycled_pages[i] = nullptr;
    }
    units.m_unit_add_count = 0;
    units.m_head_unit_add_idx = 0;
    units.m_page_count = 0;
    units.m_recycled_unit_add_count = 0;
    units.m_recycled_page_count = 0;
}

void GameIo::clr_cities (CityArray& cities) {
    for (u16 i = 0; i < CityArray::MAX_PAGES; ++i) {
        delete[] cities.m_pages[i];
        cities.m_pages[i] = nullptr;
    }
    cities.m_city_count = 0;
    cities.m_page_count = 0;
    cities.clear_banks();
}

bool GameIo::rd_bit_bank (void* fp_raw, GeneralBitBank** out) {
    std::FILE* fp = static_cast<std::FILE*>(fp_raw);
    if (fp == nullptr || out == nullptr) {
        return false;
    }
    u16 batch_size = 0;
    u16 claimed = 0;
    u8 page_n = 0;
    if (std::fread(&batch_size, sizeof(batch_size), 1, fp) != 1
        || std::fread(&claimed, sizeof(claimed), 1, fp) != 1
        || std::fread(&page_n, sizeof(page_n), 1, fp) != 1) {
        return false;
    }
    delete *out;
    *out = nullptr;
    if (batch_size == 0 && claimed == 0 && page_n == 0) {
        return true;
    }
    if (batch_size == 0) {
        return false;
    }
    GeneralBitBank* bank = new GeneralBitBank(batch_size);
    bank->m_claimed_batch_count = claimed;
    bank->m_allocated_page_count = 0;
    const u32 page_bytes = bank_page_byte_n(batch_size);
    for (u8 p = 0; p < page_n; ++p) {
        bank->m_pages[p] = new u8[page_bytes];
        if (std::fread(bank->m_pages[p], 1, page_bytes, fp) != page_bytes) {
            delete bank;
            return false;
        }
        bank->m_allocated_page_count = static_cast<u8>(bank->m_allocated_page_count + 1u);
    }
    *out = bank;
    return true;
}

bool GameIo::rd_bit_cl (void* fp_raw, BitArrayCL** out) {
    std::FILE* fp = static_cast<std::FILE*>(fp_raw);
    if (fp == nullptr || out == nullptr) {
        return false;
    }
    delete *out;
    *out = nullptr;
    u32 n = 0;
    if (std::fread(&n, sizeof(n), 1, fp) != 1) {
        return false;
    }
    if (n == 0) {
        return true;
    }
    const u32 byte_n = (n + static_cast<u32>(k_bits_per_byte - 1u)) / static_cast<u32>(k_bits_per_byte);
    u8* buf = new u8[byte_n];
    if (std::fread(buf, 1, byte_n, fp) != byte_n) {
        delete[] buf;
        return false;
    }
    BitArrayCL* ba = new BitArrayCL(n);
    for (u32 i = 0; i < n; ++i) {
        const u8 bit = static_cast<u8>((buf[i / static_cast<u32>(k_bits_per_byte)]
            >> (i % static_cast<u32>(k_bits_per_byte))) & 1u);
        if (bit != 0) {
            ba->set_bit(i);
        }
    }
    delete[] buf;
    *out = ba;
    return true;
}

//================================================================================================================================
//=> - Load -
//================================================================================================================================

bool GameIo::load_map_tiles (cstr path, GameArraySimple& map) {
    if (path == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u16 w = 0;
    u16 h = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&w, sizeof(w), 1, fp) != 1
        || std::fread(&h, sizeof(h), 1, fp) != 1
        || magic != k_tiles_magic
        || ver < k_io_ver
        || w == 0
        || h == 0) {
        std::fclose(fp);
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    map.clear();
    map.m_w = w;
    map.m_h = h;
    map.m_tiles = new GameTileSimple[n];
    if (std::fread(map.m_tiles, sizeof(GameTileSimple), n, fp) != n) {
        map.clear();
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    return true;
}

bool GameIo::load_units (cstr path, UnitAddVector& units) {
    if (path == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u32 live_n = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&live_n, sizeof(live_n), 1, fp) != 1
        || magic != k_units_magic
        || ver < k_io_ver) {
        std::fclose(fp);
        return false;
    }
    clr_units(units);
    u16 max_key = 0;
    for (u32 i = 0; i < live_n; ++i) {
        u16 key = 0;
        UnitAddStruct rec = {};
        if (std::fread(&key, sizeof(key), 1, fp) != 1
            || std::fread(&rec, sizeof(rec), 1, fp) != 1) {
            clr_units(units);
            std::fclose(fp);
            return false;
        }
        const u16 page = static_cast<u16>(key >> 8);
        const u16 slot = static_cast<u16>(key & 0xFFu);
        if (page >= UnitAddVector::MAX_PAGES) {
            clr_units(units);
            std::fclose(fp);
            return false;
        }
        if (units.m_pages[page] == nullptr) {
            units.m_pages[page] = new UnitAddStruct[UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE]();
            units.m_exists_pages[page] = new u8[UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE]();
            units.m_page_count = static_cast<u16>(units.m_page_count + 1u);
        }
        units.m_pages[page][slot] = rec;
        units.m_exists_pages[page][slot] = 1;
        units.m_unit_add_count = static_cast<u16>(units.m_unit_add_count + 1u);
        if (key >= max_key) {
            max_key = static_cast<u16>(key + 1u);
        }
    }
    units.m_head_unit_add_idx = max_key;
    std::fclose(fp);
    return true;
}

bool GameIo::load_cities (cstr path, CityArray& cities) {
    if (path == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u16 cn = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&cn, sizeof(cn), 1, fp) != 1
        || magic != k_cities_magic
        || ver < k_io_ver) {
        std::fclose(fp);
        return false;
    }
    clr_cities(cities);
    for (u16 i = 0; i < cn; ++i) {
        CityDumpRec rec = {};
        if (std::fread(&rec, sizeof(rec), 1, fp) != 1 || rec.m_idx != i) {
            clr_cities(cities);
            std::fclose(fp);
            return false;
        }
        const u16 page = static_cast<u16>(i >> 8);
        const u16 slot = static_cast<u16>(i & 0xFFu);
        if (cities.m_pages[page] == nullptr) {
            cities.m_pages[page] = new City[CityArray::CITIES_PER_PAGE];
            cities.m_page_count = static_cast<u16>(cities.m_page_count + 1u);
        }
        City& city = cities.m_pages[page][slot];
        if (rec.m_owner != U16_KEY_NULL) {
            city.init(rec.m_owner, rec.m_x, rec.m_y);
            city.set_population(rec.m_pop);
            city.set_culture(rec.m_culture);
            city.m_accumulated_food = static_cast<i8>(rec.m_food);
            city.m_accumulated_production = rec.m_prod;
        }
        cities.m_city_count = static_cast<u16>(cities.m_city_count + 1u);
    }
    if (!rd_bit_bank(fp, &cities.m_flag_bank)
        || !rd_bit_bank(fp, &cities.m_res_bank)
        || !rd_bit_bank(fp, &cities.m_bld_bank)) {
        clr_cities(cities);
        std::fclose(fp);
        return false;
    }
    City::bind_banks(cities.m_flag_bank, cities.m_res_bank, cities.m_bld_bank);
    std::fclose(fp);
    return true;
}

bool GameIo::load_players (cstr path, PlayerState*& seats, u16& player_n) {
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
        || magic != k_players_magic
        || ver < k_io_ver
        || n == 0) {
        std::fclose(fp);
        return false;
    }
    if (seats != nullptr) {
        for (u16 i = 0; i < player_n; ++i) {
            delete seats[i].m_techs_researched;
            seats[i].m_techs_researched = nullptr;
        }
        delete[] seats;
        seats = nullptr;
        player_n = 0;
    }
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
            for (u16 j = 0; j <= p; ++j) {
                delete seats[j].m_techs_researched;
                seats[j].m_techs_researched = nullptr;
            }
            delete[] seats;
            seats = nullptr;
            player_n = 0;
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    return true;
}

bool GameIo::load_players (cstr path, GameState& state) {
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
