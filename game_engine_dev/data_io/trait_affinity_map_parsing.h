//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TRAIT_AFFINITY_MAP_PARSING_H
#define TRAIT_AFFINITY_MAP_PARSING_H

#include "civ_trait_parser.h"
#include "game_primitives.h"
#include "opt_str_mng.h"
#include "trait_affinity_map.h"

//================================================================================================================================
//=> - TraitAffinityMapParsing -
//================================================================================================================================

class TraitAffinityMapParsing {
public:
    static bool load_cfg (
        TraitAffinityMap& out,
        const StringManager& items,
        const CivTraitParser& civ_trait_parser
    );

private:
    TraitAffinityMapParsing () = delete;
    TraitAffinityMapParsing (const TraitAffinityMapParsing& o) = delete;
    TraitAffinityMapParsing (TraitAffinityMapParsing&& o) = delete;
};

#endif // TRAIT_AFFINITY_MAP_PARSING_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
