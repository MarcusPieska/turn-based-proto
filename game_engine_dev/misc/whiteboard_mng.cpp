//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "whiteboard_mng.h"

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cerrno>
#include <sys/stat.h>

//================================================================================================================================
//=> - WhiteboardMng statics -
//================================================================================================================================

u16 WhiteboardMng::m_w = 0;
u16 WhiteboardMng::m_h = 0;
u32 WhiteboardMng::m_tile_n = 0;
u32 WhiteboardMng::m_wh8_sz = 0;
void* WhiteboardMng::m_raw[WhiteboardMng::k_blk_n] = {};
u8* WhiteboardMng::m_mem[WhiteboardMng::k_blk_n] = {};
u8 WhiteboardMng::m_slot[WhiteboardMng::k_blk_n] = {};
u16 WhiteboardMng::m_alloc_n = 0;
u16 WhiteboardMng::m_slab_lim = 100u;
u32 WhiteboardMng::m_chkout = 0u;
bool WhiteboardMng::m_inited = false;

#ifdef WB_TRACE_DB
struct WbTraceNd {
    cstr cls;
    cstr fn;
    u32 turn;
    u8 sz_b;
    u8 blk;
    u8 sub;
    u8* ptr;
    WbTraceNd* next;
};
static WbTraceNd* g_wb_trace = nullptr;
#endif

static void wb_fatal (cstr msg) {
    std::fprintf(stderr, "WhiteboardMng fatal: %s\n", msg);
    std::abort();
}

static u32 wb_idx (u16 w, u32 x, u32 y) {
    return y * (u32)w + x;
}

//================================================================================================================================
//=> - WhiteboardMng -
//================================================================================================================================

void WhiteboardMng::init (u16 w, u16 h, u16 slab_lim) {
    if (w == 0u || h == 0u) {
        wb_fatal("init dimensions are zero");
    }
    if (slab_lim == 0u || slab_lim > k_blk_n) {
        wb_fatal("init slab_lim out of range");
    }
    if (m_inited) {
        wb_fatal("init called twice");
    }
    m_w = w;
    m_h = h;
    m_tile_n = (u32)w * (u32)h;
    m_wh8_sz = m_tile_n * 8u;
    m_slab_lim = slab_lim;
    m_alloc_n = 0;
    m_chkout = 0u;
    for (u16 i = 0; i < k_blk_n; ++i) {
        m_raw[i] = nullptr;
        m_mem[i] = nullptr;
        m_slot[i] = 0;
    }
    m_inited = true;
}

void WhiteboardMng::terminate () {
    if (!m_inited) {
        return;
    }
    for (u16 i = 0; i < m_alloc_n; ++i) {
        if (m_raw[i] != nullptr) {
            std::free(m_raw[i]);
            m_raw[i] = nullptr;
            m_mem[i] = nullptr;
        }
        m_slot[i] = 0;
    }
    m_alloc_n = 0;
    m_w = 0;
    m_h = 0;
    m_tile_n = 0;
    m_wh8_sz = 0;
    m_slab_lim = 100u;
    m_chkout = 0u;
    m_inited = false;
}

u32 WhiteboardMng::chkout () {
    return m_chkout;
}

u16 WhiteboardMng::slab_lim () {
    return m_slab_lim;
}

u16 WhiteboardMng::width () {
    return m_w;
}

u16 WhiteboardMng::height () {
    return m_h;
}

u32 WhiteboardMng::tile_n () {
    return m_tile_n;
}

WhiteboardMng::WbOut WhiteboardMng::fail_out () {
    WbOut o;
    o.ptr = nullptr;
    o.blk = 0;
    o.sub = 0;
    return o;
}

u8 WhiteboardMng::msk (u8 sz_b, u8 sub) {
    if (sz_b >= 8u) {
        return 0xFFu;
    }
    u8 m = (u8)(((1u << sz_b) - 1u) << sub);
    return m;
}

u8 WhiteboardMng::fill (u8 slot) {
    u8 n = 0;
    for (u8 b = 0; b < 8; ++b) {
        if ((slot & (u8)(1u << b)) != 0) {
            ++n;
        }
    }
    return n;
}

bool WhiteboardMng::fit (u8 slot, u8 sz_b, u8* out_sub) {
    if (sz_b == 1u) {
        for (u8 s = 0; s < 8u; ++s) {
            if ((slot & (u8)(1u << s)) == 0) {
                *out_sub = s;
                return true;
            }
        }
        return false;
    }
    if (sz_b >= 8u) {
        if (slot == 0) {
            *out_sub = 0;
            return true;
        }
        return false;
    }
    for (u8 s = 0; s <= 8u - sz_b; s += sz_b) {
        u8 need = msk(sz_b, s);
        if ((slot & need) == 0) {
            *out_sub = s;
            return true;
        }
    }
    return false;
}

u8* WhiteboardMng::new_blk (u8 blk) {
    if (blk >= k_blk_n || blk >= m_slab_lim) {
        return nullptr;
    }
    if (m_mem[blk] != nullptr) {
        return m_mem[blk];
    }
    u32 raw_sz = m_wh8_sz + 7u;
    void* raw = std::malloc(raw_sz);
    if (raw == nullptr) {
        return nullptr;
    }
    u64 addr = (u64)(uintptr_t)raw;
    u64 base = (addr + 7u) & ~7ull;
    m_raw[blk] = raw;
    m_mem[blk] = (u8*)base;
    if (blk >= m_alloc_n) {
        m_alloc_n = (u16)(blk + 1);
    }
    return m_mem[blk];
}

WhiteboardMng::WbOut WhiteboardMng::co (u8 sz_b) {
    if (!m_inited) {
        wb_fatal("checkout before init");
    }
    u16 best_blk = k_blk_n;
    u8 best_sub = 0;
    u8 best_fill = 0;
    for (u16 i = 0; i < m_alloc_n; ++i) {
        u8 sub = 0;
        if (fit(m_slot[i], sz_b, &sub)) {
            u8 f = fill(m_slot[i]);
            if (best_blk == k_blk_n || f > best_fill) {
                best_fill = f;
                best_blk = i;
                best_sub = sub;
            }
        }
    }
    if (best_blk == k_blk_n) {
        if (m_alloc_n >= m_slab_lim) {
            return fail_out();
        }
        best_blk = m_alloc_n;
        best_sub = 0;
        if (sz_b < 8u) {
            u8 sub = 0;
            if (!fit(0, sz_b, &sub)) {
                wb_fatal("checkout fresh slab fit failed");
            }
            best_sub = sub;
        }
        if (new_blk((u8)best_blk) == nullptr) {
            return fail_out();
        }
    }
    m_slot[best_blk] |= msk(sz_b, best_sub);
    WbOut o;
    o.blk = (u8)best_blk;
    o.sub = best_sub;
    o.ptr = m_mem[best_blk] + (u32)best_sub * m_tile_n;
    ++m_chkout;
    return o;
}

WhiteboardMng::WbOut WhiteboardMng::checkout_1b () {
    return co(1u);
}

WhiteboardMng::WbOut WhiteboardMng::checkout_2b () {
    return co(2u);
}

WhiteboardMng::WbOut WhiteboardMng::checkout_4b () {
    return co(4u);
}

WhiteboardMng::WbOut WhiteboardMng::checkout_8b () {
    return co(8u);
}

void WhiteboardMng::release (u8 sz_b, u8 blk, u8 sub) {
    if (!m_inited || blk >= m_alloc_n) {
        return;
    }
    m_slot[blk] &= (u8)~msk(sz_b, sub);
    if (m_chkout > 0u) {
        --m_chkout;
    }
}

u8* WhiteboardMng::get_slot_allocator () {
    return m_slot;
}

u16 WhiteboardMng::get_allocated_max () {
    return m_alloc_n;
}

//================================================================================================================================
//=> - Whiteboard_1B -
//================================================================================================================================

Whiteboard_1B::Whiteboard_1B (cstr cls, cstr fn, u32 turn)
    : m_p(nullptr), m_blk(0), m_sub(0), m_w(0), m_h(0), m_cls(cls), m_fn(fn), m_turn(turn), m_ok(false) {
    WhiteboardMng::WbOut o = WhiteboardMng::checkout_1b();
    m_p = o.ptr;
    m_blk = o.blk;
    m_sub = o.sub;
    m_w = WhiteboardMng::width();
    m_h = WhiteboardMng::height();
    m_ok = m_p != nullptr;
    WB_TRACE_LINK_1B(this);
}

Whiteboard_1B::~Whiteboard_1B () {
    WB_TRACE_UNLINK_1B(this);
    if (m_ok) {
        WhiteboardMng::release(1u, m_blk, m_sub);
    }
}

bool Whiteboard_1B::ok () const { return m_ok; }
u16 Whiteboard_1B::w () const { return m_w; }
u16 Whiteboard_1B::h () const { return m_h; }
u8* Whiteboard_1B::raw () const { return m_p; }
u8* Whiteboard_1B::get_iter_ptr () const { return m_ok ? m_p : nullptr; }
cstr Whiteboard_1B::cls () const { return m_cls; }
cstr Whiteboard_1B::fn () const { return m_fn; }
u32 Whiteboard_1B::turn () const { return m_turn; }
u8 Whiteboard_1B::blk () const { return m_blk; }
u8 Whiteboard_1B::sub () const { return m_sub; }

u8 Whiteboard_1B::rd (u32 x, u32 y) const {
    WB_TRACE_CHK_1B(this, x, y);
    return m_p[wb_idx(m_w, x, y)];
}

void Whiteboard_1B::wr (u32 x, u32 y, u8 v) {
    WB_TRACE_CHK_1B(this, x, y);
    m_p[wb_idx(m_w, x, y)] = v;
}

u8 Whiteboard_1B::rd_i (u32 i) const {
    return m_p[i];
}

void Whiteboard_1B::wr_i (u32 i, u8 v) {
    m_p[i] = v;
}

//================================================================================================================================
//=> - Whiteboard_2B -
//================================================================================================================================

Whiteboard_2B::Whiteboard_2B (cstr cls, cstr fn, u32 turn)
    : m_p(nullptr), m_blk(0), m_sub(0), m_w(0), m_h(0), m_cls(cls), m_fn(fn), m_turn(turn), m_ok(false) {
    WhiteboardMng::WbOut o = WhiteboardMng::checkout_2b();
    m_p = o.ptr;
    m_blk = o.blk;
    m_sub = o.sub;
    m_w = WhiteboardMng::width();
    m_h = WhiteboardMng::height();
    m_ok = m_p != nullptr;
    WB_TRACE_LINK_2B(this);
}

Whiteboard_2B::~Whiteboard_2B () {
    WB_TRACE_UNLINK_2B(this);
    if (m_ok) {
        WhiteboardMng::release(2u, m_blk, m_sub);
    }
}

bool Whiteboard_2B::ok () const { return m_ok; }
u16 Whiteboard_2B::w () const { return m_w; }
u16 Whiteboard_2B::h () const { return m_h; }
u8* Whiteboard_2B::raw () const { return m_p; }
u16* Whiteboard_2B::get_iter_ptr () const { return m_ok ? (u16*)m_p : nullptr; }
cstr Whiteboard_2B::cls () const { return m_cls; }
cstr Whiteboard_2B::fn () const { return m_fn; }
u32 Whiteboard_2B::turn () const { return m_turn; }
u8 Whiteboard_2B::blk () const { return m_blk; }
u8 Whiteboard_2B::sub () const { return m_sub; }

u16 Whiteboard_2B::rd (u32 x, u32 y) const {
    WB_TRACE_CHK_2B(this, x, y);
    u32 i = wb_idx(m_w, x, y);
    u16 v = m_p[i * 2u];
    v |= (u16)m_p[i * 2u + 1u] << 8;
    return v;
}

void Whiteboard_2B::wr (u32 x, u32 y, u16 v) {
    WB_TRACE_CHK_2B(this, x, y);
    u32 i = wb_idx(m_w, x, y);
    m_p[i * 2u] = (u8)(v & 0xFFu);
    m_p[i * 2u + 1u] = (u8)((v >> 8) & 0xFFu);
}

u16 Whiteboard_2B::rd_i (u32 i) const {
    u16 v = m_p[i * 2u];
    v |= (u16)m_p[i * 2u + 1u] << 8;
    return v;
}

void Whiteboard_2B::wr_i (u32 i, u16 v) {
    m_p[i * 2u] = (u8)(v & 0xFFu);
    m_p[i * 2u + 1u] = (u8)((v >> 8) & 0xFFu);
}

//================================================================================================================================
//=> - Whiteboard_4B -
//================================================================================================================================

Whiteboard_4B::Whiteboard_4B (cstr cls, cstr fn, u32 turn)
    : m_p(nullptr), m_blk(0), m_sub(0), m_w(0), m_h(0), m_cls(cls), m_fn(fn), m_turn(turn), m_ok(false) {
    WhiteboardMng::WbOut o = WhiteboardMng::checkout_4b();
    m_p = o.ptr;
    m_blk = o.blk;
    m_sub = o.sub;
    m_w = WhiteboardMng::width();
    m_h = WhiteboardMng::height();
    m_ok = m_p != nullptr;
    WB_TRACE_LINK_4B(this);
}

Whiteboard_4B::~Whiteboard_4B () {
    WB_TRACE_UNLINK_4B(this);
    if (m_ok) {
        WhiteboardMng::release(4u, m_blk, m_sub);
    }
}

bool Whiteboard_4B::ok () const { return m_ok; }
u16 Whiteboard_4B::w () const { return m_w; }
u16 Whiteboard_4B::h () const { return m_h; }
u8* Whiteboard_4B::raw () const { return m_p; }
u32* Whiteboard_4B::get_iter_ptr () const { return m_ok ? (u32*)m_p : nullptr; }
cstr Whiteboard_4B::cls () const { return m_cls; }
cstr Whiteboard_4B::fn () const { return m_fn; }
u32 Whiteboard_4B::turn () const { return m_turn; }
u8 Whiteboard_4B::blk () const { return m_blk; }
u8 Whiteboard_4B::sub () const { return m_sub; }

u32 Whiteboard_4B::rd (u32 x, u32 y) const {
    WB_TRACE_CHK_4B(this, x, y);
    u32 i = wb_idx(m_w, x, y);
    u32 v = m_p[i * 4u];
    v |= (u32)m_p[i * 4u + 1u] << 8;
    v |= (u32)m_p[i * 4u + 2u] << 16;
    v |= (u32)m_p[i * 4u + 3u] << 24;
    return v;
}

void Whiteboard_4B::wr (u32 x, u32 y, u32 v) {
    WB_TRACE_CHK_4B(this, x, y);
    u32 i = wb_idx(m_w, x, y);
    m_p[i * 4u] = (u8)(v & 0xFFu);
    m_p[i * 4u + 1u] = (u8)((v >> 8) & 0xFFu);
    m_p[i * 4u + 2u] = (u8)((v >> 16) & 0xFFu);
    m_p[i * 4u + 3u] = (u8)((v >> 24) & 0xFFu);
}

u32 Whiteboard_4B::rd_i (u32 i) const {
    u32 v = m_p[i * 4u];
    v |= (u32)m_p[i * 4u + 1u] << 8;
    v |= (u32)m_p[i * 4u + 2u] << 16;
    v |= (u32)m_p[i * 4u + 3u] << 24;
    return v;
}

void Whiteboard_4B::wr_i (u32 i, u32 v) {
    m_p[i * 4u] = (u8)(v & 0xFFu);
    m_p[i * 4u + 1u] = (u8)((v >> 8) & 0xFFu);
    m_p[i * 4u + 2u] = (u8)((v >> 16) & 0xFFu);
    m_p[i * 4u + 3u] = (u8)((v >> 24) & 0xFFu);
}

//================================================================================================================================
//=> - Whiteboard_8B -
//================================================================================================================================

Whiteboard_8B::Whiteboard_8B (cstr cls, cstr fn, u32 turn)
    : m_p(nullptr), m_blk(0), m_sub(0), m_w(0), m_h(0), m_cls(cls), m_fn(fn), m_turn(turn), m_ok(false) {
    WhiteboardMng::WbOut o = WhiteboardMng::checkout_8b();
    m_p = o.ptr;
    m_blk = o.blk;
    m_sub = o.sub;
    m_w = WhiteboardMng::width();
    m_h = WhiteboardMng::height();
    m_ok = m_p != nullptr;
    WB_TRACE_LINK_8B(this);
}

Whiteboard_8B::~Whiteboard_8B () {
    WB_TRACE_UNLINK_8B(this);
    if (m_ok) {
        WhiteboardMng::release(8u, m_blk, m_sub);
    }
}

bool Whiteboard_8B::ok () const { return m_ok; }
u16 Whiteboard_8B::w () const { return m_w; }
u16 Whiteboard_8B::h () const { return m_h; }
u8* Whiteboard_8B::raw () const { return m_p; }
u64* Whiteboard_8B::get_iter_ptr () const { return m_ok ? (u64*)m_p : nullptr; }
cstr Whiteboard_8B::cls () const { return m_cls; }
cstr Whiteboard_8B::fn () const { return m_fn; }
u32 Whiteboard_8B::turn () const { return m_turn; }
u8 Whiteboard_8B::blk () const { return m_blk; }
u8 Whiteboard_8B::sub () const { return m_sub; }

u64 Whiteboard_8B::rd (u32 x, u32 y) const {
    WB_TRACE_CHK_8B(this, x, y);
    u32 i = wb_idx(m_w, x, y);
    u64 v = 0;
    for (u32 b = 0; b < 8u; ++b) {
        v |= (u64)m_p[i * 8u + b] << (b * 8u);
    }
    return v;
}

void Whiteboard_8B::wr (u32 x, u32 y, u64 v) {
    WB_TRACE_CHK_8B(this, x, y);
    u32 i = wb_idx(m_w, x, y);
    for (u32 b = 0; b < 8u; ++b) {
        m_p[i * 8u + b] = (u8)((v >> (b * 8u)) & 0xFFu);
    }
}

u64 Whiteboard_8B::rd_i (u32 i) const {
    u64 v = 0;
    for (u32 b = 0; b < 8u; ++b) {
        v |= (u64)m_p[i * 8u + b] << (b * 8u);
    }
    return v;
}

void Whiteboard_8B::wr_i (u32 i, u64 v) {
    for (u32 b = 0; b < 8u; ++b) {
        m_p[i * 8u + b] = (u8)((v >> (b * 8u)) & 0xFFu);
    }
}

//================================================================================================================================
//=> - WhiteboardMngTestUtil -
//================================================================================================================================

u8* WhiteboardMngTestUtil::slot_allocator () {
    return WhiteboardMng::get_slot_allocator();
}

u16 WhiteboardMngTestUtil::allocated_max () {
    return WhiteboardMng::get_allocated_max();
}

#ifdef WB_TRACE_DB
static void wb_trace_push (cstr cls, cstr fn, u32 turn, u8 sz_b, u8 blk, u8 sub, u8* ptr) {
    WbTraceNd* nd = (WbTraceNd*)std::malloc(sizeof(WbTraceNd));
    if (nd == nullptr) {
        wb_fatal("trace node malloc failed");
    }
    nd->cls = cls ? cls : "";
    nd->fn = fn ? fn : "";
    nd->turn = turn;
    nd->sz_b = sz_b;
    nd->blk = blk;
    nd->sub = sub;
    nd->ptr = ptr;
    nd->next = g_wb_trace;
    g_wb_trace = nd;
}

static void wb_trace_pop (u8* ptr) {
    WbTraceNd** pp = &g_wb_trace;
    while (*pp != nullptr) {
        if ((*pp)->ptr == ptr) {
            WbTraceNd* dead = *pp;
            *pp = dead->next;
            std::free(dead);
            return;
        }
        pp = &(*pp)->next;
    }
}

static void wb_trace_chk (u16 w, u16 h, u32 x, u32 y) {
    if (x >= (u32)w || y >= (u32)h) {
        std::fprintf(stderr, "WhiteboardMng access OOB: (%u,%u) w=%u h=%u\n", x, y, w, h);
        std::abort();
    }
}

void WhiteboardMngTestUtil::link_1b (const Whiteboard_1B* wb) {
    if (wb == nullptr || !wb->ok()) return;
    wb_trace_push(wb->cls(), wb->fn(), wb->turn(), 1u, wb->blk(), wb->sub(), wb->raw());
}

void WhiteboardMngTestUtil::unlink_1b (const Whiteboard_1B* wb) {
    if (wb == nullptr || !wb->ok()) return;
    wb_trace_pop(wb->raw());
}

void WhiteboardMngTestUtil::chk_1b (const Whiteboard_1B* wb, u32 x, u32 y) {
    if (wb == nullptr) return;
    wb_trace_chk(wb->w(), wb->h(), x, y);
}

void WhiteboardMngTestUtil::link_2b (const Whiteboard_2B* wb) {
    if (wb == nullptr || !wb->ok()) return;
    wb_trace_push(wb->cls(), wb->fn(), wb->turn(), 2u, wb->blk(), wb->sub(), wb->raw());
}

void WhiteboardMngTestUtil::unlink_2b (const Whiteboard_2B* wb) {
    if (wb == nullptr || !wb->ok()) return;
    wb_trace_pop(wb->raw());
}

void WhiteboardMngTestUtil::chk_2b (const Whiteboard_2B* wb, u32 x, u32 y) {
    if (wb == nullptr) return;
    wb_trace_chk(wb->w(), wb->h(), x, y);
}

void WhiteboardMngTestUtil::link_4b (const Whiteboard_4B* wb) {
    if (wb == nullptr || !wb->ok()) return;
    wb_trace_push(wb->cls(), wb->fn(), wb->turn(), 4u, wb->blk(), wb->sub(), wb->raw());
}

void WhiteboardMngTestUtil::unlink_4b (const Whiteboard_4B* wb) {
    if (wb == nullptr || !wb->ok()) return;
    wb_trace_pop(wb->raw());
}

void WhiteboardMngTestUtil::chk_4b (const Whiteboard_4B* wb, u32 x, u32 y) {
    if (wb == nullptr) return;
    wb_trace_chk(wb->w(), wb->h(), x, y);
}

void WhiteboardMngTestUtil::link_8b (const Whiteboard_8B* wb) {
    if (wb == nullptr || !wb->ok()) return;
    wb_trace_push(wb->cls(), wb->fn(), wb->turn(), 8u, wb->blk(), wb->sub(), wb->raw());
}

void WhiteboardMngTestUtil::unlink_8b (const Whiteboard_8B* wb) {
    if (wb == nullptr || !wb->ok()) return;
    wb_trace_pop(wb->raw());
}

void WhiteboardMngTestUtil::chk_8b (const Whiteboard_8B* wb, u32 x, u32 y) {
    if (wb == nullptr) return;
    wb_trace_chk(wb->w(), wb->h(), x, y);
}
#endif

void WhiteboardMngTestUtil::dump_inv () {
#ifdef WB_TRACE_DB
    u32 n = 0;
    for (WbTraceNd* nd = g_wb_trace; nd != nullptr; nd = nd->next) {
        ++n;
    }
    std::printf("Whiteboard checkout inventory: %u active\n", n);
    for (WbTraceNd* nd = g_wb_trace; nd != nullptr; nd = nd->next) {
        std::printf("  %ub blk=%u sub=%u turn=%u cls=%s fn=%s ptr=%p\n",
            nd->sz_b, nd->blk, nd->sub, nd->turn, nd->cls, nd->fn, (void*)nd->ptr);
    }
#else
    std::printf("Whiteboard checkout inventory: WB_TRACE_DB not enabled\n");
#endif
}

struct WbHeld {
    u8 sz_b;
    u8 blk;
    u8 sub;
    u8* ptr;
};

static const u32 k_seq_stk_n = 256u;
static WbHeld g_seq_stk[k_seq_stk_n];
static u32 g_seq_stk_n = 0;
static WbRetPol g_ret_pol = RET_RIGHT;
static FILE* g_seq_tr = nullptr;

static void seq_putc (char c) {
    std::putchar(c);
    if (g_seq_tr != nullptr) {
        std::fputc(c, g_seq_tr);
    }
}

static void seq_puts (cstr s) {
    if (s == nullptr) {
        return;
    }
    std::fputs(s, stdout);
    if (g_seq_tr != nullptr) {
        std::fputs(s, g_seq_tr);
    }
}

static void seq_printf (cstr fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    std::vprintf(fmt, ap);
    va_end(ap);
    if (g_seq_tr != nullptr) {
        std::vfprintf(g_seq_tr, fmt, ap2);
    }
    va_end(ap2);
}

static const cstr k_trace_dir = "wb_vtest_trace";

static void seq_tr_mkdir () {
    if (mkdir(k_trace_dir, 0755) != 0 && errno != EEXIST) {
        std::fprintf(stderr, "WhiteboardMngTestUtil: mkdir %s failed\n", k_trace_dir);
    }
}

static bool seq_tr_open (cstr tag) {
    if (tag == nullptr || tag[0] == 0) {
        return false;
    }
    seq_tr_mkdir();
    char path[192];
    u32 n = 0;
    for (u32 i = 0; k_trace_dir[i] != 0 && n + 1u < sizeof(path); ++i) {
        path[n++] = k_trace_dir[i];
    }
    path[n++] = '/';
    cstr pfx = "wb_vtest_";
    for (u32 i = 0; pfx[i] != 0 && n + 1u < sizeof(path); ++i) {
        path[n++] = pfx[i];
    }
    for (u32 i = 0; tag[i] != 0 && n + 1u < sizeof(path); ++i) {
        path[n++] = tag[i];
    }
    cstr sfx = ".trace";
    for (u32 i = 0; sfx[i] != 0 && n + 1u < sizeof(path); ++i) {
        path[n++] = sfx[i];
    }
    path[n] = 0;
    g_seq_tr = std::fopen(path, "w");
    return g_seq_tr != nullptr;
}

static void seq_tr_close () {
    if (g_seq_tr != nullptr) {
        std::fclose(g_seq_tr);
        g_seq_tr = nullptr;
    }
}

void WhiteboardMngTestUtil::mk_trace_tag (cstr label, char* out, u32 cap) {
    if (out == nullptr || cap == 0u) {
        return;
    }
    out[0] = 0;
    if (label == nullptr) {
        return;
    }
    u32 n = 0;
    for (u32 i = 0; label[i] != 0 && n + 1u < cap; ++i) {
        char c = label[i];
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c - 'A' + 'a');
        }
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            out[n++] = c;
        } else {
            if (n > 0u && out[n - 1u] != '_') {
                out[n++] = '_';
            }
        }
    }
    while (n > 0u && out[n - 1u] == '_') {
        --n;
    }
    out[n] = 0;
}

static void seq_tr_hdr (cstr label, cstr tag, const i8* seq, u32 sn, u16 w, u16 h, WbRetPol ret, u16 slab_lim) {
    if (g_seq_tr == nullptr) {
        return;
    }
    std::fprintf(g_seq_tr, "# label: %s\n", label ? label : "");
    std::fprintf(g_seq_tr, "# tag: %s\n", tag ? tag : "");
    std::fprintf(g_seq_tr, "# map: %ux%u\n", w, h);
    std::fprintf(g_seq_tr, "# slab_lim: %u\n", slab_lim);
    std::fprintf(g_seq_tr, "# ret: %s\n", WhiteboardMngTestUtil::ret_pol_nm(ret));
    std::fprintf(g_seq_tr, "# steps: %u\n", sn);
    std::fprintf(g_seq_tr, "# seq:");
    for (u32 i = 0; i < sn; ++i) {
        std::fprintf(g_seq_tr, " %d", (int)seq[i]);
    }
    std::fprintf(g_seq_tr, "\n");
}

static u32 seq_find_ret (u8 sz_b) {
    u32 m[k_seq_stk_n];
    u32 mn = 0;
    for (u32 i = 0; i < g_seq_stk_n; ++i) {
        if (g_seq_stk[i].sz_b == sz_b) {
            m[mn++] = i;
        }
    }
    if (mn == 0u) {
        return g_seq_stk_n;
    }
    if (g_ret_pol == RET_LEFT) {
        return m[0];
    }
    if (g_ret_pol == RET_RIGHT) {
        return m[mn - 1u];
    }
    return m[mn / 2u];
}

static bool seq_rm_at (u32 k, WbHeld* out) {
    if (k >= g_seq_stk_n) {
        return false;
    }
    *out = g_seq_stk[k];
    while (k < g_seq_stk_n - 1u) {
        g_seq_stk[k] = g_seq_stk[k + 1u];
        ++k;
    }
    --g_seq_stk_n;
    return true;
}

cstr WhiteboardMngTestUtil::ret_pol_nm (WbRetPol ret) {
    if (ret == RET_LEFT) {
        return "LEFT";
    }
    if (ret == RET_MID) {
        return "MID";
    }
    return "RIGHT";
}

u8 WhiteboardMngTestUtil::i8_cmd_sz (i8 cmd) {
    i8 a = cmd < 0 ? (i8)(-cmd) : cmd;
    if (a == 1 || a == 2 || a == 4 || a == 8) {
        return (u8)a;
    }
    return 0;
}

bool WhiteboardMngTestUtil::i8_cmd_co (i8 cmd) {
    return cmd > 0;
}

void WhiteboardMngTestUtil::seq_reset () {
    while (g_seq_stk_n > 0u) {
        --g_seq_stk_n;
        WbHeld* h = &g_seq_stk[g_seq_stk_n];
        WhiteboardMng::release(h->sz_b, h->blk, h->sub);
    }
}

bool WhiteboardMngTestUtil::seq_step (i8 cmd) {
    u8 sz_b = i8_cmd_sz(cmd);
    if (sz_b == 0u) {
        return false;
    }
    if (i8_cmd_co(cmd)) {
        WhiteboardMng::WbOut o;
        if (sz_b == 1u) {
            o = WhiteboardMng::checkout_1b();
        } else if (sz_b == 2u) {
            o = WhiteboardMng::checkout_2b();
        } else if (sz_b == 4u) {
            o = WhiteboardMng::checkout_4b();
        } else {
            o = WhiteboardMng::checkout_8b();
        }
        if (o.ptr == nullptr) {
            return false;
        }
        if (g_seq_stk_n >= k_seq_stk_n) {
            return false;
        }
        WbHeld* h = &g_seq_stk[g_seq_stk_n];
        h->sz_b = sz_b;
        h->blk = o.blk;
        h->sub = o.sub;
        h->ptr = o.ptr;
        ++g_seq_stk_n;
        return true;
    }
    u32 k = seq_find_ret(sz_b);
    if (k >= g_seq_stk_n) {
        return false;
    }
    WbHeld h;
    if (!seq_rm_at(k, &h)) {
        return false;
    }
    WhiteboardMng::release(h.sz_b, h.blk, h.sub);
    return true;
}

static void pr_slot_byte (u8 v) {
    for (u8 b = 0; b < 8u; ++b) {
        if ((v & (u8)(1u << b)) != 0) {
            seq_putc('1');
        } else {
            seq_putc('0');
        }
    }
}

void WhiteboardMngTestUtil::pr_step_line (i8 cmd) {
    u8 sz_b = i8_cmd_sz(cmd);
    if (sz_b == 0u) {
        seq_printf("?%d invalid\n", (int)cmd);
        return;
    }
    if (i8_cmd_co(cmd)) {
        seq_printf("+%uB ", sz_b);
    } else {
        seq_printf("-%uB ", sz_b);
    }
    u16 n = allocated_max();
    u8* sl = slot_allocator();
    for (u16 i = 0; i < n; ++i) {
        if (i > 0u) {
            seq_putc(' ');
        }
        pr_slot_byte(sl[i]);
    }
    seq_putc('\n');
}

void WhiteboardMngTestUtil::wait_enter () {
    if (std::getenv("WB_VTEST_AUTO") != nullptr) {
        return;
    }
    std::printf("press enter to continue...\n");
    std::fflush(stdout);
    std::getchar();
}

void WhiteboardMngTestUtil::run_seq (cstr label, cstr tag, const i8* seq, u32 n, u16 w, u16 h, WbRetPol ret, u16 slab_lim) {
    g_ret_pol = ret;
    char tag_buf[96];
    cstr use_tag = tag;
    if (use_tag == nullptr || use_tag[0] == 0) {
        mk_trace_tag(label, tag_buf, 96u);
        use_tag = tag_buf;
    }
    seq_reset();
    WhiteboardMng::terminate();
    if (!seq_tr_open(use_tag)) {
        std::fprintf(stderr, "WhiteboardMngTestUtil: trace open failed %s/wb_vtest_%s.trace\n", k_trace_dir, use_tag);
    }
    seq_tr_hdr(label, use_tag, seq, n, w, h, ret, slab_lim);
    WhiteboardMng::init(w, h, slab_lim);
    seq_printf("\n=== %s ===\n", label ? label : "");
    seq_printf("map %ux%u  slabs=%u  ret=%s  (chunk bits left=sub0)\n", w, h, slab_lim, ret_pol_nm(ret));
    for (u32 i = 0; i < n; ++i) {
        if (!seq_step(seq[i])) {
            seq_printf("*** step %u failed cmd=%d\n", i, (int)seq[i]);
        }
        pr_step_line(seq[i]);
    }
    seq_tr_close();
    wait_enter();
    seq_reset();
    WhiteboardMng::terminate();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
