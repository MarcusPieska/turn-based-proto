//================================================================================================================================
//=> - Test driver for MapMini -
//================================================================================================================================

#include <iostream>
#include <SDL2/SDL.h>

#include "map_mini.h"

#include "../map_types.h"

int main (int argc, char* argv[]) {
    const int MINI_W = 200;
    const int MINI_H = 150;
    const int MAP_W = 1000;
    const int MAP_H = 750;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    SDL_Window* wnd;
    wnd = SDL_CreateWindow("Minimap Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, MAP_W, MAP_H, SDL_WINDOW_SHOWN);
    if (wnd == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* rend = SDL_CreateRenderer(wnd, -1, SDL_RENDERER_ACCELERATED);
    if (rend == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(wnd);
        SDL_Quit();
        return 1;
    }

    Size mini_size(MINI_W, MINI_H);
    Size canvas_size(MAP_W, MAP_H);
    Size map_size(MAP_W, MAP_H);
    MapMini mini(rend, "/home/w/Projects/rts-proto-map/first-test/cont001.png", mini_size, canvas_size, map_size);
    int cam_x = 500;
    int cam_y = 400;
    int vis_w = MAP_W;
    int vis_h = MAP_H;
    Point click_pt(-1, -1);
    bool run = true;
    SDL_Event e;
    while (run) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                run = false;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        run = false;
                        break;
                    case SDLK_LEFT:
                        cam_x = std::max(0, cam_x - 50);
                        break;
                    case SDLK_RIGHT:
                        cam_x = std::min(MAP_W - vis_w, cam_x + 50);
                        break;
                    case SDLK_UP:
                        cam_y = std::max(0, cam_y - 50);
                        break;
                    case SDLK_DOWN:
                        cam_y = std::min(MAP_H - vis_h, cam_y + 50);
                        break;
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    Point scr_pt(e.button.x, e.button.y);
                    if (mini.isPointOnMinimap(scr_pt)) {
                        Point map_pt;
                        mini.minimapToMap(scr_pt, map_pt);
                        mini.setBox(Rect(map_pt.x - 50, map_pt.y - 50, 100, 100));
                        click_pt = map_pt;
                        cam_x = std::max(0, std::min(MAP_W - vis_w, map_pt.x - vis_w / 2));
                        cam_y = std::max(0, std::min(MAP_H - vis_h, map_pt.y - vis_h / 2));
                    }
                }
            }
        }
        SDL_SetRenderDrawColor(rend, 64, 64, 64, 255);
        SDL_RenderClear(rend);
        if (click_pt.x >= 0 && click_pt.y >= 0) {
            SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
            int dot_x = click_pt.x - cam_x;
            int dot_y = click_pt.y - cam_y;
            if (dot_x >= 0 && dot_x < vis_w && dot_y >= 0 && dot_y < vis_h) {
                for (int dy = -3; dy <= 3; dy++) {
                    for (int dx = -3; dx <= 3; dx++) {
                        if (dx * dx + dy * dy <= 9) {
                            SDL_RenderDrawPoint(rend, dot_x + dx, dot_y + dy);
                        }
                    }
                }
            }
        }
        mini.renderFast(cam_x, cam_y, vis_w, vis_h);
        SDL_RenderPresent(rend);
        SDL_Delay(16);
    }
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(wnd);
    SDL_Quit();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
