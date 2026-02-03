//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_MODEL_H
#define MAP_MODEL_H

#include <vector>
#include <string>
#include <map>

#include "map_model_tile.h"

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
    
    void allocateTiles (int num_rows, int num_cols);
    void setLines (const std::vector<Line> &lines);
    void setTileElevations (int **z_values);
    int getWidth () const;
    int getHeight () const;
    MapModelTile** getTiles ();
    MapModelTile* getTile (int row, int col);
    std::vector<Line> getLines () const;
    void validateCornerElevations () const;
    
    MapTiler *m_tiler;

private:
    MapModelTile** m_tiles;
    int m_num_rows;
    int m_num_cols;
    std::vector<Line> m_lines;
};

#endif // MAP_MODEL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
