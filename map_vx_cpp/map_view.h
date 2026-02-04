//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_VIEW_H
#define MAP_VIEW_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <map>

#include "map_model.h"
#include "map_view_tile.h"

class Dat15Reader;
class Dat31Reader;
class MapMini;

//================================================================================================================================
//=> - Class: MapView -
//================================================================================================================================

class MapView {
public:
    MapView (int window_width, int window_height, int map_width, int map_height, int tile_width, int tile_height, MapModel *model);
    ~MapView ();
    
    bool initialize ();
    void preRenderSetup (SDL_Surface* img_surface_height, float factor);
    void renderOpt ();
    void handleEvents ();
    void update ();
    bool isRunning () const;
    void cleanup ();
    int testTileClickDetection ();

private:
    typedef struct VisMeta {
        int32_t visible_h;
        int32_t visible_w;
        int32_t y_min;
        int32_t y_max;
        int32_t min_col;
        int32_t max_col;
        int32_t min_row;
        int32_t max_row;
    } VisMeta;

    VisMeta __getVisibilityMeta ();

    void __handleEventMouseMotion (SDL_Event &e);
    void __handleEventsKeyDown (SDL_Event &e);
    void __handleEventsKeyUp (SDL_Event &e);
    void __handleMouseClick (SDL_Event &e);
    void __highlightTile (MapModelTile *tile, Uint8 r, Uint8 g, Uint8 b);
    void __highlightCurrentTile (MapModelTile *tile, Uint8 r, Uint8 g, Uint8 b);
    void __clearHighlights ();
    void __centerOnTile (MapModelTile *tile);
    void __toggleFullscreen ();
    void __flipVertically ();
    void __handleWndLimits ();
    void __clearOldTextures (const VisMeta &meta);
    bool __inBounds (int x, int y, const MapModelTile &tile, int row, int col);
    std::tuple<MapModelTile*, int, int> __searchTileFromPt (int map_x, int map_y, int start_row, int start_col);

    SDL_Window *m_wnd;
    SDL_Renderer *m_rend;
    MapModel *m_model;
    bool m_running;
    int m_wnd_w, m_wnd_h;
    int m_wnd_w_orig, m_wnd_h_orig;
    int m_map_w, m_map_h;
    int m_tile_w, m_tile_h;
    int m_mtn_decal_w, m_mtn_decal_h;
    float m_factor;
    int m_cx, m_cy;
    int m_scroll_add;
    MapModelTile *m_clicked_tile;
    NearTiles m_near_tiles;
    int m_zoom_level;
    std::map<std::pair<int, int>, float> m_corner_elevations;
    Dat15Reader* m_tex_read_ocean;
    Dat15Reader* m_tex_read_desert;
    Dat15Reader* m_tex_read_plains;
    Dat15Reader* m_tex_read_grassland;
    Dat15Reader* m_tex_read_tundra;
    Dat31Reader* m_decal_read_mtn;
    int m_tile_text_w;
    int m_tile_text_h;
    MapMini* m_mini;
    SDL_Surface* m_terrain_map;
    MapViewTile** m_view_tiles;
    int m_num_rows;
    int m_num_cols;

    int m_prev_min_row;
    int m_prev_min_col;
    int m_prev_max_row;
    int m_prev_max_col;
    
    static const int SCROLL_SPEED = 10;
    static const int SCROLL_MAX = 100;
    static const int EDGE_SCROLL_MARGIN = 50;
    static const int EDGE_SCROLL_SPEED = 5;
    static const int ZOOM_DEFAULT = 100;
    static const int ZOOM_MAX = 200;
    static const int ZOOM_MIN = 10;
    static const int ZOOM_STEP = 10;
};

#endif // MAP_VIEW_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
