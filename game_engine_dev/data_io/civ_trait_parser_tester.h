//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_TRAIT_PARSER_TESTER_H
#define CIV_TRAIT_PARSER_TESTER_H

#include "civ_trait_parser.h"
#include "civ_trait_static_data.h"
#include "path_mng.h"
#include "name_to_idx_callbacks.h"
#include "item_effects.h"

#include "building_static_data.h"
#include "city_flag_static_data.h"
#include "civ_static_data.h"
#include "civ_trait_static_data.h"
#include "resource_static_data.h"
#include "small_wonder_static_data.h"
#include "tech_static_data.h"
#include "unit_static_data.h"
#include "unit_action_static_data.h"
#include "unit_type_static_data.h"
#include "wonder_static_data.h"

//================================================================================================================================
//=> - CivTraitParserTester class -
//================================================================================================================================

class CivTraitParserTester {
public:
    CivTraitParserTester ();
    void set_plvl (int lvl);
    int run ();
    void open_writer ();
    void close_writer ();
    void pr_item (const CivTraitStaticDataStruct& item);
    
    void set_building_sd (const BuildingStaticData* sd);
    void set_city_flag_sd (const CityFlagStaticData* sd);
    void set_civ_sd (const CivStaticData* sd);
    void set_civ_trait_sd (const CivTraitStaticData* sd);
    void set_resource_sd (const ResourceStaticData* sd);
    void set_small_wonder_sd (const SmallWonderStaticData* sd);
    void set_tech_sd (const TechStaticData* sd);
    void set_unit_sd (const UnitStaticData* sd);
    void set_unit_action_sd (const UnitActionStaticData* sd);
    void set_unit_type_sd (const UnitTypeStaticData* sd);
    void set_wonder_sd (const WonderStaticData* sd);

private:
    typedef const char* cstr;
    int m_plvl;
    FILE* m_out;
    FILE* out () const;
    
    const BuildingStaticData* m_building_sd;
    const CityFlagStaticData* m_city_flag_sd;
    const CivStaticData* m_civ_sd;
    const CivTraitStaticData* m_civ_trait_sd;
    const ResourceStaticData* m_resource_sd;
    const SmallWonderStaticData* m_small_wonder_sd;
    const TechStaticData* m_tech_sd;
    const UnitStaticData* m_unit_sd;
    const UnitActionStaticData* m_unit_action_sd;
    const UnitTypeStaticData* m_unit_type_sd;
    const WonderStaticData* m_wonder_sd;

    const DataParserBase* m_building_psr;
    const DataParserBase* m_city_flag_psr;
    const DataParserBase* m_civ_psr;
    const DataParserBase* m_civ_trait_psr;
    const DataParserBase* m_resource_psr;
    const DataParserBase* m_small_wonder_psr;
    const DataParserBase* m_tech_psr;
    const DataParserBase* m_unit_psr;
    const DataParserBase* m_unit_action_psr;
    const DataParserBase* m_unit_type_psr;
    const DataParserBase* m_wonder_psr;

    static CivTraitParserTester* s_inst;

    static u16 st_building_n2i (cstr name);
    static u16 st_city_flag_n2i (cstr name);
    static u16 st_civ_n2i (cstr name);
    static u16 st_civ_trait_n2i (cstr name);
    static u16 st_resource_n2i (cstr name);
    static u16 st_small_wonder_n2i (cstr name);
    static u16 st_tech_n2i (cstr name);
    static u16 st_unit_n2i (cstr name);
    static u16 st_unit_action_n2i (cstr name);
    static u16 st_unit_type_n2i (cstr name);
    static u16 st_wonder_n2i (cstr name);

    bool ld_sm (StringManager& sm, cstr path);
    void pr_u16 (cstr label, u16 value);
    void pr_u32 (cstr label, u32 value);
    void pr_reqs (cstr label, const ItemReqsStruct& reqs);
    void pr_fx (cstr label, const ItemEffectsStruct& e);
    void pr_traits (cstr label, const CivTraitStruct& traits);
};

#endif // CIV_TRAIT_PARSER_TESTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
