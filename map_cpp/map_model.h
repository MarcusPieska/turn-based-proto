//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_MODEL_H
#define MAP_MODEL_H

#include <vector>
#include <string>
#include "map_tile.h"

class MapTiler;

struct Line {
    Point pt1;
    Point pt2;
    Line () {}
    Line (Point p1, Point p2) : pt1(p1), pt2(p2) {}
};

class MapModel {
public:
    MapModel ();
    ~MapModel ();
    
    void setTiles (const std::vector<std::vector<MapTile>> &tiles);
    void setLines (const std::vector<Line> &lines);
    std::vector<std::vector<MapTile>> getTiles () const;
    std::vector<std::vector<MapTile>> &getTilesRef ();
    std::vector<Line> getLines () const;
    void saveTilesToFile (const std::string &filename) const;
    
    MapTiler *m_tiler;

private:
    std::vector<std::vector<MapTile>> m_tiles;
    std::vector<Line> m_lines;
};

#endif // MAP_MODEL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
