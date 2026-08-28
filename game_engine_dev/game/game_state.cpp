//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <new>

#include "game_state.h"
#include "unit_movement_mng.h"
#include "player_ledger.h"
#include "city.h"
#include "tile_yields.h"
#include "tile_working.h"
#include "tile_imp_helper.h"
#include "worker_guidance.h"
#include "city_tile_manager.h"
#include "city_border.h"
#include "city_connector.h"

//================================================================================================================================
//=> - GameState -
//================================================================================================================================

void GameState::clear () {
    if (m_player_states != nullptr) {
        for (u16 i = 0; i < m_player_n; ++i) {
            delete[] m_player_states[i].m_small_wonder_city;
            m_player_states[i].m_small_wonder_city = nullptr;
            
            delete m_player_states[i].m_explored_overlay;
            m_player_states[i].m_explored_overlay = nullptr;
           
            delete m_player_states[i].m_techs_researched;
            m_player_states[i].m_techs_researched = nullptr;
        }
        delete[] m_player_states;
        m_player_states = nullptr;
    }
    
    delete[] m_wonder_city;
    m_wonder_city = nullptr;
    
    m_wonder_count = 0;
    m_small_wonder_count = 0;
    m_civ_relations.reset(0);
    m_combat_mods.clear();
    m_map.clear();

    // These are the arrays over which the game loop iterates, and does most of its work.
    m_units.~UnitAddVector();
    new (&m_units) UnitAddVector();
    m_cities.~CityArray();
    new (&m_cities) CityArray();

    // These are used for fast but suboptimal pathing, and should be used most of the time,
    m_sector_rt.~SectorNetworkRouter();
    new (&m_sector_rt) SectorNetworkRouter();
    m_sector_net.~SectorNetwork();
    new (&m_sector_net) SectorNetwork();
    m_city_net.~CityNetwork();
    new (&m_city_net) CityNetwork();
    m_ai_help.clr();

    // Some static helper classes need access to the game state to be able to do anything useful.
    UnitMovementMng::bind_state(nullptr);
    PlayerLedger::bind_state(nullptr);
    TileYields::bind_map(nullptr);
    TileWorking::bind_map(nullptr);
    WorkerGuidance::bind_map(nullptr);
    WorkerGuidance::bind_statics(nullptr);
    TileImpHelper::bind_statics(nullptr);
    CityTileManager::bind_cities(nullptr);
    CityBorder::bind_map(nullptr);
    City::bind_wonder_cities(nullptr);
    City::bind_player_states(nullptr, 0);
    m_statics = nullptr;
    m_current_turn = 0;
    m_turn_limit = 1000;
    m_player_n = 0;
    m_players_remaining = 0;
}

bool GameState::city_net_on_found (u16 city_idx) {
    if (!m_city_net.is_valid()) {
        if (!m_city_net.begin(m_cities, m_map)) {
            return false;
        }
    }
    if (!m_city_net.add(city_idx)) {
        return false;
    }
    CityConnector::on_city_net_changed(*this, city_idx);
    return true;
}

bool GameState::spawn (u16 x, u16 y, u16 player_idx, const u16* typ_idxs, u16 typ_n) {
    if (typ_idxs == nullptr || typ_n == 0) {
        return false;
    }
    for (u16 i = 0; i < typ_n; ++i) {
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(*this, x, y, player_idx, typ_idxs[i], &key)) {
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
