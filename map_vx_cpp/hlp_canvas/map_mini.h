//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_MINI_H
#define MAP_MINI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "../map_types.h"

//================================================================================================================================
//=> - Class: MapMini -
//================================================================================================================================

class MapMini {
public:
    MapMini (SDL_Renderer* rend, const char* img_path, Size mini_size, Size canvas_size, Size map_size);
    ~MapMini ();
    void render (int cam_x, int cam_y, int vis_w, int vis_h);
    void renderFast (int cam_x, int cam_y, int vis_w, int vis_h);
    bool isPointOnMinimap (Point scr_pt) const;
    void minimapToMap (Point mini_pt, Point& map_pt) const;
    void setBox (Rect map_rect);

private:
    void __buildCache ();
    
    SDL_Renderer* m_rend;
    SDL_Texture* m_map_tex;
    SDL_Texture* m_cached_tex;
    Size m_mini_size;
    Size m_canvas_size;
    Size m_map_size;
    Point m_mini_pos;
    Point m_img_pos;
    Size m_img_size;
    float m_scale;
    Rect m_box_rect;
};

#endif

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
