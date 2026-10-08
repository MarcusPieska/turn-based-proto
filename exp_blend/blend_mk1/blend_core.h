//================================================================================================================================
// Debug mesh + ownership clip (flat id bake). Threading lives in blend_thread; texture blit in blend_tile.
//================================================================================================================================

#ifndef BLEND_CORE_H
#define BLEND_CORE_H

#include <vector>

#include "blend_thread.h"

//================================================================================================================================
//=> - Class: SubtileMesh -
//================================================================================================================================

class SubtileMesh {
public:
    SubtileMesh ();
    ~SubtileMesh ();

    void build (const BlendMap &map);
    int  n_tile () const;
    int  n_vx () const;
    const Vx* tile_vx (int r, int c) const;

private:
    int m_rows;                                                                  // tile rows
    int m_cols;                                                                  // tile cols
    int m_nv;                                                                    // verts per tile side (DIV+1)
    std::vector<Vx> m_vx;                                                        // all tile lattices, packed
};

//================================================================================================================================
//=> - Class: BlendClip -
//================================================================================================================================

class BlendClip {
public:
    BlendClip ();
    ~BlendClip ();

    void run (const BlendMap &map, const EdgeThread &th, ClipStyle style);
    int  get (int x, int y) const;
    int  w () const;
    int  h () const;

private:
    float path_y (const std::vector<Vx> &pts, float x) const;
    float path_x (const std::vector<Vx> &pts, float y) const;
    void  mark_seg (std::vector<uint8_t> &seam, Vx a, Vx b) const;
    void  mark_seams (std::vector<uint8_t> &seam, const EdgeThread &th) const;
    void  apply_h (const EdgePath &ep);
    void  apply_v (const EdgePath &ep);
    void  flood (const BlendMap &map, const EdgeThread &th);

    int m_w;                                                                     // pixel width
    int m_h;                                                                     // pixel height
    std::vector<int> m_ter;                                                      // clipped terrain ids per pixel
};

#endif // BLEND_CORE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
