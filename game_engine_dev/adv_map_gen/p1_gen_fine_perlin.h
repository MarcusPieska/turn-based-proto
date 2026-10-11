//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_FINE_PERLIN_H
#define P1_GEN_FINE_PERLIN_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_FinePerlin -
//================================================================================================================================
//
//  Compress src Perlin to w/4 x h/4 (tile A), H-flip to B, row ABAB as C, V-flip C to D, then rows
//  CDCD into caller out_fine; also builds gray viz in result.m_ov.
//
//================================================================================================================================

struct P1_Gen_FinePerlinRslt {
    u16 m_w; // Map width
    u16 m_h; // Map height
    u16 m_tw; // Compressed tile width (w/4)
    u16 m_th; // Compressed tile height (h/4)
    MapArrayOverlay m_ov; // Gray viz of fine field
};

class P1_Gen_FinePerlin {
public:
    explicit P1_Gen_FinePerlin (const P1_RunPrm& prm);

    bool generate (const f32* src, u16 w, u16 h, f32* out_fine);
    bool is_valid () const;
    const P1_Gen_FinePerlinRslt& result () const;

private:
    P1_Gen_FinePerlin (const P1_Gen_FinePerlin& other) = delete;
    P1_Gen_FinePerlin (P1_Gen_FinePerlin&& other) = delete;

    P1_RunPrm m_prm; // Run seed/size
    bool m_ok; // True after successful generate
    P1_Gen_FinePerlinRslt m_rslt; // Gray viz and sizes
};

#endif // P1_GEN_FINE_PERLIN_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
