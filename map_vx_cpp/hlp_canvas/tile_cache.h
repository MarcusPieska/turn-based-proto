//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TILE_CACHE_H
#define TILE_CACHE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "sliding_matrix.h"

//================================================================================================================================
//=> - Struct: VisibleTile -
//================================================================================================================================

typedef struct VisibleTile {
    SDL_Texture* tex;
    VisibleTile ();
} VisibleTile;

//================================================================================================================================
//=> - Class: TileCache -
//================================================================================================================================

class TileCache : public SlidingMatrix<VisibleTile> {
public:
    TileCache (int rows, int cols, int row_limit, int col_lim, int left_col, int top_row);
    void shiftWindow (int left_col, int top_row);

    SDL_Texture* getTexture (int row, int col);
    void setTexture (int row, int col, SDL_Texture* tex);
    
private:
    void __deallocRowHead ();
    void __deallocRowTail ();
    void __deallocColumnHead ();
    void __deallocColumnTail ();

    void __nullSetNewRowHead ();
    void __nullSetNewRowTail ();
    void __nullSetNewColumnHead ();
    void __nullSetNewColumnTail ();

    int m_leftCol;
    int m_topRow;
    int m_rows;
    int m_cols;
    int m_rowLimit;
    int m_colLim;
};

#endif

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
