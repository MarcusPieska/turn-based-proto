//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_ROAD_NETWORK_H
#define GEN_ROAD_NETWORK_H

#include "game_primitives.h"
#include "starting_point_generator.h"
#include "whiteboard_mng.h"

class GameArraySimple;

//================================================================================================================================
//=> - GenRoadNetwork -
//================================================================================================================================
//
//  Pre-compute AI road plan. mk03: assume CITY intents (and starts) are cities; CityNetwork-style
//  nearest neighbor per NE/NW/SE/SW quadrant (flood, range 25); CityConnector spines on each unique
//  link. m_road = segment id; m_foll reused as city-index scratch. Stamps ROAD_VIRTUAL. Default impl.
//
//================================================================================================================================

class GenRoadNetwork {
public:
    GenRoadNetwork ();
    ~GenRoadNetwork ();

    bool begin (GameArraySimple& map);
    bool build (const SpgCoordPair* starts, u32 start_n);
    void clr ();

    bool ok () const;
    u32 term_n () const;
    u32 road_n () const;
    u16 seg_n () const;
    const Whiteboard_2B& roads () const;

private:
    GenRoadNetwork (const GenRoadNetwork& other) = delete;
    GenRoadNetwork& operator= (const GenRoadNetwork& other) = delete;
    GenRoadNetwork (GenRoadNetwork&& other) = delete;
    GenRoadNetwork& operator= (GenRoadNetwork&& other) = delete;

    GameArraySimple* m_map; // Non-owning; stamped with ROAD_VIRTUAL
    Whiteboard_2B m_road; // Segment id on planned road tiles
    Whiteboard_2B m_foll; // Segment id on followed spine tiles
    u32 m_term_n; // Terminal count used in last build
    u32 m_road_n; // Tiles marked in m_road / stamped
    u16 m_seg_n; // Highest segment id issued
    bool m_ok; // True after begin
};

#endif // GEN_ROAD_NETWORK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
