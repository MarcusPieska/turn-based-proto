//================================================================================================================================
//=> - Test driver for SlidingMatrix -
//================================================================================================================================

#include "sliding_matrix.h"
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <cstdio>

typedef struct TileData {
    int x;
    int y;
} TileData;

//================================================================================================================================
//=> - Class: LimitedSlidingMatrix -
//================================================================================================================================

class LimitedSlidingMatrix : public SlidingMatrix<TileData> {
public:
    LimitedSlidingMatrix (int max_rows, int max_cols);
    void slideUp ();
    void slideDown ();
    void slideLeft ();
    void slideRight ();
    void printIndices () const;
    void validateIndices () const;

private:
    void __setRowData (int row);
    void __setColumnData (int col);
    void __updateFromSubstrate ();
    bool __isRowInSubstrate (int row) const;
    bool __isColInSubstrate (int col) const;
    TileData __getFromSubstrate (int substrate_x, int substrate_y) const;
    SlidingMatrix<TileData> m_substrate;
    int m_max_rows;
    int m_max_cols;
    int m_substrate_rows;
    int m_substrate_cols;
    int m_offset_row;
    int m_offset_col;
};

LimitedSlidingMatrix::LimitedSlidingMatrix (int max_rows, int max_cols) :
    m_max_rows(max_rows),
    m_max_cols(max_cols),
    m_substrate_rows(20),
    m_substrate_cols(20),
    m_offset_row(0),
    m_offset_col(0) {
    for (int r = 0; r < m_substrate_rows; r++) {
        m_substrate.addRowTail();
    }
    for (int c = 0; c < m_substrate_cols; c++) {
        m_substrate.addColumnTail();
    }
    for (int r = 0; r < m_substrate_rows; r++) {
        for (int c = 0; c < m_substrate_cols; c++) {
            TileData td;
            td.x = c;
            td.y = r;
            m_substrate.set(r, c, td);
        }
    }
    addRowTail();
    for (int c = 1; c < m_max_cols; c++) {
        addColumnTail();
    }
    for (int r = 1; r < m_max_rows; r++) {
        addRowTail();
    }
    __updateFromSubstrate();
}

TileData LimitedSlidingMatrix::__getFromSubstrate (int substrate_x, int substrate_y) const {
    if (substrate_x >= 0 && substrate_x < m_substrate_cols && substrate_y >= 0 && substrate_y < m_substrate_rows) {
        return m_substrate.get(substrate_y, substrate_x);
    }
    TileData td;
    td.x = -999;
    td.y = -999;
    return td;
}

void LimitedSlidingMatrix::__updateFromSubstrate () {
    for (int r = 0; r < getRows(); r++) {
        for (int c = 0; c < getCols(); c++) {
            int substrate_x = m_offset_col + c;
            int substrate_y = m_offset_row + r;
            if (substrate_x >= 0 && substrate_x < m_substrate_cols && substrate_y >= 0 && substrate_y < m_substrate_rows) {
                TileData td = m_substrate.get(substrate_y, substrate_x);
                set(r, c, td);
            }
        }
    }
}

void LimitedSlidingMatrix::__setRowData (int row) {
    if (getRows() == 0 || getCols() == 0) return;
    for (int c = 0; c < getCols(); c++) {
        int substrate_x = m_offset_col + c;
        int substrate_y = m_offset_row + row;
        if (substrate_x >= 0 && substrate_x < m_substrate_cols && substrate_y >= 0 && substrate_y < m_substrate_rows) {
            TileData td = m_substrate.get(substrate_y, substrate_x);
            set(row, c, td);
        }
    }
}

void LimitedSlidingMatrix::__setColumnData (int col) {
    if (getRows() == 0 || getCols() == 0) return;
    for (int r = 0; r < getRows(); r++) {
        int substrate_x = m_offset_col + col;
        int substrate_y = m_offset_row + r;
        if (substrate_x >= 0 && substrate_x < m_substrate_cols && substrate_y >= 0 && substrate_y < m_substrate_rows) {
            TileData td = m_substrate.get(substrate_y, substrate_x);
            set(r, col, td);
        }
    }
}

bool LimitedSlidingMatrix::__isRowInSubstrate (int row) const {
    int substrate_y = m_offset_row + row;
    return substrate_y >= 0 && substrate_y < m_substrate_rows;
}

bool LimitedSlidingMatrix::__isColInSubstrate (int col) const {
    int substrate_x = m_offset_col + col;
    return substrate_x >= 0 && substrate_x < m_substrate_cols;
}

void LimitedSlidingMatrix::slideUp () {
    m_offset_row--;
    bool newTopInSubstrate = __isRowInSubstrate(0);
    if (newTopInSubstrate) {
        if (getRows() < m_max_rows) {
            addRowHead();
            __setRowData(0);
        } else {
            delRowTail();
            addRowHead();
            __setRowData(0);
        }
    } else {
        while (getRows() > 0 && !__isRowInSubstrate(0)) {
            delRowHead();
        }
    }
    __updateFromSubstrate();
}

void LimitedSlidingMatrix::slideDown () {
    m_offset_row++;
    bool newBottomInSubstrate = (getRows() > 0) ? __isRowInSubstrate(getRows() - 1) : false;
    if (newBottomInSubstrate) {
        if (getRows() < m_max_rows) {
            addRowTail();
            __setRowData(getRows() - 1);
        } else {
            delRowHead();
            addRowTail();
            __setRowData(getRows() - 1);
        }
    } else {
        while (getRows() > 0 && !__isRowInSubstrate(getRows() - 1)) {
            delRowTail();
        }
    }
    __updateFromSubstrate();
}

void LimitedSlidingMatrix::slideLeft () {
    m_offset_col--;
    bool newLeftInSubstrate = __isColInSubstrate(0);
    if (newLeftInSubstrate) {
        if (getCols() < m_max_cols) {
            addColumnHead();
            __setColumnData(0);
        } else {
            delColumnTail();
            addColumnHead();
            __setColumnData(0);
        }
    } else {
        while (getCols() > 0 && !__isColInSubstrate(0)) {
            delColumnHead();
        }
    }
    __updateFromSubstrate();
}

void LimitedSlidingMatrix::slideRight () {
    m_offset_col++;
    bool newRightInSubstrate = (getCols() > 0) ? __isColInSubstrate(getCols() - 1) : false;
    if (newRightInSubstrate) {
        if (getCols() < m_max_cols) {
            addColumnTail();
            __setColumnData(getCols() - 1);
        } else {
            delColumnHead();
            addColumnTail();
            __setColumnData(getCols() - 1);
        }
    } else {
        while (getCols() > 0 && !__isColInSubstrate(getCols() - 1)) {
            delColumnTail();
        }
    }
    __updateFromSubstrate();
}

void LimitedSlidingMatrix::printIndices () const {
    std::cout << "Matrix indices:" << std::endl;
    for (int r = 0; r < getRows(); r++) {
        for (int c = 0; c < getCols(); c++) {
            TileData td = get(r, c);
            std::cout << "(" << td.x << "," << td.y << ") ";
        }
        std::cout << std::endl;
    }
    std::cout << "Offset: (" << m_offset_col << "," << m_offset_row << "), Size: " << getCols() << "x" << getRows() << std::endl;
}

void LimitedSlidingMatrix::validateIndices () const {
    if (getRows() == 0 || getCols() == 0) {
        return;
    }
    for (int r = 0; r < getRows(); r++) {
        for (int c = 0; c < getCols(); c++) {
            TileData td = get(r, c);
            if (c > 0) {
                TileData prev = get(r, c - 1);
                if (td.x <= prev.x) {
                    std::cout << "VALIDATION FAILED" << std::endl;
                    exit(1);
                }
            }
            if (r > 0) {
                TileData prev = get(r - 1, c);
                if (td.y <= prev.y) {
                    std::cout << "VALIDATION FAILED" << std::endl;
                    exit(1);
                }
            }
        }
    }
    std::cout << "VALIDATION PASSED" << std::endl;
}

int getch () {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

int readArrowKey () {
    int ch = getch();
    if (ch == 27) {
        ch = getch();
        if (ch == -1) return 0;
        if (ch == 91) {
            ch = getch();
            if (ch == -1) return 0;
            switch (ch) {
                case 65: return 1;
                case 66: return 2;
                case 67: return 3;
                case 68: return 4;
            }
        }
    }
    return ch;
}

//================================================================================================================================
//=> - Main function -
//================================================================================================================================

int main (int argc, char* argv[]) {
    LimitedSlidingMatrix matrix(10, 10);
    std::cout << "SlidingMatrix test - Use arrow keys to slide, 'q' to quit" << std::endl;
    matrix.printIndices();
    matrix.validateIndices();
    while (true) {
        int key = readArrowKey();
        if (key == 'q' || key == 'Q') {
            break;
        }
        switch (key) {
            case 1:
                matrix.slideUp();
                matrix.printIndices();
                break;
            case 2:
                matrix.slideDown();
                matrix.printIndices();
                break;
            case 3:
                matrix.slideRight();
                matrix.printIndices();
                break;
            case 4:
                matrix.slideLeft();
                matrix.printIndices();
                break;
        }
        matrix.validateIndices();
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
