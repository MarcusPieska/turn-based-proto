//================================================================================================================================
// Texture tiler: exclusive tile blit plus seam-band flip from EdgeThread polylines. No path generation.
//================================================================================================================================

#ifndef BLEND_TILE_H
#define BLEND_TILE_H

#include <cstdint>
#include <vector>

#include "blend_thread.h"

//================================================================================================================================
//=> - Texture view -
//================================================================================================================================

struct TileTex {
    const uint8_t *px;                                                           // RGB packed, row-major
    int w;                                                                       // texture width
    int h;                                                                       // texture height
};

//================================================================================================================================
//=> - Class: BlendTile -
//================================================================================================================================

class BlendTile {
public:
    BlendTile ();
    ~BlendTile ();

    void compose (uint8_t *dst, int dw, int dh, const BlendMap &map, const EdgeThread &th,
                  const TileTex *tex, int n_tex, int fb, bool opt);

private:
    float path_y (const std::vector<Vx> &pts, float x) const;
    float path_x (const std::vector<Vx> &pts, float y) const;
    void  blit (uint8_t *dst, int dw, int dh, const BlendMap &map, const TileTex *tex, int n_tex, int fb) const;
    void  put (uint8_t *dst, int dw, int dh, const TileTex *tex, int n_tex, int id, int x, int y) const;
    void  seam (uint8_t *dst, int dw, int dh, const EdgeThread &th, const TileTex *tex, int n_tex, bool opt) const;

    mutable std::vector<float> m_lut;                                            // path sample scratch
};

#endif // BLEND_TILE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
