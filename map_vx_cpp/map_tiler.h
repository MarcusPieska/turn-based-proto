//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_TILER_H
#define MAP_TILER_H

#include "map_model_tile.h"
#include "map_view_tile.h"
#include "map_types.h"

class MapModel;

class MapTiler {
public:
    MapTiler (int map_w, int map_h, int tile_w, int tile_h, int tile_cols, int tile_rows, MapModel *model, int top_margin);
    ~MapTiler ();
    
    NearTiles getNearTiles (int row, int col);

private:
    NearTiles m_odd_row_offsets;
    NearTiles m_even_row_offsets;
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
