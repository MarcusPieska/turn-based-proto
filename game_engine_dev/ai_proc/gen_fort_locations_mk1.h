//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_FORT_LOCATIONS_MK1_H
#define GEN_FORT_LOCATIONS_MK1_H

#include "game_primitives.h"
#include "whiteboard_mng.h"

class GameArraySimple;

//================================================================================================================================
//=> - GenFortLocations -
//================================================================================================================================
//
//  Patches flood mountains/volcano plus connected inland sea/lake; tile idx lists give O(1)
//  patch/edge lookup. find_passes marks shore around that core, walks, mountain-only passage.
//  Needs WhiteboardMng::init (4x Whiteboard_2B + 1x Whiteboard_1B).
//
//================================================================================================================================

#define GFL_IDX_NONE 0u

struct GflFragInfo {
    u8 m_loop; // 1 when fragment has no degree-1 endpoint
    u16 m_sx; // Walk seed column
    u16 m_sy; // Walk seed row
    u16 m_steps; // Max walk count written on this fragment
    u16 m_pid; // Mountain patch for water-edge grace; 0 if none
};

class GenFortLocationsMk1 {
public:
    GenFortLocationsMk1 ();
    ~GenFortLocationsMk1 ();

    bool begin (const GameArraySimple& map);
    bool index_patches (const GameArraySimple& map);
    bool mark_frags (const GameArraySimple& map);
    bool walk_frags (const GameArraySimple& map);
    bool find_passes (const GameArraySimple& map);
    void clr ();

    bool ok () const;
    u16 patch_n () const;
    u16 frag_n () const;
    u32 pass_n () const;
    const Whiteboard_2B& patches () const;
    const Whiteboard_2B& frags () const;
    const Whiteboard_2B& edges () const;
    const Whiteboard_2B& walks () const;
    const Whiteboard_1B& passes () const;
    const GflFragInfo* frag_info (u16 frag_id) const;

private:
    GenFortLocationsMk1 (const GenFortLocationsMk1& other) = delete;
    GenFortLocationsMk1& operator= (const GenFortLocationsMk1& other) = delete;
    GenFortLocationsMk1 (GenFortLocationsMk1&& other) = delete;
    GenFortLocationsMk1& operator= (GenFortLocationsMk1&& other) = delete;

    void clr_info ();
    void clr_psz ();
    void clr_ptiles ();
    void clr_etiles ();
    void grow_info (u16 need_n);
    void grow_plists (u16 need_n);
    bool collect_patch_tiles (const GameArraySimple& map, u16 pid, u32 seed_i, u32* q, u8* vis);
    u32 split_patch (const GameArraySimple& map, u16 pid, u32* q, u8* vis, u16* out_new);
    bool mark_patch_frags (const GameArraySimple& map, u16 pid, u32* q);
    bool walk_patch_frags (const GameArraySimple& map, u16 pid);
    bool find_best_pass (const GameArraySimple& map, u16 pid, u16* out_d, u32* out_a, u32* out_b,
        u16* out_cx, u16* out_cy, u32* q, u32* par);
    bool proc_patch (const GameArraySimple& map, u16 pid, u32 n, u32* q, u32* par, u8* vis);
    bool walk_frag (const GameArraySimple& map, u16 frag_id);

    Whiteboard_2B m_patch; // Patch id on mountains/volcano and enclosed inland sea/lake
    Whiteboard_2B m_frag; // Shore edge id per tile; equals owning mountain patch id
    Whiteboard_2B m_edge; // Shore candidate mask during mark_frags; unused afterward
    Whiteboard_2B m_walk; // Walk step count on m_frag tiles; 0 if none
    Whiteboard_1B m_pass; // 1 on mountain passage tiles
    GflFragInfo* m_info; // Per-fragment state; index 1..m_frag_n
    u32* m_psz; // Per-patch fill tile count; index 1..m_patch_n
    u32** m_ptile; // Per-patch fill tile idx; O(1) via m_ptile[pid]
    u32** m_pmtn; // Per-patch mountain tile idx only
    u32* m_pmnn; // Per-patch mountain tile count
    u32** m_etile; // Per-edge/frag shore tile idx; O(1) via m_etile[fid]
    u32* m_etn; // Per-edge/frag shore tile count
    u16 m_plist_cap; // Allocated slots in m_ptile/m_pmtn/m_etile rows
    u16 m_patch_n; // Patch count (ids 1..m_patch_n)
    u16 m_frag_n; // Fragment count (ids 1..m_frag_n)
    u32 m_pass_n; // Passage tile count
    bool m_ok; // True after begin with live whiteboards
};

#endif // GEN_FORT_LOCATIONS_MK1_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
