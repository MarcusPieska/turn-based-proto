//================================================================================================================================
// CPU-side .dat15 tile bank reader (same format as game_view_mk1 Dat15Reader, no SDL).
//================================================================================================================================

#ifndef DIA_DAT15_H
#define DIA_DAT15_H

#include <vector>
#include <string>
#include <cstdint>

#include "dia_cfg.h"

//================================================================================================================================
//=> - Class: DiaDat15 -
//================================================================================================================================

class DiaDat15 {
public:
    DiaDat15 ();
    ~DiaDat15 ();

    bool ld (const std::string &path, int tw, int th);
    int n_rows () const;
    int n_cols () const;
    int tw () const;
    int th () const;
    Rgb sample (int row, int col, int lx, int ly) const;

private:
    const uint8_t *item (int row, int col) const;

    int m_rows; // bank rows
    int m_cols; // bank cols
    int m_tw; // tile width px
    int m_th; // tile height px
    std::vector<std::vector<uint8_t>> m_rgb; // each tw*th*3
};

#endif // DIA_DAT15_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
