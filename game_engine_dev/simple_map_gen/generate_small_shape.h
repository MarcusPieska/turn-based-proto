//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_SMALL_SHAPE_H
#define GENERATE_SMALL_SHAPE_H

#include "game_primitives.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - SmallShapeParams -
//================================================================================================================================

struct SmallShapeParams {
    u32 m_seed = 0;
    u16 m_width = 20;
    u16 m_height = 10;
    i32 m_angle_deg = 0;
};

//================================================================================================================================
//=> - Generate_SmallShape -
//================================================================================================================================

class Generate_SmallShape {
public:
    explicit Generate_SmallShape (const SmallShapeParams& params);

    bool generate ();
    bool is_valid () const;
    bool save_output (cstr path) const;

    u16 width () const;
    u16 height () const;
    u16 canvas_size () const;
    const MapArrayTerrain& terrain () const;

private:
    Generate_SmallShape (const Generate_SmallShape& other) = delete;
    Generate_SmallShape (Generate_SmallShape&& other) = delete;

    SmallShapeParams m_params;
    u16 m_canvas_sz;
    bool m_valid_generation;
    MapArrayTerrain m_terrain;
};

#endif // GENERATE_SMALL_SHAPE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
