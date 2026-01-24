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
#include "map_tile.h"

//================================================================================================================================
//=> - Class: MapView -
//================================================================================================================================

class MapView {
public:
    MapView (int window_width, int window_height, int map_width, int map_height, int tile_width, int tile_height, MapModel *model);
    ~MapView ();
    
    bool initialize ();
    void render_opt_pre (SDL_Surface* img_surface_height, float factor);
    void render_opt ();
    void render_old ();
    void handleEvents ();
    void update ();
    bool isRunning () const;
    void cleanup ();
    int testTileClickDetection ();

private:
    void __handleEventMouseMotion (SDL_Event &e);
    void __handleEventsKeyDown (SDL_Event &e);
    void __handleEventsKeyUp (SDL_Event &e);
    void __handleMouseClick (SDL_Event &e);
    void __highlightTile (MapTile *tile, Uint8 r, Uint8 g, Uint8 b);
    void __highlightCurrentTile (MapTile *tile, Uint8 r, Uint8 g, Uint8 b);
    void __clearHighlights ();
    void __centerOnTile (MapTile *tile);
    void __toggleFullscreen ();
    void __flipVertically ();
    void __handleWndLimits ();
    bool __inBounds (int x, int y, const MapTile &tile);
    std::tuple<MapTile*, size_t, size_t> __searchTileFromPt (int map_x, int map_y, size_t start_row, size_t start_col);

    SDL_Window *m_wnd;
    SDL_Renderer *m_rend;
    MapModel *m_model;
    bool m_running;
    int m_wnd_w, m_wnd_h;
    int m_wnd_w_orig, m_wnd_h_orig;
    int m_map_w, m_map_h;
    int m_tile_w, m_tile_h;
    float m_factor;
    int m_cx, m_cy;
    int m_scroll_add;
    MapTile *m_clicked_tile;
    std::vector<MapTile*> m_near_tiles;
    std::vector<MapTile*> m_diagonal_tiles;
    int m_zoom_level;
    std::map<std::pair<int, int>, float> m_corner_elevations;
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
