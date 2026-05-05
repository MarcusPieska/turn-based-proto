//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef STATIC_PARSING_MANAGER_H
#define STATIC_PARSING_MANAGER_H

#include <vector>

#include "path_mng.h"
#include "data_reader.h"
#include "data_parser_base.h"

#include "building_parser.h"
#include "city_flag_static_data.h"
#include "civ_static_data.h"
#include "civ_trait_static_data.h"
#include "effect_static_data.h"
#include "government_static_data.h"
#include "resource_static_data.h"
#include "small_wonder_static_data.h"
#include "tech_static_data.h"
#include "unit_static_data.h"
#include "unit_type_static_data.h"
#include "wonder_static_data.h"

//================================================================================================================================
//=> - StaticParsingManager class -
//================================================================================================================================

class StaticParsingManager {
public:
    explicit StaticParsingManager (const std::string& path_offset);
    ~StaticParsingManager ();

    const BuildingStaticDataStruct* get_building_data () const;
    u16 get_building_count () const;

    const CityFlagStaticDataStruct* get_city_flag_data () const;
    u16 get_city_flag_count () const;

    const std::vector<RawItem>& get_civ_raw_items () const;
    u16 get_civ_count () const;

    const std::vector<RawItem>& get_civ_trait_raw_items () const;
    u16 get_civ_trait_count () const;

    const std::vector<RawItem>& get_effect_raw_items () const;
    u16 get_effect_count () const;

    const std::vector<RawItem>& get_government_raw_items () const;
    u16 get_government_count () const;

    const ResourceStaticDataStruct* get_resource_data () const;
    u16 get_resource_count () const;

    const SmallWonderStaticDataStruct* get_small_wonder_data () const;
    u16 get_small_wonder_count () const;

    const TechStaticDataStruct* get_tech_data () const;
    u16 get_tech_count () const;

    const UnitStaticDataStruct* get_unit_data () const;
    u16 get_unit_count () const;

    const std::vector<RawItem>& get_unit_type_raw_items () const;
    u16 get_unit_type_count () const;

    const WonderStaticDataStruct* get_wonder_data () const;
    u16 get_wonder_count () const;

private:
    StaticParsingManager () = delete;
    StaticParsingManager (const StaticParsingManager& other) = delete;
    StaticParsingManager (StaticParsingManager&& other) = delete;

    void build_name_to_idx_callbacks ();
    void parse_supported_data ();

    static u16 safe_size_to_u16 (size_t value);

    PathMng m_paths;

    DataReader m_building_reader;
    DataReader m_city_flag_reader;
    DataReader m_civ_reader;
    DataReader m_civ_trait_reader;
    DataReader m_effect_reader;
    DataReader m_government_reader;
    DataReader m_resource_reader;
    DataReader m_small_wonder_reader;
    DataReader m_tech_reader;
    DataReader m_unit_reader;
    DataReader m_unit_type_reader;
    DataReader m_wonder_reader;

    DataParserBase m_building_name_parser;
    DataParserBase m_city_flag_name_parser;
    DataParserBase m_civ_name_parser;
    DataParserBase m_civ_trait_name_parser;
    DataParserBase m_effect_name_parser;
    DataParserBase m_government_name_parser;
    DataParserBase m_resource_name_parser;
    DataParserBase m_small_wonder_name_parser;
    DataParserBase m_tech_name_parser;
    DataParserBase m_unit_name_parser;
    DataParserBase m_unit_type_name_parser;
    DataParserBase m_wonder_name_parser;

    NameToIdxCbs m_name_to_idx_cbs;

    BuildingStaticDataStruct* m_building_data;
    CityFlagStaticDataStruct* m_city_flag_data;
    CivStaticDataStruct* m_civ_data;
    CivTraitStaticDataStruct* m_civ_trait_data;
    EffectStaticDataStruct* m_effect_data;
    GovernmentStaticDataStruct* m_government_data;
    ResourceStaticDataStruct* m_resource_data;
    SmallWonderStaticDataStruct* m_small_wonder_data;
    TechStaticDataStruct* m_tech_data;
    UnitStaticDataStruct* m_unit_data;
    UnitTypeStaticDataStruct* m_unit_type_data;
    WonderStaticDataStruct* m_wonder_data;
};

#endif // STATIC_PARSING_MANAGER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

