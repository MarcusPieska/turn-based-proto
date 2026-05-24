//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "parser_test_manager.h"

//================================================================================================================================
//=> - ParserTestManager implementation -
//================================================================================================================================

ParserTestManager::ParserTestManager () :
    m_plvl(0)
{
}

void ParserTestManager::set_plvl (int lvl) {
    m_plvl = lvl;
    
    m_building.set_plvl(lvl);
    m_city_flag.set_plvl(lvl);
    m_civ.set_plvl(lvl);
    m_civ_trait.set_plvl(lvl);
    m_resource.set_plvl(lvl);
    m_small_wonder.set_plvl(lvl);
    m_tech.set_plvl(lvl);
    m_unit.set_plvl(lvl);
    m_unit_action.set_plvl(lvl);
    m_unit_type.set_plvl(lvl);
    m_wonder.set_plvl(lvl);
}

void ParserTestManager::print_all (const RuntimeStatics& statics) {
    m_building.set_building_sd(&statics.building());
    m_building.set_city_flag_sd(&statics.city_flag());
    m_building.set_civ_sd(&statics.civ());
    m_building.set_civ_trait_sd(&statics.civ_trait());
    m_building.set_resource_sd(&statics.resource());
    m_building.set_small_wonder_sd(&statics.small_wonder());
    m_building.set_tech_sd(&statics.tech());
    m_building.set_unit_sd(&statics.unit());
    m_building.set_unit_action_sd(&statics.unit_action());
    m_building.set_unit_type_sd(&statics.unit_type());
    m_building.set_wonder_sd(&statics.wonder());
    m_city_flag.set_building_sd(&statics.building());
    m_city_flag.set_city_flag_sd(&statics.city_flag());
    m_city_flag.set_civ_sd(&statics.civ());
    m_city_flag.set_civ_trait_sd(&statics.civ_trait());
    m_city_flag.set_resource_sd(&statics.resource());
    m_city_flag.set_small_wonder_sd(&statics.small_wonder());
    m_city_flag.set_tech_sd(&statics.tech());
    m_city_flag.set_unit_sd(&statics.unit());
    m_city_flag.set_unit_action_sd(&statics.unit_action());
    m_city_flag.set_unit_type_sd(&statics.unit_type());
    m_city_flag.set_wonder_sd(&statics.wonder());
    m_civ.set_building_sd(&statics.building());
    m_civ.set_city_flag_sd(&statics.city_flag());
    m_civ.set_civ_sd(&statics.civ());
    m_civ.set_civ_trait_sd(&statics.civ_trait());
    m_civ.set_resource_sd(&statics.resource());
    m_civ.set_small_wonder_sd(&statics.small_wonder());
    m_civ.set_tech_sd(&statics.tech());
    m_civ.set_unit_sd(&statics.unit());
    m_civ.set_unit_action_sd(&statics.unit_action());
    m_civ.set_unit_type_sd(&statics.unit_type());
    m_civ.set_wonder_sd(&statics.wonder());
    m_civ_trait.set_building_sd(&statics.building());
    m_civ_trait.set_city_flag_sd(&statics.city_flag());
    m_civ_trait.set_civ_sd(&statics.civ());
    m_civ_trait.set_civ_trait_sd(&statics.civ_trait());
    m_civ_trait.set_resource_sd(&statics.resource());
    m_civ_trait.set_small_wonder_sd(&statics.small_wonder());
    m_civ_trait.set_tech_sd(&statics.tech());
    m_civ_trait.set_unit_sd(&statics.unit());
    m_civ_trait.set_unit_action_sd(&statics.unit_action());
    m_civ_trait.set_unit_type_sd(&statics.unit_type());
    m_civ_trait.set_wonder_sd(&statics.wonder());
    m_resource.set_building_sd(&statics.building());
    m_resource.set_city_flag_sd(&statics.city_flag());
    m_resource.set_civ_sd(&statics.civ());
    m_resource.set_civ_trait_sd(&statics.civ_trait());
    m_resource.set_resource_sd(&statics.resource());
    m_resource.set_small_wonder_sd(&statics.small_wonder());
    m_resource.set_tech_sd(&statics.tech());
    m_resource.set_unit_sd(&statics.unit());
    m_resource.set_unit_action_sd(&statics.unit_action());
    m_resource.set_unit_type_sd(&statics.unit_type());
    m_resource.set_wonder_sd(&statics.wonder());
    m_small_wonder.set_building_sd(&statics.building());
    m_small_wonder.set_city_flag_sd(&statics.city_flag());
    m_small_wonder.set_civ_sd(&statics.civ());
    m_small_wonder.set_civ_trait_sd(&statics.civ_trait());
    m_small_wonder.set_resource_sd(&statics.resource());
    m_small_wonder.set_small_wonder_sd(&statics.small_wonder());
    m_small_wonder.set_tech_sd(&statics.tech());
    m_small_wonder.set_unit_sd(&statics.unit());
    m_small_wonder.set_unit_action_sd(&statics.unit_action());
    m_small_wonder.set_unit_type_sd(&statics.unit_type());
    m_small_wonder.set_wonder_sd(&statics.wonder());
    m_tech.set_building_sd(&statics.building());
    m_tech.set_city_flag_sd(&statics.city_flag());
    m_tech.set_civ_sd(&statics.civ());
    m_tech.set_civ_trait_sd(&statics.civ_trait());
    m_tech.set_resource_sd(&statics.resource());
    m_tech.set_small_wonder_sd(&statics.small_wonder());
    m_tech.set_tech_sd(&statics.tech());
    m_tech.set_unit_sd(&statics.unit());
    m_tech.set_unit_action_sd(&statics.unit_action());
    m_tech.set_unit_type_sd(&statics.unit_type());
    m_tech.set_wonder_sd(&statics.wonder());
    m_unit.set_building_sd(&statics.building());
    m_unit.set_city_flag_sd(&statics.city_flag());
    m_unit.set_civ_sd(&statics.civ());
    m_unit.set_civ_trait_sd(&statics.civ_trait());
    m_unit.set_resource_sd(&statics.resource());
    m_unit.set_small_wonder_sd(&statics.small_wonder());
    m_unit.set_tech_sd(&statics.tech());
    m_unit.set_unit_sd(&statics.unit());
    m_unit.set_unit_action_sd(&statics.unit_action());
    m_unit.set_unit_type_sd(&statics.unit_type());
    m_unit.set_wonder_sd(&statics.wonder());
    m_unit_action.set_building_sd(&statics.building());
    m_unit_action.set_city_flag_sd(&statics.city_flag());
    m_unit_action.set_civ_sd(&statics.civ());
    m_unit_action.set_civ_trait_sd(&statics.civ_trait());
    m_unit_action.set_resource_sd(&statics.resource());
    m_unit_action.set_small_wonder_sd(&statics.small_wonder());
    m_unit_action.set_tech_sd(&statics.tech());
    m_unit_action.set_unit_sd(&statics.unit());
    m_unit_action.set_unit_action_sd(&statics.unit_action());
    m_unit_action.set_unit_type_sd(&statics.unit_type());
    m_unit_action.set_wonder_sd(&statics.wonder());
    m_unit_type.set_building_sd(&statics.building());
    m_unit_type.set_city_flag_sd(&statics.city_flag());
    m_unit_type.set_civ_sd(&statics.civ());
    m_unit_type.set_civ_trait_sd(&statics.civ_trait());
    m_unit_type.set_resource_sd(&statics.resource());
    m_unit_type.set_small_wonder_sd(&statics.small_wonder());
    m_unit_type.set_tech_sd(&statics.tech());
    m_unit_type.set_unit_sd(&statics.unit());
    m_unit_type.set_unit_action_sd(&statics.unit_action());
    m_unit_type.set_unit_type_sd(&statics.unit_type());
    m_unit_type.set_wonder_sd(&statics.wonder());
    m_wonder.set_building_sd(&statics.building());
    m_wonder.set_city_flag_sd(&statics.city_flag());
    m_wonder.set_civ_sd(&statics.civ());
    m_wonder.set_civ_trait_sd(&statics.civ_trait());
    m_wonder.set_resource_sd(&statics.resource());
    m_wonder.set_small_wonder_sd(&statics.small_wonder());
    m_wonder.set_tech_sd(&statics.tech());
    m_wonder.set_unit_sd(&statics.unit());
    m_wonder.set_unit_action_sd(&statics.unit_action());
    m_wonder.set_unit_type_sd(&statics.unit_type());
    m_wonder.set_wonder_sd(&statics.wonder());

    m_building.open_writer();
    for (u16 i = 0; i < statics.building().get_item_count(); ++i) {
        m_building.pr_item(statics.building().get_item(BuildingStaticDataKey::from_raw(i)));
    }
    m_building.close_writer();
    
    m_city_flag.open_writer();
    for (u16 i = 0; i < statics.city_flag().get_item_count(); ++i) {
        m_city_flag.pr_item(statics.city_flag().get_item(CityFlagStaticDataKey::from_raw(i)));
    }
    m_city_flag.close_writer();
    
    m_civ.open_writer();
    for (u16 i = 0; i < statics.civ().get_item_count(); ++i) {
        m_civ.pr_item(statics.civ().get_item(CivStaticDataKey::from_raw(i)));
    }
    m_civ.close_writer();
    
    m_civ_trait.open_writer();
    for (u16 i = 0; i < statics.civ_trait().get_item_count(); ++i) {
        m_civ_trait.pr_item(statics.civ_trait().get_item(CivTraitStaticDataKey::from_raw(i)));
    }
    m_civ_trait.close_writer();
    
    m_resource.open_writer();
    for (u16 i = 0; i < statics.resource().get_item_count(); ++i) {
        m_resource.pr_item(statics.resource().get_item(ResourceStaticDataKey::from_raw(i)));
    }
    m_resource.close_writer();
    
    m_small_wonder.open_writer();
    for (u16 i = 0; i < statics.small_wonder().get_item_count(); ++i) {
        m_small_wonder.pr_item(statics.small_wonder().get_item(SmallWonderStaticDataKey::from_raw(i)));
    }
    m_small_wonder.close_writer();
    
    m_tech.open_writer();
    for (u16 i = 0; i < statics.tech().get_item_count(); ++i) {
        m_tech.pr_item(statics.tech().get_item(TechStaticDataKey::from_raw(i)));
    }
    m_tech.close_writer();
    
    m_unit.open_writer();
    for (u16 i = 0; i < statics.unit().get_item_count(); ++i) {
        m_unit.pr_item(statics.unit().get_item(UnitStaticDataKey::from_raw(i)));
    }
    m_unit.close_writer();
    
    m_unit_action.open_writer();
    for (u16 i = 0; i < statics.unit_action().get_item_count(); ++i) {
        m_unit_action.pr_item(statics.unit_action().get_item(UnitActionStaticDataKey::from_raw(i)));
    }
    m_unit_action.close_writer();
    
    m_unit_type.open_writer();
    for (u16 i = 0; i < statics.unit_type().get_item_count(); ++i) {
        m_unit_type.pr_item(statics.unit_type().get_item(UnitTypeStaticDataKey::from_raw(i)));
    }
    m_unit_type.close_writer();
    
    m_wonder.open_writer();
    for (u16 i = 0; i < statics.wonder().get_item_count(); ++i) {
        m_wonder.pr_item(statics.wonder().get_item(WonderStaticDataKey::from_raw(i)));
    }
    m_wonder.close_writer();
    
    m_building.set_building_sd(NULL);
    m_building.set_city_flag_sd(NULL);
    m_building.set_civ_sd(NULL);
    m_building.set_civ_trait_sd(NULL);
    m_building.set_resource_sd(NULL);
    m_building.set_small_wonder_sd(NULL);
    m_building.set_tech_sd(NULL);
    m_building.set_unit_sd(NULL);
    m_building.set_unit_action_sd(NULL);
    m_building.set_unit_type_sd(NULL);
    m_building.set_wonder_sd(NULL);
    m_city_flag.set_building_sd(NULL);
    m_city_flag.set_city_flag_sd(NULL);
    m_city_flag.set_civ_sd(NULL);
    m_city_flag.set_civ_trait_sd(NULL);
    m_city_flag.set_resource_sd(NULL);
    m_city_flag.set_small_wonder_sd(NULL);
    m_city_flag.set_tech_sd(NULL);
    m_city_flag.set_unit_sd(NULL);
    m_city_flag.set_unit_action_sd(NULL);
    m_city_flag.set_unit_type_sd(NULL);
    m_city_flag.set_wonder_sd(NULL);
    m_civ.set_building_sd(NULL);
    m_civ.set_city_flag_sd(NULL);
    m_civ.set_civ_sd(NULL);
    m_civ.set_civ_trait_sd(NULL);
    m_civ.set_resource_sd(NULL);
    m_civ.set_small_wonder_sd(NULL);
    m_civ.set_tech_sd(NULL);
    m_civ.set_unit_sd(NULL);
    m_civ.set_unit_action_sd(NULL);
    m_civ.set_unit_type_sd(NULL);
    m_civ.set_wonder_sd(NULL);
    m_civ_trait.set_building_sd(NULL);
    m_civ_trait.set_city_flag_sd(NULL);
    m_civ_trait.set_civ_sd(NULL);
    m_civ_trait.set_civ_trait_sd(NULL);
    m_civ_trait.set_resource_sd(NULL);
    m_civ_trait.set_small_wonder_sd(NULL);
    m_civ_trait.set_tech_sd(NULL);
    m_civ_trait.set_unit_sd(NULL);
    m_civ_trait.set_unit_action_sd(NULL);
    m_civ_trait.set_unit_type_sd(NULL);
    m_civ_trait.set_wonder_sd(NULL);
    m_resource.set_building_sd(NULL);
    m_resource.set_city_flag_sd(NULL);
    m_resource.set_civ_sd(NULL);
    m_resource.set_civ_trait_sd(NULL);
    m_resource.set_resource_sd(NULL);
    m_resource.set_small_wonder_sd(NULL);
    m_resource.set_tech_sd(NULL);
    m_resource.set_unit_sd(NULL);
    m_resource.set_unit_action_sd(NULL);
    m_resource.set_unit_type_sd(NULL);
    m_resource.set_wonder_sd(NULL);
    m_small_wonder.set_building_sd(NULL);
    m_small_wonder.set_city_flag_sd(NULL);
    m_small_wonder.set_civ_sd(NULL);
    m_small_wonder.set_civ_trait_sd(NULL);
    m_small_wonder.set_resource_sd(NULL);
    m_small_wonder.set_small_wonder_sd(NULL);
    m_small_wonder.set_tech_sd(NULL);
    m_small_wonder.set_unit_sd(NULL);
    m_small_wonder.set_unit_action_sd(NULL);
    m_small_wonder.set_unit_type_sd(NULL);
    m_small_wonder.set_wonder_sd(NULL);
    m_tech.set_building_sd(NULL);
    m_tech.set_city_flag_sd(NULL);
    m_tech.set_civ_sd(NULL);
    m_tech.set_civ_trait_sd(NULL);
    m_tech.set_resource_sd(NULL);
    m_tech.set_small_wonder_sd(NULL);
    m_tech.set_tech_sd(NULL);
    m_tech.set_unit_sd(NULL);
    m_tech.set_unit_action_sd(NULL);
    m_tech.set_unit_type_sd(NULL);
    m_tech.set_wonder_sd(NULL);
    m_unit.set_building_sd(NULL);
    m_unit.set_city_flag_sd(NULL);
    m_unit.set_civ_sd(NULL);
    m_unit.set_civ_trait_sd(NULL);
    m_unit.set_resource_sd(NULL);
    m_unit.set_small_wonder_sd(NULL);
    m_unit.set_tech_sd(NULL);
    m_unit.set_unit_sd(NULL);
    m_unit.set_unit_action_sd(NULL);
    m_unit.set_unit_type_sd(NULL);
    m_unit.set_wonder_sd(NULL);
    m_unit_action.set_building_sd(NULL);
    m_unit_action.set_city_flag_sd(NULL);
    m_unit_action.set_civ_sd(NULL);
    m_unit_action.set_civ_trait_sd(NULL);
    m_unit_action.set_resource_sd(NULL);
    m_unit_action.set_small_wonder_sd(NULL);
    m_unit_action.set_tech_sd(NULL);
    m_unit_action.set_unit_sd(NULL);
    m_unit_action.set_unit_action_sd(NULL);
    m_unit_action.set_unit_type_sd(NULL);
    m_unit_action.set_wonder_sd(NULL);
    m_unit_type.set_building_sd(NULL);
    m_unit_type.set_city_flag_sd(NULL);
    m_unit_type.set_civ_sd(NULL);
    m_unit_type.set_civ_trait_sd(NULL);
    m_unit_type.set_resource_sd(NULL);
    m_unit_type.set_small_wonder_sd(NULL);
    m_unit_type.set_tech_sd(NULL);
    m_unit_type.set_unit_sd(NULL);
    m_unit_type.set_unit_action_sd(NULL);
    m_unit_type.set_unit_type_sd(NULL);
    m_unit_type.set_wonder_sd(NULL);
    m_wonder.set_building_sd(NULL);
    m_wonder.set_city_flag_sd(NULL);
    m_wonder.set_civ_sd(NULL);
    m_wonder.set_civ_trait_sd(NULL);
    m_wonder.set_resource_sd(NULL);
    m_wonder.set_small_wonder_sd(NULL);
    m_wonder.set_tech_sd(NULL);
    m_wonder.set_unit_sd(NULL);
    m_wonder.set_unit_action_sd(NULL);
    m_wonder.set_unit_type_sd(NULL);
    m_wonder.set_wonder_sd(NULL);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
