//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SECTOR_PATH_GRADIENTS_H
#define SECTOR_PATH_GRADIENTS_H

#include "game_primitives.h"
#include "walk_p2p.h"

class GameState;
class RuntimeStatics;
class Whiteboard_1B;
class Whiteboard_2B;

//================================================================================================================================
//=> - SectorPathGradients -
//================================================================================================================================
//
//  Sector path-gradient bank (impl via SECTOR_PATH_GRADIENTS_IMPL; default mk03). setup() measures
//  sector AABBs and chunk sizes. Two process-wide pool banks share slots across all war camps.
//  lease / lease_multi pin a slot; fill() Dials into the chunk (sector-clipped). mk03 packs chunks
//  into heap turn/rem banks. pack_path(): full Dial then keep a dir tape in a one-chunk (k_path).
//
//================================================================================================================================

class SectorPathGradients {
public:
    static const u16 k_cap = 20u;
    static const u16 k_pool_n = 2u;
    static const u16 k_wh_cap = 16u;
    static const u16 k_bank_mul = 4u;
    static const u16 k_multi_cap = 8u;
    static const u16 k_opt_n = WalkP2P::k_opt_n;

    typedef WalkP2P::StepRes StepRes;

    SectorPathGradients ();
    ~SectorPathGradients ();

    static bool setup (const Whiteboard_2B& sec, u16 sec_n);
    static void teardown ();
    static bool setup_ok ();
    static bool pool_setup (const Whiteboard_2B& sec, u16 sec_n);
    static void pool_teardown ();
    static SectorPathGradients* pool_bank (u16 i);
    static SectorPathGradients* pool_acq (u16* out_slot);

    bool bind (const Whiteboard_2B* sec);
    void clr ();
    u16 free_slot () const;

    bool lease (u16 slot, u16 sector);
    bool lease_multi (u16 slot, const u16* sectors, u16 n);
    void release (u16 slot);

    bool fill (
        u16 slot,
        GameState& st,
        const RuntimeStatics& rst,
        u16 sx,
        u16 sy,
        u16 dx,
        u16 dy,
        u16 seat,
        u8 acc);
    bool pack_path (
        u16 slot,
        GameState& st,
        const RuntimeStatics& rst,
        u16 sx,
        u16 sy,
        u16 dx,
        u16 dy,
        u16 seat,
        u8 acc);

    bool covers (u16 slot, u16 x, u16 y) const;
    bool clip_mask (u16 slot, Whiteboard_1B& mask) const;

    bool ok (u16 i) const;
    u16 chunk_n () const;
    u16 wb_n () const;
    StepRes peek (u16 i, const GameState& s, u16 x, u16 y) const;
    void peek_opts (u16 i, const GameState& s, u16 x, u16 y, u16* ox, u16* oy) const;

private:
    SectorPathGradients (const SectorPathGradients& o) = delete;
    SectorPathGradients (SectorPathGradients&& o) = delete;
    SectorPathGradients& operator= (const SectorPathGradients& o) = delete;
    SectorPathGradients& operator= (SectorPathGradients&& o) = delete;

    enum : u8 { k_off = 0u, k_one = 1u, k_dual = 2u, k_full = 3u, k_path = 4u };

    bool ens_bank (u16** tn, u16** rm, u16* n, u16 bi, u32 cells);
    void drop_bank (u16** tn, u16** rm, u16* n, u16* use, u16 bi);
    bool take_chunk (u8 kind, u16 slot, u16 ox, u16 oy, u16 cw, u16 ch);
    bool take_full (u16 slot);
    bool slot_ok (u16 i) const;
    u16* turn_ptr (u16 i) const;
    u16* rem_ptr (u16 i) const;
    StepRes peek_path (u16 i, const GameState& s, u16 x, u16 y) const;

    static bool s_ok;
    static u16 s_cw;
    static u16 s_ch;
    static u16 s_dw;
    static u16 s_dh;
    static u16 s_map_w;
    static u16 s_map_h;
    static u32 s_tile_n;
    static u16 s_sec_n;
    static u16 s_fit_one;
    static u16 s_fit_dual;
    static u16* s_min_x;
    static u16* s_min_y;
    static u16* s_max_x;
    static u16* s_max_y;
    static SectorPathGradients s_pool[k_pool_n]; // Process-wide shared banks
    static bool s_pool_on;

    u16* m_tn_one[k_wh_cap]; // Packed turn banks (fit_one chunks each)
    u16* m_rm_one[k_wh_cap]; // Packed rem banks
    u16 m_use_one[k_wh_cap];
    u16 m_n_one;
    u16* m_tn_dual[k_wh_cap]; // Packed dual turn banks
    u16* m_rm_dual[k_wh_cap]; // Packed dual rem banks
    u16 m_use_dual[k_wh_cap];
    u16 m_n_dual;
    WalkP2P* m_full[k_cap]; // Exclusive full-map fallback per slot

    u16* m_own_one;
    u16 m_own_one_n;
    u16* m_own_dual;
    u16 m_own_dual_n;

    u8 m_mode[k_cap];
    u16 m_wh_i[k_cap];
    u16 m_chk_i[k_cap];
    u16 m_ox[k_cap];
    u16 m_oy[k_cap];
    u16 m_cw[k_cap];
    u16 m_ch[k_cap];
    u16 m_sa[k_cap]; // Leased sector A (0-based); U16_KEY_NULL if none
    u16 m_sb[k_cap]; // Leased sector B for dual; U16_KEY_NULL if single
    u16 m_pn[k_cap]; // Packed path step count (k_path)
    u16 m_sx[k_cap]; // Packed path origin x (k_path)
    u16 m_sy[k_cap]; // Packed path origin y (k_path)
    mutable u16 m_pi[k_cap]; // Next tape index (k_path cursor)
    mutable u16 m_ax[k_cap]; // Current path vertex x (k_path)
    mutable u16 m_ay[k_cap]; // Current path vertex y (k_path)
    u8 m_on[k_cap];
    const Whiteboard_2B* m_sec;
};

#endif // SECTOR_PATH_GRADIENTS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
