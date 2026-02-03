//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "tile_cache.h"

//================================================================================================================================
//=> - TileCache public methods -
//================================================================================================================================

TileCache::TileCache (int rows, int cols, int row_limit, int col_lim, int left_col, int top_row) :
    m_leftCol(left_col),
    m_topRow(top_row),
    m_rows(rows),
    m_cols(cols),
    m_rowLimit(row_limit),
    m_colLim(col_lim) {
    for (int r = 0; r < m_rows; r++) {
        addRowTail();
    }
    for (int c = 0; c < m_cols; c++) {
        addColumnTail();
    }
}

void TileCache::shiftWindow (int left_col, int top_row) {
    int col_diff = left_col - m_leftCol;
    int row_diff = top_row - m_topRow;
    int actual_left = m_leftCol;
    int actual_top = m_topRow;
    if (col_diff > 0) {
        for (int i = 0; i < col_diff; i++) {
            int new_left = actual_left + 1;
            int new_right = new_left + getCols() - 1;
            if (new_right < m_colLim) {
                __deallocColumnHead();
                addColumnTail();
                actual_left = new_left;
            } else {
                break;
            }
        }
    } else if (col_diff < 0) {
        for (int i = 0; i < -col_diff; i++) {
            int new_left = actual_left - 1;
            if (new_left >= 0) {
                __deallocColumnTail();
                addColumnHead();
                actual_left = new_left;
            } else {
                break;
            }
        }
    }
    if (row_diff > 0) {
        for (int i = 0; i < row_diff; i++) {
            int new_top = actual_top + 1;
            int new_bottom = new_top + getRows() - 1;
            if (new_bottom < m_rowLimit) {
                __deallocRowHead();
                addRowTail();
                actual_top = new_top;
            } else {
                break;
            }
        }
    } else if (row_diff < 0) {
        for (int i = 0; i < -row_diff; i++) {
            int new_top = actual_top - 1;
            if (new_top >= 0) {
                __deallocRowTail();
                addRowHead();
                actual_top = new_top;
            } else {
                break;
            }
        }
    }
    m_leftCol = actual_left;
    m_topRow = actual_top;
}

SDL_Texture* TileCache::getTexture (int row, int col) {
    return get(row, col).tex;
}

void TileCache::setTexture (int row, int col, SDL_Texture* tex) {
    get(row, col).tex = tex;
}

//================================================================================================================================
//=> - TileCache private methods -
//================================================================================================================================

void TileCache::__deallocRowHead () {
    for (int c = 0; c < getCols(); c++) {
        VisibleTile tile = get(0, c);
        SDL_DestroyTexture(tile.tex);
    }
    delRowHead();
}

void TileCache::__deallocRowTail () {
    int last_row = getRows() - 1;
    for (int c = 0; c < getCols(); c++) {
        VisibleTile tile = get(last_row, c);
        SDL_DestroyTexture(tile.tex);
    }
    delRowTail();
}

void TileCache::__deallocColumnHead () {
    for (int r = 0; r < getRows(); r++) {
        VisibleTile tile = get(r, 0);
        SDL_DestroyTexture(tile.tex);
    }
    delColumnHead();
}

void TileCache::__deallocColumnTail () {
    int last_col = getCols() - 1;
    for (int r = 0; r < getRows(); r++) {
        VisibleTile tile = get(r, last_col);
        SDL_DestroyTexture(tile.tex);
    }
    delColumnTail();
}

void TileCache::__nullSetNewRowHead () {
    addRowHead();
    for (int c = 0; c < getCols(); c++) {
        m_data[0][c].tex = nullptr;
    }
}

void TileCache::__nullSetNewRowTail () {
    addRowTail();
    int last_row = getRows() - 1;
    for (int c = 0; c < getCols(); c++) {
        m_data[last_row][c].tex = nullptr;
    }
}

void TileCache::__nullSetNewColumnHead () {
    addColumnHead();
    for (int r = 0; r < getRows(); r++) {
        m_data[r][0].tex = nullptr;
    }
}

void TileCache::__nullSetNewColumnTail () {
    addColumnTail();
    int last_col = getCols() - 1;
    for (int r = 0; r < getRows(); r++) {
        m_data[r][last_col].tex = nullptr;
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
