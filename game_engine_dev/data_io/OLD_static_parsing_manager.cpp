//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>

#include "static_parsing_manager.h"

//================================================================================================================================
//=> - Private callback glue -
//================================================================================================================================

namespace {

const DataParserBase* g_building_name_parser = nullptr;
const DataParserBase* g_city_flag_name_parser = nullptr;
const DataParserBase* g_civ_name_parser = nullptr;
const DataParserBase* g_civ_trait_name_parser = nullptr;
const DataParserBase* g_effect_name_parser = nullptr;
const DataParserBase* g_government_name_parser = nullptr;
const DataParserBase* g_resource_name_parser = nullptr;
const DataParserBase* g_small_wonder_name_parser = nullptr;
const DataParserBase* g_tech_name_parser = nullptr;
const DataParserBase* g_unit_name_parser = nullptr;
const DataParserBase* g_unit_type_name_parser = nullptr;
const DataParserBase* g_wonder_name_parser = nullptr;

u16 cb_building_name_to_idx (cstr name) {
    return g_building_name_parser->name_to_idx(name);
}

u16 cb_city_flag_name_to_idx (cstr name) {
    return g_city_flag_name_parser->name_to_idx(name);
}

u16 cb_civ_name_to_idx (cstr name) {
    return g_civ_name_parser->name_to_idx(name);
}

u16 cb_civ_trait_name_to_idx (cstr name) {
    return g_civ_trait_name_parser->name_to_idx(name);
}

u16 cb_effect_name_to_idx (cstr name) {
    return g_effect_name_parser->name_to_idx(name);
}

u16 cb_government_name_to_idx (cstr name) {
    return g_government_name_parser->name_to_idx(name);
}

u16 cb_resource_name_to_idx (cstr name) {
    return g_resource_name_parser->name_to_idx(name);
}

u16 cb_small_wonder_name_to_idx (cstr name) {
    return g_small_wonder_name_parser->name_to_idx(name);
}

u16 cb_tech_name_to_idx (cstr name) {
    return g_tech_name_parser->name_to_idx(name);
}

u16 cb_unit_name_to_idx (cstr name) {
    return g_unit_name_parser->name_to_idx(name);
}

u16 cb_unit_type_name_to_idx (cstr name) {
    return g_unit_type_name_parser->name_to_idx(name);
}

u16 cb_wonder_name_to_idx (cstr name) {
    return g_wonder_name_parser->name_to_idx(name);
}

class CityFlagParser : public DataParserBase {
public:
    CityFlagParser (const std::vector<RawItem>& items, const NameToIdxCbs& map) :
        DataParserBase(items, map) {
    }

    CityFlagStaticDataStruct* parse_data_dependencies () {
        CityFlagStaticDataStruct* parsed_data = new CityFlagStaticDataStruct[m_item_count];
        for (u32 i = 0; i < m_item_count; ++i) {
            const std::vector<std::string> line_items = get_line_items(m_raw_items[i].raw_line);
            parsed_data[i].name = m_raw_items[i].name;
            parsed_data[i].reqs = parse_item_reqs(line_items, 1);
            parsed_data[i].effects = parse_item_effects(line_items, 2);
        }
        return parsed_data;
    }
};

} // namespace

//================================================================================================================================
//=> - StaticParsingManager implementation -
//================================================================================================================================

StaticParsingManager::StaticParsingManager (const std::string& path_offset) :
    m_paths(path_offset),

    m_building_reader(m_paths.get_path_to_buildings()),
    m_city_flag_reader(m_paths.get_path_to_city_flags()),
    m_civ_reader(m_paths.get_path_to_civs()),
    m_civ_trait_reader(m_paths.get_path_to_civ_traits()),
    m_effect_reader(m_paths.get_path_to_effects()),
    m_government_reader(m_paths.get_path_to_governments()),
    m_resource_reader(m_paths.get_path_to_resources()),
    m_small_wonder_reader(m_paths.get_path_to_small_wonders()),
    m_tech_reader(m_paths.get_path_to_techs()),
    m_unit_reader(m_paths.get_path_to_units()),
    m_unit_type_reader(m_paths.get_path_to_unit_types()),
    m_wonder_reader(m_paths.get_path_to_wonders()),

    m_building_name_parser(m_building_reader.get_raw_items(), NameToIdxCbs()),
    m_city_flag_name_parser(m_city_flag_reader.get_raw_items(), NameToIdxCbs()),
    m_civ_name_parser(m_civ_reader.get_raw_items(), NameToIdxCbs()),
    m_civ_trait_name_parser(m_civ_trait_reader.get_raw_items(), NameToIdxCbs()),
    m_effect_name_parser(m_effect_reader.get_raw_items(), NameToIdxCbs()),
    m_government_name_parser(m_government_reader.get_raw_items(), NameToIdxCbs()),
    m_resource_name_parser(m_resource_reader.get_raw_items(), NameToIdxCbs()),
    m_small_wonder_name_parser(m_small_wonder_reader.get_raw_items(), NameToIdxCbs()),
    m_tech_name_parser(m_tech_reader.get_raw_items(), NameToIdxCbs()),
    m_unit_name_parser(m_unit_reader.get_raw_items(), NameToIdxCbs()),
    m_unit_type_name_parser(m_unit_type_reader.get_raw_items(), NameToIdxCbs()),
    m_wonder_name_parser(m_wonder_reader.get_raw_items(), NameToIdxCbs()),

    m_name_to_idx_cbs(),

    m_tech_data(nullptr),
    m_resource_data(nullptr),
    m_city_flag_data(nullptr),
    m_building_data(nullptr),
    m_unit_data(nullptr),
    m_wonder_data(nullptr),
    m_small_wonder_data(nullptr) 
{
    build_name_to_idx_callbacks();
    parse_supported_data();
}

StaticParsingManager::~StaticParsingManager () {
}

const BuildingStaticDataStruct* StaticParsingManager::get_building_data () const {
    return m_building_data;
}

u16 StaticParsingManager::get_building_count () const {
    return safe_size_to_u16(m_building_reader.get_raw_items().size());
}

const CityFlagStaticDataStruct* StaticParsingManager::get_city_flag_data () const {
    return m_city_flag_data;
}

u16 StaticParsingManager::get_city_flag_count () const {
    return safe_size_to_u16(m_city_flag_reader.get_raw_items().size());
}

const std::vector<RawItem>& StaticParsingManager::get_civ_raw_items () const {
    return m_civ_reader.get_raw_items();
}

u16 StaticParsingManager::get_civ_count () const {
    return safe_size_to_u16(m_civ_reader.get_raw_items().size());
}

const std::vector<RawItem>& StaticParsingManager::get_civ_trait_raw_items () const {
    return m_civ_trait_reader.get_raw_items();
}

u16 StaticParsingManager::get_civ_trait_count () const {
    return safe_size_to_u16(m_civ_trait_reader.get_raw_items().size());
}

const std::vector<RawItem>& StaticParsingManager::get_effect_raw_items () const {
    return m_effect_reader.get_raw_items();
}

u16 StaticParsingManager::get_effect_count () const {
    return safe_size_to_u16(m_effect_reader.get_raw_items().size());
}

const std::vector<RawItem>& StaticParsingManager::get_government_raw_items () const {
    return m_government_reader.get_raw_items();
}

u16 StaticParsingManager::get_government_count () const {
    return safe_size_to_u16(m_government_reader.get_raw_items().size());
}

const ResourceStaticDataStruct* StaticParsingManager::get_resource_data () const {
    return m_resource_data;
}

u16 StaticParsingManager::get_resource_count () const {
    return safe_size_to_u16(m_resource_reader.get_raw_items().size());
}

const SmallWonderStaticDataStruct* StaticParsingManager::get_small_wonder_data () const {
    return m_small_wonder_data;
}

u16 StaticParsingManager::get_small_wonder_count () const {
    return safe_size_to_u16(m_small_wonder_reader.get_raw_items().size());
}

const TechStaticDataStruct* StaticParsingManager::get_tech_data () const {
    return m_tech_data;
}

u16 StaticParsingManager::get_tech_count () const {
    return safe_size_to_u16(m_tech_reader.get_raw_items().size());
}

const UnitStaticDataStruct* StaticParsingManager::get_unit_data () const {
    return m_unit_data;
}

u16 StaticParsingManager::get_unit_count () const {
    return safe_size_to_u16(m_unit_reader.get_raw_items().size());
}

const std::vector<RawItem>& StaticParsingManager::get_unit_type_raw_items () const {
    return m_unit_type_reader.get_raw_items();
}

u16 StaticParsingManager::get_unit_type_count () const {
    return safe_size_to_u16(m_unit_type_reader.get_raw_items().size());
}

const WonderStaticDataStruct* StaticParsingManager::get_wonder_data () const {
    return m_wonder_data;
}

u16 StaticParsingManager::get_wonder_count () const {
    return safe_size_to_u16(m_wonder_reader.get_raw_items().size());
}

void StaticParsingManager::build_name_to_idx_callbacks () {
    g_building_name_parser = &m_building_name_parser;
    g_city_flag_name_parser = &m_city_flag_name_parser;
    g_civ_name_parser = &m_civ_name_parser;
    g_civ_trait_name_parser = &m_civ_trait_name_parser;
    g_effect_name_parser = &m_effect_name_parser;
    g_government_name_parser = &m_government_name_parser;
    g_resource_name_parser = &m_resource_name_parser;
    g_small_wonder_name_parser = &m_small_wonder_name_parser;
    g_tech_name_parser = &m_tech_name_parser;
    g_unit_name_parser = &m_unit_name_parser;
    g_unit_type_name_parser = &m_unit_type_name_parser;
    g_wonder_name_parser = &m_wonder_name_parser;

    m_name_to_idx_cbs.building_name_to_idx = cb_building_name_to_idx;
    m_name_to_idx_cbs.city_flag_name_to_idx = cb_city_flag_name_to_idx;
    m_name_to_idx_cbs.civ_name_to_idx = cb_civ_name_to_idx;
    m_name_to_idx_cbs.civ_trait_name_to_idx = cb_civ_trait_name_to_idx;
    m_name_to_idx_cbs.effect_name_to_idx = cb_effect_name_to_idx;
    m_name_to_idx_cbs.government_name_to_idx = cb_government_name_to_idx;
    m_name_to_idx_cbs.resource_name_to_idx = cb_resource_name_to_idx;
    m_name_to_idx_cbs.small_wonder_name_to_idx = cb_small_wonder_name_to_idx;
    m_name_to_idx_cbs.tech_name_to_idx = cb_tech_name_to_idx;
    m_name_to_idx_cbs.unit_name_to_idx = cb_unit_name_to_idx;
    m_name_to_idx_cbs.unit_type_name_to_idx = cb_unit_type_name_to_idx;
    m_name_to_idx_cbs.wonder_name_to_idx = cb_wonder_name_to_idx;

    DataParserBase::set_item_effect_handler(&m_name_to_idx_cbs, &m_effect_reader.get_raw_items());
}

void StaticParsingManager::parse_supported_data () {
    BuildingParser building_parser(m_building_reader.get_raw_items(), m_name_to_idx_cbs);
    CityFlagParser city_flag_parser(m_city_flag_reader.get_raw_items(), m_name_to_idx_cbs);
    CivParser civ_parser(m_civ_reader.get_raw_items(), m_name_to_idx_cbs);
    CivTraitParser civ_trait_parser(m_civ_trait_reader.get_raw_items(), m_name_to_idx_cbs);
    EffectParser effect_parser(m_effect_reader.get_raw_items(), m_name_to_idx_cbs);
    GovernmentParser government_parser(m_government_reader.get_raw_items(), m_name_to_idx_cbs);
    ResourceParser resource_parser(m_resource_reader.get_raw_items(), m_name_to_idx_cbs);
    SmallWonderParser small_wonder_parser(m_small_wonder_reader.get_raw_items(), m_name_to_idx_cbs);
    TechParser tech_parser(m_tech_reader.get_raw_items(), m_name_to_idx_cbs);
    UnitParser unit_parser(m_unit_reader.get_raw_items(), m_name_to_idx_cbs);
    UnitTypeParser unit_type_parser(m_unit_type_reader.get_raw_items(), m_name_to_idx_cbs);
    WonderParser wonder_parser(m_wonder_reader.get_raw_items(), m_name_to_idx_cbs);

    m_building_data = building_parser.parse_data_dependencies();
    m_city_flag_data = city_flag_parser.parse_data_dependencies();
    m_civ_data = civ_parser.parse_data_dependencies();
    m_civ_trait_data = civ_trait_parser.parse_data_dependencies();
    m_effect_data = effect_parser.parse_data_dependencies();
    m_government_data = government_parser.parse_data_dependencies();
    m_resource_data = resource_parser.parse_data_dependencies();
    m_small_wonder_data = small_wonder_parser.parse_data_dependencies();
    m_tech_data = tech_parser.parse_data_dependencies();
    m_unit_data = unit_parser.parse_data_dependencies();
    m_unit_type_data = unit_type_parser.parse_data_dependencies();
    m_wonder_data = wonder_parser.parse_data_dependencies();
}

u16 StaticParsingManager::safe_size_to_u16 (size_t value) {
    const size_t max_u16 = static_cast<size_t>(-1) >> (sizeof(size_t) * 8 - 16);
    if (value > max_u16) {
        printf("ERROR: StaticParsingManager count overflow for u16\n");
        return static_cast<u16>(max_u16);
    }
    return static_cast<u16>(value);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

