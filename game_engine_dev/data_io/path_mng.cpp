//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "path_mng.h"

//================================================================================================================================
//=> - Private helper functions -
//================================================================================================================================

static bool does_file_exist (const std::string& path) {
    std::ifstream file(path.c_str());
    return file.good();
}

//================================================================================================================================
//=> - PathMng implementation -
//================================================================================================================================

PathMng::PathMng (const std::string& path_offset) :
    m_path_offset(path_offset) {
    build_paths();
    validate_paths_or_exit();
}

const std::string& PathMng::get_path_to_techs () const {
    return m_path_techs;
}

const std::string& PathMng::get_path_to_resources () const {
    return m_path_resources;
}

const std::string& PathMng::get_path_to_wonders_small () const {
    return m_path_wonders_small;
}

const std::string& PathMng::get_path_to_city_flags () const {
    return m_path_city_flags;
}

const std::string& PathMng::get_path_to_unit_types () const {
    return m_path_unit_types;
}

const std::string& PathMng::get_path_to_wonders () const {
    return m_path_wonders;
}

const std::string& PathMng::get_path_to_governments () const {
    return m_path_governments;
}

const std::string& PathMng::get_path_to_callbacks () const {
    return m_path_callbacks;
}

const std::string& PathMng::get_path_to_civ_traits () const {
    return m_path_civ_traits;
}

const std::string& PathMng::get_path_to_units () const {
    return m_path_units;
}

const std::string& PathMng::get_path_to_civs () const {
    return m_path_civs;
}

const std::string& PathMng::get_path_to_buildings () const {
    return m_path_buildings;
}

const std::string& PathMng::get_path_to_effects () const {
    return m_path_effects;
}

void PathMng::build_paths () {
    m_path_techs = m_path_offset + "/game_config.techs";
    m_path_resources = m_path_offset + "/game_config.resources";
    m_path_wonders_small = m_path_offset + "/game_config.wonders_small";
    m_path_city_flags = m_path_offset + "/game_config.city_flags";
    m_path_unit_types = m_path_offset + "/game_config.unit_types";
    m_path_wonders = m_path_offset + "/game_config.wonders";
    m_path_governments = m_path_offset + "/game_config.governments";
    m_path_callbacks = m_path_offset + "/game_config.callbacks";
    m_path_civ_traits = m_path_offset + "/game_config.civ_traits";
    m_path_units = m_path_offset + "/game_config.units";
    m_path_civs = m_path_offset + "/game_config.civs";
    m_path_buildings = m_path_offset + "/game_config.buildings";
    m_path_effects = m_path_offset + "/game_config.effects";
}

void PathMng::validate_paths_or_exit () const {
    u32 error_count = 0;

    if (!does_file_exist(m_path_techs)) {
        printf("ERROR: Missing file: %s\n", m_path_techs.c_str());
        ++error_count;
    }
    if (!does_file_exist(m_path_resources)) {
        printf("ERROR: Missing file: %s\n", m_path_resources.c_str());
        ++error_count;
    }
    if (!does_file_exist(m_path_wonders_small)) {
        printf("ERROR: Missing file: %s\n", m_path_wonders_small.c_str());
        ++error_count;
    }
    if (!does_file_exist(m_path_city_flags)) {
        printf("ERROR: Missing file: %s\n", m_path_city_flags.c_str());
        ++error_count;
    }
    if (!does_file_exist(m_path_unit_types)) {
        printf("ERROR: Missing file: %s\n", m_path_unit_types.c_str());
        ++error_count;
    }
    if (!does_file_exist(m_path_wonders)) {
        printf("ERROR: Missing file: %s\n", m_path_wonders.c_str());
        ++error_count;
    }
    if (!does_file_exist(m_path_governments)) {
        printf("ERROR: Missing file: %s\n", m_path_governments.c_str());
        ++error_count;
    }
    if (!does_file_exist(m_path_callbacks)) {
        printf("ERROR: Missing file: %s\n", m_path_callbacks.c_str());
        ++error_count;
    }
    if (!does_file_exist(m_path_civ_traits)) {
        printf("ERROR: Missing file: %s\n", m_path_civ_traits.c_str());
        ++error_count;
    }
    if (!does_file_exist(m_path_units)) {
        printf("ERROR: Missing file: %s\n", m_path_units.c_str());
        ++error_count;
    }
    if (!does_file_exist(m_path_civs)) {
        printf("ERROR: Missing file: %s\n", m_path_civs.c_str());
        ++error_count;
    }
    if (!does_file_exist(m_path_buildings)) {
        printf("ERROR: Missing file: %s\n", m_path_buildings.c_str());
        ++error_count;
    }
    if (!does_file_exist(m_path_effects)) {
        printf("ERROR: Missing file: %s\n", m_path_effects.c_str());
        ++error_count;
    }

    if (error_count > 0) {
        printf("ERROR: Path validation failed (%u missing files)\n", error_count);
        std::exit(1);
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

