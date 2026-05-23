//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCE_PARSER_TESTER_H
#define RESOURCE_PARSER_TESTER_H

#include "resource_parser.h"
#include "resource_static_data.h"
#include "path_mng.h"
#include "name_to_idx_callbacks.h"
#include "item_effects.h"

//================================================================================================================================
//=> - ResourceParserTester class -
//================================================================================================================================

class ResourceParserTester {
public:
    ResourceParserTester ();
    void set_plvl (int lvl);
    int run ();
    void pr_item (const ResourceStaticDataStruct& item);

private:
    typedef const char* cstr;
    int m_plvl;
    
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

    static ResourceParserTester* s_inst;

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

#endif // RESOURCE_PARSER_TESTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
