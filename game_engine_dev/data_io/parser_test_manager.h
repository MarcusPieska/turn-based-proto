//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PARSER_TEST_MANAGER_H
#define PARSER_TEST_MANAGER_H

#include "building_parser_tester.h"
#include "city_flag_parser_tester.h"
#include "civ_parser_tester.h"
#include "civ_trait_parser_tester.h"
#include "resource_parser_tester.h"
#include "small_wonder_parser_tester.h"
#include "tech_parser_tester.h"
#include "unit_parser_tester.h"
#include "unit_action_parser_tester.h"
#include "unit_type_parser_tester.h"
#include "wonder_parser_tester.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - ParserTestManager class -
//================================================================================================================================

class ParserTestManager {
public:
    ParserTestManager ();
    void set_plvl (int lvl);
    void print_all (const RuntimeStatics& statics);

    BuildingParserTester& building () { return m_building; }
    CityFlagParserTester& city_flag () { return m_city_flag; }
    CivParserTester& civ () { return m_civ; }
    CivTraitParserTester& civ_trait () { return m_civ_trait; }
    ResourceParserTester& resource () { return m_resource; }
    SmallWonderParserTester& small_wonder () { return m_small_wonder; }
    TechParserTester& tech () { return m_tech; }
    UnitParserTester& unit () { return m_unit; }
    UnitActionParserTester& unit_action () { return m_unit_action; }
    UnitTypeParserTester& unit_type () { return m_unit_type; }
    WonderParserTester& wonder () { return m_wonder; }

private:
    int m_plvl;
    
    BuildingParserTester m_building;
    CityFlagParserTester m_city_flag;
    CivParserTester m_civ;
    CivTraitParserTester m_civ_trait;
    ResourceParserTester m_resource;
    SmallWonderParserTester m_small_wonder;
    TechParserTester m_tech;
    UnitParserTester m_unit;
    UnitActionParserTester m_unit_action;
    UnitTypeParserTester m_unit_type;
    WonderParserTester m_wonder;
};

#endif // PARSER_TEST_MANAGER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
