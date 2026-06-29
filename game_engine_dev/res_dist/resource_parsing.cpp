//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "resource_parsing.h"
#include "resource_types.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - ResParser token tables -
//================================================================================================================================

typedef struct ResTokRow {
    cstr m_tok;
    u8 m_val;
} ResTokRow;

static const ResTokRow k_terr_tok[] = {
    {"ALL_TERRAINS", RES_TERR_ALL},
    {"TERR_OCEAN", TERR_OCEAN[0]},
    {"TERR_SEA", TERR_SEA[0]},
    {"TERR_COASTAL", TERR_COASTAL[0]},
    {"TERR_PLAINS", TERR_PLAINS[0]},
    {"TERR_HILLS", TERR_HILLS[0]},
    {"TERR_MOUNTAINS", TERR_MOUNTAINS[0]},
};

static const ResTokRow k_clim_tok[] = {
    {"ALL_CLIMATES", RES_CLIM_ALL},
    {"CLIMATE_TUNDRA", CLIMATE_TUNDRA},
    {"CLIMATE_GRASSLAND", CLIMATE_GRASSLAND},
    {"CLIMATE_PLAINS", CLIMATE_PLAINS},
    {"CLIMATE_DESERT", CLIMATE_DESERT},
};

static const ResTokRow k_ov_tok[] = {
    {"ALL_OVERLAYS", RES_OV_ALL},
    {"NO_OVERLAYS", RES_OV_NONE},
    {"OVERLAY_SWAMPS", RES_OV_SWAMPS},
    {"OVERLAY_FORESTS", RES_OV_FORESTS},
    {"OVERLAY_JUNGLES", RES_OV_JUNGLES},
    {"OVERLAY_RIVERS", RES_OV_RIVERS},
};

//================================================================================================================================
//=> - ResParser helpers -
//================================================================================================================================

static void trim_sp (char* s) {
    if (s == nullptr) {
        return;
    }
    u32 n = (u32)std::strlen(s);
    u32 b = 0;
    while (b < n && std::isspace(static_cast<unsigned char>(s[b]))) {
        ++b;
    }
    u32 e = n;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) {
        --e;
    }
    u32 j = 0;
    for (u32 i = b; i < e; ++i) {
        s[j++] = s[i];
    }
    s[j] = '\0';
}

static bool tok_to_u8 (cstr tok, const ResTokRow* rows, u32 row_n, u8* out) {
    if (tok == nullptr || out == nullptr) {
        return false;
    }
    for (u32 i = 0; i < row_n; ++i) {
        if (std::strcmp(tok, rows[i].m_tok) == 0) {
            *out = rows[i].m_val;
            return true;
        }
    }
    return false;
}

static bool parse_u16_tok (cstr tok, u16* out) {
    if (tok == nullptr || out == nullptr || tok[0] == '\0') {
        return false;
    }
    char* end = nullptr;
    const unsigned long v = std::strtoul(tok, &end, 10);
    if (end == tok) {
        return false;
    }
    *out = (u16)v;
    return true;
}

static bool parse_tech (cstr tok, char* out, u32 cap) {
    if (tok == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    const char* pfx = "tech(";
    const u32 pfx_n = (u32)std::strlen(pfx);
    if (std::strncmp(tok, pfx, pfx_n) != 0) {
        return false;
    }
    const u32 tok_n = (u32)std::strlen(tok);
    if (tok_n <= pfx_n + 1u || tok[tok_n - 1] != ')') {
        return false;
    }
    const u32 body_n = tok_n - pfx_n - 1u;
    if (body_n + 1u > cap) {
        return false;
    }
    std::memcpy(out, tok + pfx_n, body_n);
    out[body_n] = '\0';
    return true;
}

static bool parse_quad_inner (cstr inner, ResQuad* q) {
    if (inner == nullptr || q == nullptr) {
        return false;
    }
    char buf[256];
    if (std::strlen(inner) >= sizeof(buf)) {
        return false;
    }
    std::snprintf(buf, sizeof(buf), "%s", inner);
    char* parts[4] = {nullptr, nullptr, nullptr, nullptr};
    u32 pn = 0;
    char* p = buf;
    while (pn < 4u) {
        parts[pn++] = p;
        char* sep = std::strchr(p, ':');
        if (sep == nullptr) {
            break;
        }
        *sep = '\0';
        p = sep + 1;
    }
    if (pn != 4u) {
        return false;
    }
    for (u32 i = 0; i < 4u; ++i) {
        trim_sp(parts[i]);
    }
    if (!tok_to_u8(parts[0], k_terr_tok, (u32)(sizeof(k_terr_tok) / sizeof(k_terr_tok[0])), &q->m_terr)) {
        return false;
    }
    if (!tok_to_u8(parts[1], k_clim_tok, (u32)(sizeof(k_clim_tok) / sizeof(k_clim_tok[0])), &q->m_clim)) {
        return false;
    }
    if (!tok_to_u8(parts[2], k_ov_tok, (u32)(sizeof(k_ov_tok) / sizeof(k_ov_tok[0])), &q->m_ov)) {
        return false;
    }
    return parse_u16_tok(parts[3], &q->m_wt);
}

static bool parse_quad_tok (cstr tok, ResQuad* q) {
    if (tok == nullptr || q == nullptr || tok[0] != '(') {
        return false;
    }
    const u32 n = (u32)std::strlen(tok);
    if (n < 3u || tok[n - 1] != ')') {
        return false;
    }
    char inner[256];
    const u32 inner_n = n - 2u;
    if (inner_n >= sizeof(inner)) {
        return false;
    }
    std::memcpy(inner, tok + 1, inner_n);
    inner[inner_n] = '\0';
    return parse_quad_inner(inner, q);
}

static cstr skip_ws (cstr s) {
    if (s == nullptr) {
        return nullptr;
    }
    while (*s != '\0' && std::isspace(static_cast<unsigned char>(*s))) {
        ++s;
    }
    return s;
}

static bool parse_plc_tail (cstr tail, ResPlacement* plc) {
    if (tail == nullptr || plc == nullptr) {
        return false;
    }
    tail = skip_ws(tail);
    if (tail[0] == '\0') {
        return false;
    }
    char gw_buf[16];
    u32 gi = 0;
    while (tail[gi] != '\0' && tail[gi] != ':') {
        ++gi;
    }
    if (gi >= sizeof(gw_buf)) {
        return false;
    }
    std::memcpy(gw_buf, tail, gi);
    gw_buf[gi] = '\0';
    trim_sp(gw_buf);
    if (!parse_u16_tok(gw_buf, &plc->m_res_wt)) {
        return false;
    }
    tail = skip_ws(tail + gi);
    if (tail[0] == ':') {
        ++tail;
    }
    plc->m_quad_n = 0;
    while (plc->m_quad_n < RES_IO_QUAD_MAX) {
        tail = skip_ws(tail);
        if (tail[0] != '(') {
            break;
        }
        u32 dep = 0;
        u32 qi = 0;
        while (tail[qi] != '\0') {
            if (tail[qi] == '(') {
                ++dep;
            } else if (tail[qi] == ')') {
                --dep;
                if (dep == 0) {
                    ++qi;
                    break;
                }
            }
            ++qi;
        }
        if (dep != 0 || qi >= 256u) {
            return false;
        }
        char quad_tok[256];
        if (qi >= sizeof(quad_tok)) {
            return false;
        }
        std::memcpy(quad_tok, tail, qi);
        quad_tok[qi] = '\0';
        if (!parse_quad_tok(quad_tok, &plc->m_quads[plc->m_quad_n])) {
            return false;
        }
        ++plc->m_quad_n;
        tail = skip_ws(tail + qi);
        if (tail[0] == ':') {
            ++tail;
        }
    }
    return plc->m_quad_n > 0;
}

static bool next_field (cstr* io, char* out, u32 cap) {
    if (io == nullptr || *io == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    cstr s = skip_ws(*io);
    if (s[0] == '\0') {
        return false;
    }
    u32 dep = 0;
    u32 n = 0;
    for (; s[n] != '\0'; ++n) {
        if (s[n] == '(') {
            ++dep;
        } else if (s[n] == ')') {
            if (dep > 0) {
                --dep;
            }
        } else if (s[n] == ':' && dep == 0) {
            break;
        }
    }
    if (n >= cap) {
        return false;
    }
    std::memcpy(out, s, n);
    out[n] = '\0';
    trim_sp(out);
    if (s[n] == ':') {
        *io = s + n + 1;
    } else {
        *io = s + n;
    }
    return out[0] != '\0';
}

static bool parse_line (cstr line, ResEntry* out) {
    if (line == nullptr || out == nullptr) {
        return false;
    }
    cstr cur = line;
    char fld[512];
    if (!next_field(&cur, fld, sizeof(fld))) {
        return false;
    }
    if (std::strlen(fld) >= RES_IO_NAME_MAX) {
        return false;
    }
    std::memcpy(out->m_name, fld, std::strlen(fld) + 1u);
    if (!next_field(&cur, fld, sizeof(fld)) || !parse_u16_tok(fld, &out->m_food)) {
        return false;
    }
    if (!next_field(&cur, fld, sizeof(fld)) || !parse_u16_tok(fld, &out->m_shields)) {
        return false;
    }
    if (!next_field(&cur, fld, sizeof(fld)) || !parse_u16_tok(fld, &out->m_commerce)) {
        return false;
    }
    if (!next_field(&cur, fld, sizeof(fld)) || !parse_tech(fld, out->m_tech, RES_IO_TECH_MAX)) {
        return false;
    }
    out->m_has_plc = 0;
    cur = skip_ws(cur);
    if (cur[0] != '\0') {
        if (parse_plc_tail(cur, &out->m_plc)) {
            out->m_has_plc = 1;
        }
    }
    return true;
}

//================================================================================================================================
//=> - ResParser -
//================================================================================================================================

ResParser::ResParser () {
    m_cat.m_n = 0;
    m_cat.m_items = nullptr;
}

ResParser::~ResParser () {
    release();
}

void ResParser::rst () {
    m_cat.m_n = 0;
    m_cat.m_items = nullptr;
}

void ResParser::release () {
    if (m_cat.m_items != nullptr) {
        delete[] m_cat.m_items;
    }
    rst();
}

bool ResParser::parse_file (const ResIoFile& file) {
    release();
    const u32 ln = file.line_n();
    if (ln == 0 || ln > RES_IO_ENTRY_MAX) {
        return false;
    }
    m_cat.m_items = new ResEntry[ln]();
    if (m_cat.m_items == nullptr) {
        return false;
    }
    for (u32 i = 0; i < ln; ++i) {
        if (!parse_line(file.line(i), &m_cat.m_items[i])) {
            release();
            return false;
        }
        ++m_cat.m_n;
    }
    return true;
}

const ResCatalog& ResParser::catalog () const {
    return m_cat;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
