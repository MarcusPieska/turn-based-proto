//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_VIEW_H
#define MAP_VIEW_H

#include <SDL2/SDL.h>
#include <vector>
#include "map_model.h"
#include "map_tile.h"

//================================================================================================================================
//=> - Class: MapView -
//================================================================================================================================

class MapView {
public:
    MapView (int window_width, int window_height, int map_width, int map_height, const MapModel *model);
    ~MapView ();
    
    bool initialize ();
    void render_opt ();
    void render_old ();
    void handleEvents ();
    void update ();
    bool isRunning () const;
    void cleanup ();

private:
    void __handleEventMouseMotion (SDL_Event &e);
    void __handleEventsKeyDown (SDL_Event &e);
    void __handleEventsKeyUp (SDL_Event &e);
    void __handleMouseClick (SDL_Event &e);
    void __highlightTile (MapTile *tile, Uint8 r, Uint8 g, Uint8 b);
    void __clearHighlights ();
    void __centerOnTile (MapTile *tile);
    void __toggleFullscreen ();

    SDL_Window *m_wnd;
    SDL_Renderer *m_rend;
    const MapModel *m_model;
    bool m_running;
    int m_wnd_w, m_wnd_h;
    int m_wnd_w_orig, m_wnd_h_orig;
    int m_map_w, m_map_h;
    int m_cx, m_cy;
    int m_scroll_add;
    MapTile *m_clicked_tile;
    std::vector<MapTile*> m_near_tiles;
    std::vector<MapTile*> m_diagonal_tiles;
    static const int SCROLL_SPEED = 10;
    static const int SCROLL_MAX = 100;
    static const int EDGE_SCROLL_MARGIN = 50;
    static const int EDGE_SCROLL_SPEED = 5;
};

#endif // MAP_VIEW_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
