//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_MTN_PASSES_H
#define GEN_MTN_PASSES_H

#include "game_primitives.h"
#include "whiteboard_mng.h"

class GameArraySimple;

//================================================================================================================================
//=> - GenMtnPasses -
//================================================================================================================================
//
//  Index mountain patches (mountains/volcano plus enclosed inland sea/lake). For patches at or above
//  GFL_MIN_PATCH_TILES, flood perimeter, scan with 11x11 window (partial strip when step-adjacent).
//  Each pass path also picks one non-resource fort step. Needs WhiteboardMng::init (2x Whiteboard_2B + 3x Whiteboard_1B).
//
//================================================================================================================================

#define GFL_IDX_NONE 0u

class GenMtnPasses {
public:
    GenMtnPasses ();
    ~GenMtnPasses ();

    bool begin (const GameArraySimple& map);
    bool index_patches (const GameArraySimple& map);
    bool find_passes (const GameArraySimple& map);
    void clr ();

    bool ok () const;
    u16 patch_n () const;
    u32 pass_n () const;
    u32 pass_fort_n () const;
    u32 patch_sz (u16 pid) const;
    u16 max_walk () const;
    const Whiteboard_2B& patches () const;
    const Whiteboard_2B& walks () const;
    const Whiteboard_1B& passes () const;
    const Whiteboard_1B& pass_forts () const;

private:
    GenMtnPasses (const GenMtnPasses& other) = delete;
    GenMtnPasses& operator= (const GenMtnPasses& other) = delete;
    GenMtnPasses (GenMtnPasses&& other) = delete;
    GenMtnPasses& operator= (GenMtnPasses&& other) = delete;

    void clr_psz ();
    void grow_psz (u16 need_n);
    bool proc_patch (const GameArraySimple& map, u16 pid, u32* q, u8* vis, u32* par);

    Whiteboard_2B m_patch; // Patch id on mountains/volcano and enclosed inland sea/lake
    Whiteboard_2B m_walk; // Perimeter walk step; 0 if none
    Whiteboard_1B m_wdir; // Entry dir 1..8 into walk tile; 0 if none
    Whiteboard_1B m_pass; // 1 on mountain passage tiles
    Whiteboard_1B m_pass_fort; // 1 on chosen fort step per pass path
    u32* m_psz; // Per-patch fill tile count; index 1..m_patch_n
    u16 m_psz_cap; // Allocated slots in m_psz
    u16 m_patch_n; // Patch count (ids 1..m_patch_n)
    u32 m_pass_n; // Passage tile count
    u32 m_pass_fort_n; // Pass fort step count
    u16 m_max_walk; // Max perimeter walk step written
    bool m_ok; // True after begin with live whiteboards
};

#endif // GEN_MTN_PASSES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
