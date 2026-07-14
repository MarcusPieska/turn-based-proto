//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WHITEBOARD_MNG_H
#define WHITEBOARD_MNG_H

#include "game_primitives.h"

class Whiteboard_1B;
class Whiteboard_2B;
class Whiteboard_4B;
class Whiteboard_8B;
class WhiteboardMngTestUtil;

//================================================================================================================================
//=> - WhiteboardMng -
//================================================================================================================================
//
//  Buddy allocator over u64-aligned wh8 slabs (w*h*8 bytes). Slab count capped at init; each u8 in m_slot tracks 8 wh1 sub-slots.
//  Checkout prefers the fullest slab that still fits; returns null ptr on exhaustion (wrappers expose ok()).
//  chkout() counts live RAII handles; slot_use() counts wh1 sub-slots set in m_slot (ones in the slot bitmap).
//
//================================================================================================================================

class WhiteboardMng {
public:
    static void init (u16 w, u16 h, u16 slab_lim = 100u);
    static void terminate ();
    static u16 width ();
    static u16 height ();
    static u32 tile_n ();
    static u16 slab_lim ();
    static u32 chkout ();
    static u16 slot_use ();
    static u16 slab_n ();

protected:
    struct WbOut {
        u8* ptr;
        u8 blk;
        u8 sub;
    };

    static WbOut checkout_1b ();
    static WbOut checkout_2b ();
    static WbOut checkout_4b ();
    static WbOut checkout_8b ();
    static void release (u8 sz_b, u8 blk, u8 sub);
    static u8* get_slot_allocator ();
    static u16 get_allocated_max ();

    friend class Whiteboard_1B;
    friend class Whiteboard_2B;
    friend class Whiteboard_4B;
    friend class Whiteboard_8B;
    friend class WhiteboardMngTestUtil;

private:
    WhiteboardMng () = delete;

    static WbOut co (u8 sz_b);
    static WbOut fail_out ();
    static bool fit (u8 slot, u8 sz_b, u8* out_sub);
    static u8 pop_slot (u8 slot);
    static u8 fill (u8 slot);
    static u8 msk (u8 sz_b, u8 sub);
    static u8* new_blk (u8 blk);

    static const u16 k_blk_n = 100u;

    static u16 m_w;
    static u16 m_h;
    static u32 m_tile_n;
    static u32 m_wh8_sz;
    static void* m_raw[k_blk_n];
    static u8* m_mem[k_blk_n];
    static u8 m_slot[k_blk_n];
    static u16 m_alloc_n;
    static u16 m_slab_lim;
    static u32 m_chkout;
    static bool m_inited;
};

//================================================================================================================================
//=> - Whiteboard_1B -
//================================================================================================================================
//
//  RAII checkout of one byte per tile; returns its slab sub-slot on destruction.
//
//================================================================================================================================

class Whiteboard_1B {
public:
    Whiteboard_1B (cstr cls, cstr fn, u32 turn);
    ~Whiteboard_1B ();
    Whiteboard_1B (const Whiteboard_1B& o) = delete;
    Whiteboard_1B& operator= (const Whiteboard_1B& o) = delete;
    bool ok () const;
    u16 w () const;
    u16 h () const;
    u8 rd (u32 x, u32 y) const;
    void wr (u32 x, u32 y, u8 v);
    u8 rd_i (u32 i) const;
    void wr_i (u32 i, u8 v);
    u8* raw () const;
    u8* get_iter_ptr () const;
    cstr cls () const;
    cstr fn () const;
    u32 turn () const;
    u8 blk () const;
    u8 sub () const;

private:
    u8* m_p;
    u8 m_blk;
    u8 m_sub;
    u16 m_w;
    u16 m_h;
    cstr m_cls;
    cstr m_fn;
    u32 m_turn;
    bool m_ok;
};

//================================================================================================================================
//=> - Whiteboard_2B -
//================================================================================================================================

class Whiteboard_2B {
public:
    Whiteboard_2B (cstr cls, cstr fn, u32 turn);
    ~Whiteboard_2B ();
    Whiteboard_2B (const Whiteboard_2B& o) = delete;
    Whiteboard_2B& operator= (const Whiteboard_2B& o) = delete;
    bool ok () const;
    u16 w () const;
    u16 h () const;
    u16 rd (u32 x, u32 y) const;
    void wr (u32 x, u32 y, u16 v);
    u16 rd_i (u32 i) const;
    void wr_i (u32 i, u16 v);
    u8* raw () const;
    u16* get_iter_ptr () const;
    cstr cls () const;
    cstr fn () const;
    u32 turn () const;
    u8 blk () const;
    u8 sub () const;

    friend void whiteboard_2b_swap (Whiteboard_2B& a, Whiteboard_2B& b) noexcept;

private:
    u8* m_p;
    u8 m_blk;
    u8 m_sub;
    u16 m_w;
    u16 m_h;
    cstr m_cls;
    cstr m_fn;
    u32 m_turn;
    bool m_ok;
};

//================================================================================================================================
//=> - Whiteboard_4B -
//================================================================================================================================

class Whiteboard_4B {
public:
    Whiteboard_4B (cstr cls, cstr fn, u32 turn);
    ~Whiteboard_4B ();
    Whiteboard_4B (const Whiteboard_4B& o) = delete;
    Whiteboard_4B& operator= (const Whiteboard_4B& o) = delete;
    bool ok () const;
    u16 w () const;
    u16 h () const;
    u32 rd (u32 x, u32 y) const;
    void wr (u32 x, u32 y, u32 v);
    u32 rd_i (u32 i) const;
    void wr_i (u32 i, u32 v);
    u8* raw () const;
    u32* get_iter_ptr () const;
    cstr cls () const;
    cstr fn () const;
    u32 turn () const;
    u8 blk () const;
    u8 sub () const;

private:
    u8* m_p;
    u8 m_blk;
    u8 m_sub;
    u16 m_w;
    u16 m_h;
    cstr m_cls;
    cstr m_fn;
    u32 m_turn;
    bool m_ok;
};

//================================================================================================================================
//=> - Whiteboard_8B -
//================================================================================================================================

class Whiteboard_8B {
public:
    Whiteboard_8B (cstr cls, cstr fn, u32 turn);
    ~Whiteboard_8B ();
    Whiteboard_8B (const Whiteboard_8B& o) = delete;
    Whiteboard_8B& operator= (const Whiteboard_8B& o) = delete;
    bool ok () const;
    u16 w () const;
    u16 h () const;
    u64 rd (u32 x, u32 y) const;
    void wr (u32 x, u32 y, u64 v);
    u64 rd_i (u32 i) const;
    void wr_i (u32 i, u64 v);
    u8* raw () const;
    u64* get_iter_ptr () const;
    cstr cls () const;
    cstr fn () const;
    u32 turn () const;
    u8 blk () const;
    u8 sub () const;

private:
    u8* m_p;
    u8 m_blk;
    u8 m_sub;
    u16 m_w;
    u16 m_h;
    cstr m_cls;
    cstr m_fn;
    u32 m_turn;
    bool m_ok;
};

//================================================================================================================================
//=> - WhiteboardMngTestUtil -
//================================================================================================================================

enum WbRetPol {
    RET_LEFT,
    RET_RIGHT,
    RET_MID
};

class WhiteboardMngTestUtil {
public:
    static u8* slot_allocator ();
    static u16 allocated_max ();
    static void dump_inv ();
    static u8 i8_cmd_sz (i8 cmd);
    static bool i8_cmd_co (i8 cmd);
    static void seq_reset ();
    static bool seq_step (i8 cmd);
    static void pr_step_line (i8 cmd);
    static void run_seq (cstr label, cstr tag, const i8* seq, u32 n, u16 w, u16 h, WbRetPol ret, u16 slab_lim = 100u);
    static void mk_trace_tag (cstr label, char* out, u32 cap);
    static cstr ret_pol_nm (WbRetPol ret);
    static void wait_enter ();

#ifdef WB_TRACE_DB
    static void link_1b (const Whiteboard_1B* wb);
    static void unlink_1b (const Whiteboard_1B* wb);
    static void chk_1b (const Whiteboard_1B* wb, u32 x, u32 y);
    static void link_2b (const Whiteboard_2B* wb);
    static void unlink_2b (const Whiteboard_2B* wb);
    static void chk_2b (const Whiteboard_2B* wb, u32 x, u32 y);
    static void link_4b (const Whiteboard_4B* wb);
    static void unlink_4b (const Whiteboard_4B* wb);
    static void chk_4b (const Whiteboard_4B* wb, u32 x, u32 y);
    static void link_8b (const Whiteboard_8B* wb);
    static void unlink_8b (const Whiteboard_8B* wb);
    static void chk_8b (const Whiteboard_8B* wb, u32 x, u32 y);
#endif
};

#ifdef WB_TRACE_DB
#define WB_TRACE_LINK_1B(wb) WhiteboardMngTestUtil::link_1b(wb)
#define WB_TRACE_UNLINK_1B(wb) WhiteboardMngTestUtil::unlink_1b(wb)
#define WB_TRACE_CHK_1B(wb, x, y) WhiteboardMngTestUtil::chk_1b(wb, x, y)
#define WB_TRACE_LINK_2B(wb) WhiteboardMngTestUtil::link_2b(wb)
#define WB_TRACE_UNLINK_2B(wb) WhiteboardMngTestUtil::unlink_2b(wb)
#define WB_TRACE_CHK_2B(wb, x, y) WhiteboardMngTestUtil::chk_2b(wb, x, y)
#define WB_TRACE_LINK_4B(wb) WhiteboardMngTestUtil::link_4b(wb)
#define WB_TRACE_UNLINK_4B(wb) WhiteboardMngTestUtil::unlink_4b(wb)
#define WB_TRACE_CHK_4B(wb, x, y) WhiteboardMngTestUtil::chk_4b(wb, x, y)
#define WB_TRACE_LINK_8B(wb) WhiteboardMngTestUtil::link_8b(wb)
#define WB_TRACE_UNLINK_8B(wb) WhiteboardMngTestUtil::unlink_8b(wb)
#define WB_TRACE_CHK_8B(wb, x, y) WhiteboardMngTestUtil::chk_8b(wb, x, y)
#else
#define WB_TRACE_LINK_1B(wb)
#define WB_TRACE_UNLINK_1B(wb)
#define WB_TRACE_CHK_1B(wb, x, y)
#define WB_TRACE_LINK_2B(wb)
#define WB_TRACE_UNLINK_2B(wb)
#define WB_TRACE_CHK_2B(wb, x, y)
#define WB_TRACE_LINK_4B(wb)
#define WB_TRACE_UNLINK_4B(wb)
#define WB_TRACE_CHK_4B(wb, x, y)
#define WB_TRACE_LINK_8B(wb)
#define WB_TRACE_UNLINK_8B(wb)
#define WB_TRACE_CHK_8B(wb, x, y)
#endif

#endif // WHITEBOARD_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
