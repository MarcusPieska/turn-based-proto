//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_BLD_DISCOUNT_MAP_PARSING_H
#define CIV_BLD_DISCOUNT_MAP_PARSING_H

#include "building_parser.h"
#include "civ_trait_parser.h"
#include "data_reader.h"
#include "game_primitives.h"
#include "static_bit_bank.h"

//================================================================================================================================
//=> - CivBldDiscountMapParsing class -
//================================================================================================================================

class CivBldDiscountMapParsing {
public:
    static void load_cfg_map (
        StaticBitBank& bank,
        const DataReader& mapping_reader,
        const CivTraitParser& civ_trait_parser,
        const BuildingParser& building_parser
    );

private:
    CivBldDiscountMapParsing () = delete;
    CivBldDiscountMapParsing (const CivBldDiscountMapParsing& other) = delete;
    CivBldDiscountMapParsing (CivBldDiscountMapParsing&& other) = delete;
};

#endif // CIV_BLD_DISCOUNT_MAP_PARSING_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
