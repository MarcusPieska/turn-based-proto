//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_TYPE_ACTION_MAP_PARSING_H
#define UNIT_TYPE_ACTION_MAP_PARSING_H

#include "game_primitives.h"
#include "opt_str_mng.h"
#include "static_bit_bank.h"
#include "unit_action_parser.h"
#include "unit_type_parser.h"

//================================================================================================================================
//=> - UnitTypeActionMapParsing class -
//================================================================================================================================

class UnitTypeActionMapParsing {
public:
    static void load_cfg_map (
        StaticBitBank& bank,
        const StringManager& mapping_items,
        const UnitTypeParser& unit_type_parser,
        const UnitActionParser& unit_action_parser
    );

private:
    UnitTypeActionMapParsing () = delete;
    UnitTypeActionMapParsing (const UnitTypeActionMapParsing& other) = delete;
    UnitTypeActionMapParsing (UnitTypeActionMapParsing&& other) = delete;
};

#endif // UNIT_TYPE_ACTION_MAP_PARSING_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
