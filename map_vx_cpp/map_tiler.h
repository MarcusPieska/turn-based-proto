//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_TILER_H
#define MAP_TILER_H

#include <vector>
#include <tuple>
#include "map_tile.h"

class MapModel;

class MapTiler {
public:
    MapTiler (int map_w, int map_h, int tile_w, int tile_h, MapModel *model, int added_top_margin);
    ~MapTiler ();
    
    std::tuple<MapTile*, int, int> coordsToTile (int x, int y, const std::vector<std::vector<MapTile>> *tiles = nullptr);
    std::vector<MapTile*> coordsToNearTiles (int x, int y);
    std::vector<MapTile*> coordsToDiagonalTiles (int x, int y);
    std::vector<MapTile*> tileIdxToNearTiles (int row, int col);
    std::vector<MapTile*> tileIdxToDiagonalTiles (int row, int col);

private:
    std::tuple<int, int> coordsToIndices (int x, int y, const std::vector<std::vector<MapTile>> &tiles);
    
    int m_tile_w;
    int m_tile_h;
    int m_half_w;
    int m_half_h;
    int m_num_cols;
    int m_num_rows;
    int m_x_min;
    int m_y_min;
    MapModel *m_model;
};

#endif // MAP_TILER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
