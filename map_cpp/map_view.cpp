//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <iostream>
#include "map_view.h"
#include "map_tiler.h"

#define ADJ_CX(x) ((x - m_cx) * m_zoom_level / 100)
#define ADJ_CY(y) ((y - m_cy) * m_zoom_level / 100)

//================================================================================================================================
//=> - MapView public methods -
//================================================================================================================================

MapView::MapView (int wnd_width, int wnd_height, int map_width, int map_height, const MapModel *model) :
    m_wnd (nullptr),
    m_rend (nullptr),
    m_model (model),
    m_wnd_w (wnd_width),
    m_wnd_h (wnd_height),
    m_wnd_w_orig (wnd_width),
    m_wnd_h_orig (wnd_height),
    m_map_w (map_width),
    m_map_h (map_height),
    m_running (false),
    m_cx (0),
    m_cy (0),
    m_scroll_add (0),
    m_clicked_tile (nullptr),
    m_zoom_level (ZOOM_DEFAULT) {
}

MapView::~MapView () {
    cleanup ();
}

bool MapView::initialize () {
    if (SDL_Init (SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError () << std::endl;
        return false;
    }
    m_wnd = SDL_CreateWindow ("Map View", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_wnd_w, m_wnd_h, SDL_WINDOW_SHOWN);
    if (m_wnd == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError () << std::endl;
        SDL_Quit ();
        return false;
    }
    m_rend = SDL_CreateRenderer (m_wnd, -1, SDL_RENDERER_ACCELERATED);
    if (m_rend == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError () << std::endl;
        SDL_DestroyWindow (m_wnd);
        SDL_Quit ();
        return false;
    }
    m_running = true;
    return true;
}

void MapView::render_old () {
    SDL_SetRenderDrawColor (m_rend, 255, 255, 255, 255);
    SDL_RenderClear (m_rend);
    SDL_SetRenderDrawColor (m_rend, 0, 0, 0, 255);
    std::vector<std::vector<MapTile>> tiles = m_model->getTiles ();
    for (const auto &row : tiles) {
        for (const auto &t : row) {
            for (int i = 0; i < 4; i++) {
                int j = (i + 1) % 4;
                SDL_RenderDrawLine (m_rend, ADJ_CX(t.pts[i].x), ADJ_CY(t.pts[i].y), ADJ_CX(t.pts[j].x), ADJ_CY(t.pts[j].y));
            }
        }
    }
    SDL_RenderPresent (m_rend);
}

void MapView::update () {
    int mouse_x, mouse_y;
    Uint32 buttons = SDL_GetMouseState (&mouse_x, &mouse_y);
    
    int scroll_speed_x = EDGE_SCROLL_SPEED * 100 / m_zoom_level;
    int scroll_speed_y = EDGE_SCROLL_SPEED * 100 / m_zoom_level;
    
    if (mouse_x < EDGE_SCROLL_MARGIN) {
        m_cx -= scroll_speed_x + (EDGE_SCROLL_MARGIN - mouse_x) * 100 / m_zoom_level;
    } else if (mouse_x > m_wnd_w - EDGE_SCROLL_MARGIN) {
        m_cx += scroll_speed_x + (EDGE_SCROLL_MARGIN - (m_wnd_w - mouse_x)) * 100 / m_zoom_level;
    }
    
    if (mouse_y < EDGE_SCROLL_MARGIN) {
        m_cy -= scroll_speed_y + (EDGE_SCROLL_MARGIN - mouse_y) * 100 / m_zoom_level;
    } else if (mouse_y > m_wnd_h - EDGE_SCROLL_MARGIN) {
        m_cy += scroll_speed_y + (EDGE_SCROLL_MARGIN - (m_wnd_h - mouse_y)) * 100 / m_zoom_level;
    }
    
    __handleWndLimits ();
}

void MapView::render_opt () {
    SDL_SetRenderDrawColor (m_rend, 255, 255, 255, 255);
    SDL_RenderClear (m_rend);
    SDL_SetRenderDrawColor (m_rend, 0, 0, 0, 255);
    for (auto *tile : m_diagonal_tiles) {
        if (tile != nullptr) {
            __highlightTile (tile, 144, 238, 144);
        }
    }
    for (auto *tile : m_near_tiles) {
        if (tile != nullptr) {
            __highlightTile (tile, 34, 139, 34);
        }
    }
    if (m_clicked_tile != nullptr) {
        __highlightTile (m_clicked_tile, 0, 100, 0);
    }
    std::vector<Line> lines = m_model->getLines ();
    for (const auto &l : lines) {
        SDL_RenderDrawLine (m_rend, ADJ_CX(l.pt1.x), ADJ_CY(l.pt1.y), ADJ_CX(l.pt2.x), ADJ_CY(l.pt2.y));
    }
    SDL_RenderPresent (m_rend);
}

void MapView::handleEvents () {
    SDL_Event e;
    while (SDL_PollEvent (&e)) {
        if (e.type == SDL_QUIT) {
            m_running = false;
        } else if (e.type == SDL_MOUSEMOTION) {
            __handleEventMouseMotion (e);
        } else if (e.type == SDL_KEYDOWN) {
            __handleEventsKeyDown (e);
        } else if (e.type == SDL_KEYUP) {
            __handleEventsKeyUp (e);
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            __handleMouseClick (e);
        }
    }
}

bool MapView::isRunning () const {
    return m_running;
}

void MapView::cleanup () {
    if (m_rend != nullptr) {
        SDL_DestroyRenderer (m_rend);
        m_rend = nullptr;
    }
    if (m_wnd != nullptr) {
        SDL_DestroyWindow (m_wnd);
        m_wnd = nullptr;
    }
    SDL_Quit ();
}

//================================================================================================================================
//=> - MapView private methods -
//================================================================================================================================

void MapView::__handleEventMouseMotion (SDL_Event &e) {
}

void MapView::__handleEventsKeyDown (SDL_Event &e) {
    switch (e.key.keysym.sym) {
        case SDLK_c:
            __centerOnTile (m_clicked_tile);
            break;
        case SDLK_f:
            __toggleFullscreen ();
            break;
        case SDLK_PLUS:
        case SDLK_EQUALS:
            if (m_zoom_level + ZOOM_STEP <= ZOOM_MAX) {
                m_zoom_level += ZOOM_STEP;
                __handleWndLimits ();
            }
            break;
        case SDLK_MINUS:
            if (m_zoom_level - ZOOM_STEP >= ZOOM_MIN) {
                m_zoom_level -= ZOOM_STEP;
                __handleWndLimits ();
            }
            break;
        case SDLK_LEFT:
            m_scroll_add = m_scroll_add < SCROLL_MAX ? m_scroll_add + 1 : SCROLL_MAX;
            m_cx -= SCROLL_SPEED + m_scroll_add;
            __handleWndLimits ();
            break;
        case SDLK_RIGHT:
            m_scroll_add = m_scroll_add < SCROLL_MAX ? m_scroll_add + 1 : SCROLL_MAX;
            m_cx += SCROLL_SPEED + m_scroll_add;
            __handleWndLimits ();
            break;
        case SDLK_UP:
            m_scroll_add = m_scroll_add < SCROLL_MAX ? m_scroll_add + 1 : SCROLL_MAX;
            m_cy -= SCROLL_SPEED + m_scroll_add;
            __handleWndLimits ();
            break;
        case SDLK_DOWN:
            m_scroll_add = m_scroll_add < SCROLL_MAX ? m_scroll_add + 1 : SCROLL_MAX;
            m_cy += SCROLL_SPEED + m_scroll_add;
            __handleWndLimits ();
            break;
        default: break;
    }
}

void MapView::__handleEventsKeyUp (SDL_Event &e) {
    switch (e.key.keysym.sym) {
        case SDLK_LEFT:
            m_scroll_add = 0;
            break;
        case SDLK_RIGHT:
            m_scroll_add = 0;
            break;
        case SDLK_UP:
            m_scroll_add = 0;
            break;
        case SDLK_DOWN:
            m_scroll_add = 0;
            break;
        default: break;
    }
}

void MapView::__handleMouseClick (SDL_Event &e) {
    switch (e.button.button) {
        case SDL_BUTTON_LEFT: {
            int screen_x = e.button.x;
            int screen_y = e.button.y;
            int map_x = (screen_x * 100 / m_zoom_level) + m_cx;
            int map_y = (screen_y * 100 / m_zoom_level) + m_cy;
            if (m_model->m_tiler != nullptr) {
                auto result = m_model->m_tiler->coordsToTile(map_x, map_y);
                MapTile *tile = std::get<0>(result);
                int row = std::get<1>(result);
                int col = std::get<2>(result);
                std::cout << "Clicked tile: " << row << ", " << col << std::endl;
                if (tile != nullptr) {
                    __clearHighlights ();
                    m_clicked_tile = tile;
                    m_near_tiles = m_model->m_tiler->coordsToNearTiles(map_x, map_y);
                    m_diagonal_tiles = m_model->m_tiler->coordsToDiagonalTiles(map_x, map_y);
                } 
            }
            break;
        }
        default: break;
    }
}

void MapView::__highlightTile (MapTile *tile, Uint8 r, Uint8 g, Uint8 b) {
    if (tile == nullptr) {
        return;
    }
    SDL_SetRenderDrawColor (m_rend, r, g, b, 255);
    SDL_Vertex vertices[4];
    for (int i = 0; i < 4; i++) {
        vertices[i].position.x = static_cast<float>(ADJ_CX(tile->pts[i].x));
        vertices[i].position.y = static_cast<float>(ADJ_CY(tile->pts[i].y));
        vertices[i].color.r = r;
        vertices[i].color.g = g;
        vertices[i].color.b = b;
        vertices[i].color.a = 255;
    }
    int indices[6] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry (m_rend, nullptr, vertices, 4, indices, 6);
}

void MapView::__clearHighlights () {
    m_clicked_tile = nullptr;
    m_near_tiles.clear ();
    m_diagonal_tiles.clear ();
}

void MapView::__centerOnTile (MapTile *tile) {
    if (tile == nullptr) {
        return;
    }
    int center_x = 0;
    int center_y = 0;
    for (int i = 0; i < 4; i++) {
        center_x += tile->pts[i].x;
        center_y += tile->pts[i].y;
    }
    center_x /= 4;
    center_y /= 4;
    m_cx = center_x - m_wnd_w / 2;
    m_cy = center_y - m_wnd_h / 2;
    
    m_cx = m_cx < 0 ? 0 : m_cx;
    m_cx = m_cx > m_map_w - m_wnd_w ? m_map_w - m_wnd_w : m_cx;
    m_cy = m_cy < 0 ? 0 : m_cy;
    m_cy = m_cy > m_map_h - m_wnd_h ? m_map_h - m_wnd_h : m_cy;
}

void MapView::__toggleFullscreen () {
    Uint32 flags = SDL_GetWindowFlags (m_wnd);
    if (flags & SDL_WINDOW_FULLSCREEN) { 
        SDL_SetWindowFullscreen (m_wnd, 0);
        m_wnd_w = m_wnd_w_orig;
        m_wnd_h = m_wnd_h_orig;
    } else {
        SDL_SetWindowFullscreen (m_wnd, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_GetWindowSize (m_wnd, &m_wnd_w, &m_wnd_h);
    }
}

void MapView::__handleWndLimits () {
    int visible_w = m_wnd_w * 100 / m_zoom_level;
    int visible_h = m_wnd_h * 100 / m_zoom_level;
    int max_cx = m_map_w - visible_w;
    int max_cy = m_map_h - visible_h;
    
    max_cx = max_cx < 0 ? 0 : max_cx;
    max_cy = max_cy < 0 ? 0 : max_cy;
    
    m_cx = m_cx < 0 ? 0 : m_cx;
    m_cx = m_cx > max_cx ? max_cx : m_cx;
    m_cy = m_cy < 0 ? 0 : m_cy;
    m_cy = m_cy > max_cy ? max_cy : m_cy;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
