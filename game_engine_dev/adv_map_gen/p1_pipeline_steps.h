//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_PIPELINE_STEPS_H
#define P1_PIPELINE_STEPS_H

#include "game_primitives.h"

//================================================================================================================================
//=> - P1 pipeline steps -
//================================================================================================================================

static const u16 P1_STEP_OUTLINE = 1u;
static const u16 P1_STEP_OUTLINE_FILL = 2u;
static const u16 P1_STEP_NOISE = 3u;
static const u16 P1_STEP_LAND_DEPTH = 4u;
static const u16 P1_STEP_SHAPED_OUTLINE = 7u;
static const u16 P1_STEP_RIVER_PROB = 8u;
static const u16 P1_STEP_OCEAN_INDEX = 9u;
static const u16 P1_STEP_RIVER_PTS = 10u;
static const u16 P1_STEP_RIVER_SECTORS = 11u;
static const u16 P1_STEP_RIVER_SECT_ADJ = 12u;
static const u16 P1_STEP_COASTAL_MTN_LIMITS = 13u;
static const u16 P1_STEP_RIVER_NETWORK = 14u;
static const u16 P1_STEP_RIVER_LINES = 15u;
static const u16 P1_STEP_COASTAL_MTN_RIVERS = 16u;
static const u16 P1_STEP_RIVER_LAKES = 17u;
static const u16 P1_STEP_RIVER_INLETS = 18u;
static const u16 P1_STEP_WATERSHED_MTN = 19u;
static const u16 P1_STEP_DISTANCE_TO_RIVER = 20u;
static const u16 P1_STEP_RIVER_DIST = 21u;
static const u16 P1_STEP_NEARNESS_WATERSHED_MTN = 22u;
static const u16 P1_STEP_LAND_ALTITUDE = 23u;
static const u16 P1_STEP_ENSURE_COASTS = 24u;
static const u16 P1_STEP_ENSURE_SEAS = 25u;
static const u16 P1_STEP_ENSURE_RIVER_VALLEYS = 26u;
static const u16 P1_STEP_ENSURE_MTN_FOOTHILLS = 27u;
static const u16 P1_STEP_WIND = 28u;
static const u16 P1_STEP_RAIN = 29u;
static const u16 P1_STEP_CLIMATE = 30u;
static const u16 P1_STEP_DESERT_CULL = 31u;
static const u16 P1_STEP_LOESS = 32u;
static const u16 P1_STEP_GRASS_LOESS = 33u;
static const u16 P1_STEP_MAKE_MAP = 35u;
static const u16 P1_STEP_RICH_COAST_FERT = 36u;
static const u16 P1_STEP_COAST_FERT_ADJ = 37u;
static const u16 P1_STEP_FOREST_OVERLAY = 39u;
static const u16 P1_STEP_DELTA_SWAMPS = 40u;
static const u16 P1_STEP_ENSURE_ADJ = 41u;
static const u16 P1_STEP_RESOURCES = 42u;

#endif // P1_PIPELINE_STEPS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
