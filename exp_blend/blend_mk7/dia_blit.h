//================================================================================================================================
// Pixel-first diamond blit: each pixel finds a home subtile, then samples that terrain texture.
//================================================================================================================================

#ifndef DIA_BLIT_H
#define DIA_BLIT_H

#include <cstdint>

#include "dia_cfg.h"
#include "dia_map.h"
#include "dia_grid.h"
#include "dia_dat15.h"

//================================================================================================================================
//=> - Texture view -
//================================================================================================================================

struct DiaTex {
    const uint8_t *px; // RGB packed, row-major
    int w; // texture width
    int h; // texture height
};

//================================================================================================================================
//=> - Class: DiaBlit -
//================================================================================================================================

class DiaBlit {
public:
    DiaBlit ();
    ~DiaBlit ();

    void bind (const DiaTex *tex, int n);
    void bind_banks (const DiaDat15 *const *banks, int n);
    void run (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg, const DiaGrid &grid) const;

private:
    bool in_dia (int cx, int cy, int x, int y) const;
    bool owns (const BlendMap &map, const DiaGrid &grid, int r, int c, int x, int y) const;
    void home (Vx ctr, int x, int y, int &si, int &sj) const;
    Rgb sample (int id, int tr, int tc, int lx, int ly) const;
    void extend_down (uint8_t *dst, int dw, int dh) const;

    const DiaTex *m_tex; // flat PNG textures
    const DiaDat15 *const *m_banks; // game .dat15 banks
    int m_n; // texture / bank count
};

#endif // DIA_BLIT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
