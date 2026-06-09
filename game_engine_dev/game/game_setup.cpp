//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "game_setup.h"

#include "map_loader.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const u16 k_latt_div = 10;

static void latt_for_map (u16 w, u16 h, u16* rows, u16* cols) {
    u16 r = h / k_latt_div;
    u16 c = w / k_latt_div;
    if (r == 0) {
        r = 1;
    }
    if (c == 0) {
        c = 1;
    }
    *rows = r;
    *cols = c;
}

//================================================================================================================================
//=> - GameSetup -
//================================================================================================================================

GameSetup::GameSetup () {
    m_starts.n = 0;
}

GameSetup::~GameSetup () {
}

bool GameSetup::load_map (cstr path) {
    m_starts.n = 0;
    m_map.clear();
    return MapLoader::load_terrain_ppm(path, m_map);
}

bool GameSetup::set_player_count (u16 n) {
    m_starts.n = 0;
    if (m_map.data() == nullptr || m_map.width() == 0 || m_map.height() == 0) {
        return false;
    }
    if (n == 0 || n > SPG_MAX_PICK_PTS) {
        return false;
    }
    u16 latt_rows = 0;
    u16 latt_cols = 0;
    latt_for_map(m_map.width(), m_map.height(), &latt_rows, &latt_cols);
    StartingPointGeneratorParams par = {};
    par.map = &m_map;
    par.pick_n = n;
    par.latt_rows = latt_rows;
    par.latt_cols = latt_cols;
    StartingPointGenerator gen(par);
    if (!gen.generate()) {
        return false;
    }
    m_starts = gen.picks_coords();
    u32 expect = static_cast<u32>(n);
    if (gen.candidate_count() < expect) {
        expect = gen.candidate_count();
    }
    if (m_starts.n != expect) {
        m_starts.n = 0;
        return false;
    }
    if (!gen.picks_are_start_land()) {
        m_starts.n = 0;
        return false;
    }
    return true;
}

const MapTerrainData& GameSetup::get_map () const {
    return m_map;
}

const SpgPickCoords& GameSetup::get_starts () const {
    return m_starts;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
