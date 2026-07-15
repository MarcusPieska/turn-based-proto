//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "data_parser_base.h"
#include "game_map_defs.h"
#include "item_effect_handler.h"

typedef struct PlcTokRow {
    cstr m_tok;
    u8 m_val;
} PlcTokRow;

static const PlcTokRow k_terr_tok[] = {
    {"ALL_TERRAINS", PLC_TERR_ALL},
    {"TERR_OCEAN", TERR_OCEAN[0]},
    {"TERR_SEA", TERR_SEA[0]},
    {"TERR_COASTAL", TERR_COASTAL[0]},
    {"TERR_PLAINS", TERR_PLAINS[0]},
    {"TERR_HILLS", TERR_HILLS[0]},
    {"TERR_MOUNTAINS", TERR_MOUNTAINS[0]},
};

static const PlcTokRow k_clim_tok[] = {
    {"ALL_CLIMATES", PLC_CLIM_ALL},
    {"CLIMATE_GRASSLAND", CLIMATE_GRASSLAND},
    {"CLIMATE_PLAINS", CLIMATE_PLAINS},
    {"CLIMATE_DESERT", CLIMATE_DESERT},
};

static const PlcTokRow k_ov_tok[] = {
    {"ALL_OVERLAYS", PLC_OV_ALL},
    {"NO_OVERLAYS", PLC_OV_REQ_NONE},
    {"OVERLAY_SWAMPS", OV_SWAMP[0]},
    {"OVERLAY_FORESTS", OV_FOREST[0]},
    {"OVERLAY_JUNGLES", OV_JUNGLE[0]},
    {"OVERLAY_RIVERS", PLC_OV_RIVERS},
};

namespace {

void trim_ws_idx (StringManager& m, u32 i) {
    m.trim_head_char(i, ' ');
    m.trim_tail_char(i, ' ');
    m.trim_head_char(i, '\t');
    m.trim_tail_char(i, '\t');
    m.trim_head_char(i, '\r');
    m.trim_tail_char(i, '\r');
    m.trim_head_char(i, '\n');
    m.trim_tail_char(i, '\n');
}

void trim_ws_all (StringManager& m) {
    for (u32 i = 0; i < m.get_string_count(); ++i) {
        trim_ws_idx(m, i);
    }
}

bool streq_trimmed (cstr a, cstr b) {
    StringManager am;
    am.load_cstr_content(a ? a : "");
    trim_ws_idx(am, 0);
    StringManager bm;
    bm.load_cstr_content(b ? b : "");
    trim_ws_idx(bm, 0);
    return std::strcmp(am.get_string_content(0), bm.get_string_content(0)) == 0;
}

bool is_empty_trimmed (cstr in) {
    StringManager m;
    m.load_cstr_content(in ? in : "");
    trim_ws_idx(m, 0);
    return m.get_string_content(0)[0] == '\0';
}

}

static bool plc_tok_u8 (cstr tok, const PlcTokRow* rows, u32 row_n, u8* out) {
    if (!tok || !out) return false;
    for (u32 i = 0; i < row_n; ++i) {
        if (std::strcmp(tok, rows[i].m_tok) == 0) {
            *out = rows[i].m_val;
            return true;
        }
    }
    return false;
}

static bool plc_u16_tok (cstr tok, u16* out) {
    if (!tok || !out || tok[0] == '\0') return false;
    char* end = 0;
    const unsigned long v = std::strtoul(tok, &end, 10);
    if (end == tok) return false;
    *out = (u16)v;
    return true;
}

static bool plc_quad_inner (cstr inner, ResQuad* q) {
    if (!inner || !q) return false;
    StringManager parts;
    parts.load_cstr_content(inner);
    parts.split_string_by_char(0, ',');
    trim_ws_all(parts);
    parts.cull_empty_strings();
    if (parts.get_string_count() != 4u) return false;
    if (!plc_tok_u8(parts.get_string_content(0), k_terr_tok, (u32)(sizeof(k_terr_tok) / sizeof(k_terr_tok[0])), &q->m_terr)) return false;
    if (!plc_tok_u8(parts.get_string_content(1), k_clim_tok, (u32)(sizeof(k_clim_tok) / sizeof(k_clim_tok[0])), &q->m_clim)) return false;
    if (!plc_tok_u8(parts.get_string_content(2), k_ov_tok, (u32)(sizeof(k_ov_tok) / sizeof(k_ov_tok[0])), &q->m_ov)) return false;
    return plc_u16_tok(parts.get_string_content(3), &q->m_wt);
}

static bool plc_quad_tok (cstr tok, ResQuad* q) {
    if (!tok || !q) return false;
    StringManager quad;
    quad.load_cstr_content(tok);
    trim_ws_idx(quad, 0);
    cstr s = quad.get_string_content(0);
    if (s[0] != '(') return false;
    const u32 n = (u32)std::strlen(s);
    if (n < 3u || s[n - 1] != ')') return false;
    quad.split_string_by_char(0, '(');
    if (quad.get_string_count() < 2u) return false;
    StringManager inside;
    inside.load_cstr_content(quad.get_string_content(1));
    inside.split_string_by_char(0, ')');
    inside.cull_empty_strings();
    if (inside.get_string_count() == 0) return false;
    return plc_quad_inner(inside.get_string_content(0), q);
}

//================================================================================================================================
//=> - DataParserBase implementation -
//================================================================================================================================

u32 DataParserBase::m_error_count = 0;
ItemEffectHandler* DataParserBase::s_item_effect_handler = 0;
const StringManager* DataParserBase::s_effect_definition_items = 0;

void DataParserBase::set_item_effect_handler (const NameToIdxCbs* name_to_idx_cbs, const StringManager* effect_defs) {
    if (s_item_effect_handler != 0) {
        return;
    }
    if (effect_defs == 0 || effect_defs->get_string_count() == 0 || name_to_idx_cbs == 0) {
        printf("ERROR: DataParserBase set_item_effect_handler: invalid parameters\n");
        ++m_error_count;
        return;
    }
    s_item_effect_handler = new ItemEffectHandler(name_to_idx_cbs);
    s_effect_definition_items = effect_defs;
}

void DataParserBase::clear_item_effect_handler () {
    if (s_item_effect_handler != 0) {
        delete s_item_effect_handler;
        s_item_effect_handler = 0;
    }
    s_effect_definition_items = 0;
}

void DataParserBase::derive_names_from_raw_lines (const StringManager& raw_lines, StringManager& out_names) const {
    u32 n = raw_lines.get_string_count();
    u32 total = 0;
    for (u32 i = 0; i < n; ++i) {
        StringManager parts;
        parts.load_cstr_content(raw_lines.get_string_content(i));
        parts.split_string_by_char(0, ':');
        if (parts.get_string_count() == 0) continue;
        parts.trim_head_char(0, ' ');
        parts.trim_tail_char(0, ' ');
        parts.trim_head_char(0, '\t');
        parts.trim_tail_char(0, '\t');
        parts.trim_head_char(0, '\r');
        parts.trim_tail_char(0, '\r');
        total += (u32)std::strlen(parts.get_string_content(0)) + 1;
    }
    if (total == 0) {
        out_names.load_cstr_content("");
        out_names.cull_empty_strings();
        return;
    }
    char* buf = new char[(std::size_t)total];
    u32 k = 0;
    for (u32 i = 0; i < n; ++i) {
        StringManager parts;
        parts.load_cstr_content(raw_lines.get_string_content(i));
        parts.split_string_by_char(0, ':');
        if (parts.get_string_count() == 0) continue;
        parts.trim_head_char(0, ' ');
        parts.trim_tail_char(0, ' ');
        parts.trim_head_char(0, '\t');
        parts.trim_tail_char(0, '\t');
        parts.trim_head_char(0, '\r');
        parts.trim_tail_char(0, '\r');
        cstr name = parts.get_string_content(0);
        std::size_t len = std::strlen(name);
        std::memcpy(buf + k, name, len);
        k += (u32)len;
        buf[k++] = '\n';
    }
    if (k > 0) --k;
    buf[k] = '\0';
    out_names.load_cstr_content(buf);
    out_names.split_string_by_char(0, '\n');
    out_names.cull_empty_strings();
    delete[] buf;
}

DataParserBase::DataParserBase (const StringManager& raw_lines, const NameToIdxCbs& name_to_idx_cbs) :
    m_name_to_idx_cbs(name_to_idx_cbs),
    m_item_count(0) {
    if (raw_lines.get_string_count() == 0) {
        printf("ERROR: DataParserBase received empty raw items\n");
        ++m_error_count;
        return;
    }
    u32 total = 0;
    for (u32 i = 0; i < raw_lines.get_string_count(); ++i) {
        total += (u32)std::strlen(raw_lines.get_string_content(i)) + 1;
    }
    char* buf = new char[(std::size_t)total];
    u32 k = 0;
    for (u32 i = 0; i < raw_lines.get_string_count(); ++i) {
        cstr line = raw_lines.get_string_content(i);
        std::size_t len = std::strlen(line);
        std::memcpy(buf + k, line, len);
        k += (u32)len;
        buf[k++] = '\n';
    }
    if (k > 0) --k;
    buf[k] = '\0';
    m_raw_lines.load_cstr_content(buf);
    m_raw_lines.split_string_by_char(0, '\n');
    m_raw_lines.cull_empty_strings();
    delete[] buf;

    derive_names_from_raw_lines(m_raw_lines, m_names);

    u16 upper_limit = (u16)-1;
    if (m_raw_lines.get_string_count() > upper_limit) {
        printf("ERROR: DataParserBase received too many raw items\n");
        ++m_error_count;
        m_item_count = upper_limit;
        return;
    }
    m_item_count = (u16)m_raw_lines.get_string_count();
}

u16 DataParserBase::name_to_idx (cstr name) const {
    StringManager target;
    target.load_cstr_content(name ? name : "");
    trim_ws_idx(target, 0);
    cstr target_s = target.get_string_content(0);
    for (u32 i = 0; i < m_item_count; ++i) {
        if (streq_trimmed(m_names.get_string_content(i), target_s)) {
            return (u16)i;
        }
    }
    printf("ERROR: DataParserBase could not map name '%s' to index\n", name ? name : "");
    ++m_error_count;
    return U16_KEY_NULL;
}

cstr DataParserBase::idx_to_name (u16 idx) const {
    if (idx >= m_item_count) {
        printf("ERROR: DataParserBase could not map index '%u' to name\n", idx);
        ++m_error_count;
        return "";
    }
    return m_names.get_string_content(idx);
}

void DataParserBase::check_errors () {
    if (m_error_count > 0) {
        printf("ERROR: DataParserBase encountered %u parsing error(s)\n", (unsigned int)m_error_count);
        std::exit(1);
    }
}

u32 DataParserBase::get_error_count_for_tests () {
    return m_error_count;
}

void DataParserBase::reset_error_count_for_tests () {
    m_error_count = 0;
}

const StringManager& DataParserBase::get_raw_lines () const {
    return m_raw_lines;
}

const StringManager& DataParserBase::get_names () const {
    return m_names;
}

void DataParserBase::get_line_items (cstr line, StringManager& out_items) const {
    out_items.load_cstr_content(line ? line : "");
    out_items.split_string_by_char(0, ':');
    for (u32 i = 0; i < out_items.get_string_count(); ++i) {
        out_items.trim_head_char(i, ' ');
        out_items.trim_tail_char(i, ' ');
        out_items.trim_head_char(i, '\t');
        out_items.trim_tail_char(i, '\t');
        out_items.trim_head_char(i, '\r');
        out_items.trim_tail_char(i, '\r');
        out_items.trim_head_char(i, '\n');
        out_items.trim_tail_char(i, '\n');
    }
}

u16 DataParserBase::parse_u16 (const StringManager& line_items, u16 start_idx) const {
    return (u16)std::strtoul(line_items.get_string_content(start_idx), 0, 10);
}

u32 DataParserBase::parse_u32 (const StringManager& line_items, u16 start_idx) const {
    return (u32)std::strtoul(line_items.get_string_content(start_idx), 0, 10);
}

u16 DataParserBase::parse_unit_type (const StringManager& line_items, u16 start_idx) const {
    return m_name_to_idx_cbs.unit_type_name_to_idx(line_items.get_string_content(start_idx));
}

ItemEffectsStruct DataParserBase::parse_item_effects (const StringManager& line_items, u16 start_idx) const {
    (void)start_idx;
    ItemEffectsStruct effects = {};
    if (s_effect_definition_items == 0 || s_effect_definition_items->get_string_count() == 0) {
        printf("ERROR: DataParserBase parse_item_effects: effect defs table not set or empty\n");
        ++m_error_count;
        return effects;
    }
    if (line_items.get_string_count() == 0) {
        printf("ERROR: DataParserBase parse_item_effects: empty line_items\n");
        ++m_error_count;
        return effects;
    }
    StringManager item_name_m;
    item_name_m.load_cstr_content(line_items.get_string_content(0));
    trim_ws_idx(item_name_m, 0);
    cstr item_name = item_name_m.get_string_content(0);
    if (item_name[0] == '\0') {
        printf("ERROR: DataParserBase parse_item_effects: empty item name\n");
        ++m_error_count;
        return effects;
    }

    cstr effect_line = 0;
    for (u32 i = 0; i < s_effect_definition_items->get_string_count(); ++i) {
        StringManager parts;
        get_line_items(s_effect_definition_items->get_string_content(i), parts);
        if (streq_trimmed(parts.get_string_content(0), item_name)) {
            effect_line = s_effect_definition_items->get_string_content(i);
            break;
        }
    }
    if (effect_line == 0) {
        printf("ERROR: DataParserBase parse_item_effects: no effect row for item '%s'\n", item_name);
        ++m_error_count;
        return effects;
    }
    if (s_item_effect_handler == 0) {
        printf("ERROR: DataParserBase parse_item_effects: call DataParserBase::set_item_effect_handler first\n");
        ++m_error_count;
        return effects;
    }
    s_item_effect_handler->reset_error_count();
    effects = s_item_effect_handler->parse_effects_line(effect_line);
    m_error_count += s_item_effect_handler->get_error_count();
    return effects;
}

ItemEffectsStruct DataParserBase::parse_item_effects_optional (const StringManager& line_items, u16 start_idx) const {
    (void)start_idx;
    ItemEffectsStruct effects = {};
    if (s_effect_definition_items == 0 || s_effect_definition_items->get_string_count() == 0) {
        return effects;
    }
    if (line_items.get_string_count() == 0) {
        printf("ERROR: DataParserBase parse_item_effects_optional: empty line_items\n");
        ++m_error_count;
        return effects;
    }
    StringManager item_name_m;
    item_name_m.load_cstr_content(line_items.get_string_content(0));
    trim_ws_idx(item_name_m, 0);
    cstr item_name = item_name_m.get_string_content(0);
    if (item_name[0] == '\0') {
        printf("ERROR: DataParserBase parse_item_effects_optional: empty item name\n");
        ++m_error_count;
        return effects;
    }

    cstr effect_line = 0;
    for (u32 i = 0; i < s_effect_definition_items->get_string_count(); ++i) {
        StringManager parts;
        get_line_items(s_effect_definition_items->get_string_content(i), parts);
        if (streq_trimmed(parts.get_string_content(0), item_name)) {
            effect_line = s_effect_definition_items->get_string_content(i);
            break;
        }
    }
    if (effect_line == 0) {
        return effects;
    }
    if (s_item_effect_handler == 0) {
        printf("ERROR: DataParserBase parse_item_effects_optional: call DataParserBase::set_item_effect_handler first\n");
        ++m_error_count;
        return effects;
    }
    s_item_effect_handler->reset_error_count();
    effects = s_item_effect_handler->parse_effects_line(effect_line);
    m_error_count += s_item_effect_handler->get_error_count();
    return effects;
}

ItemReqsStruct DataParserBase::parse_item_reqs (const StringManager& line_items, u16 start_idx) const {
    ItemReqsStruct reqs = {};
    for (u32 i = 0; i < MAX_PREREQ_COUNT; ++i) {
        reqs.indices[i] = U16_KEY_NULL;
    }
    u16 write_idx = 0;
    for (u16 i = start_idx; i < line_items.get_string_count(); ++i) {
        cstr raw_tok = line_items.get_string_content(i);
        if (is_empty_trimmed(raw_tok)) {
            continue;
        }
        if (write_idx >= MAX_PREREQ_COUNT) {
            printf("ERROR: DataParserBase too many (%u) ItemReqsStruct items\n", MAX_PREREQ_COUNT);
            ++m_error_count;
            return reqs;
        }
        StringManager tok_full;
        tok_full.load_cstr_content(raw_tok);
        trim_ws_idx(tok_full, 0);
        cstr token_raw = tok_full.get_string_content(0);

        StringManager tok;
        tok.load_cstr_content(token_raw);
        tok.split_string_by_char(0, '(');
        if (tok.get_string_count() < 2u) {
            printf("ERROR: DataParserBase found invalid req token '%s'\n", token_raw);
            ++m_error_count;
            continue;
        }
        trim_ws_idx(tok, 0);
        cstr type_name = tok.get_string_content(0);

        StringManager inside;
        inside.load_cstr_content(tok.get_string_content(1));
        inside.split_string_by_char(0, ')');
        inside.cull_empty_strings();
        if (inside.get_string_count() == 0 || inside.get_string_content(0)[0] == '\0') {
            printf("ERROR: DataParserBase found empty req name in token '%s'\n", token_raw);
            ++m_error_count;
            continue;
        }
        trim_ws_idx(inside, 0);

        StringManager args;
        args.load_cstr_content(inside.get_string_content(0));
        args.split_string_by_char(0, ',');
        trim_ws_all(args);
        args.cull_empty_strings();
        if (args.get_string_count() == 0 || args.get_string_content(0)[0] == '\0') {
            printf("ERROR: DataParserBase found empty req name in token '%s'\n", token_raw);
            ++m_error_count;
            continue;
        }
        cstr req_name = args.get_string_content(0);
        if (args.get_string_count() >= 2) {
            reqs.added_args[write_idx] = (u8)std::strtoul(args.get_string_content(1), 0, 10);
        }
        if (std::strcmp(type_name, "resource") == 0) {
            reqs.types[write_idx] = ITEM_REQ_TYPE_RESOURCE;
            reqs.indices[write_idx] = m_name_to_idx_cbs.resource_name_to_idx(req_name);
        } else if (std::strcmp(type_name, "flag") == 0) {
            reqs.types[write_idx] = ITEM_REQ_TYPE_FLAG;
            reqs.indices[write_idx] = m_name_to_idx_cbs.city_flag_name_to_idx(req_name);
        } else if (std::strcmp(type_name, "civ") == 0) {
            reqs.types[write_idx] = ITEM_REQ_TYPE_CIV;
            reqs.indices[write_idx] = m_name_to_idx_cbs.civ_name_to_idx(req_name);
        } else if (std::strcmp(type_name, "building") == 0) {
            reqs.types[write_idx] = ITEM_REQ_TYPE_BUILDING;
            reqs.indices[write_idx] = m_name_to_idx_cbs.building_name_to_idx(req_name);
        } else if (std::strcmp(type_name, "tech") == 0) {
            reqs.types[write_idx] = ITEM_REQ_TYPE_TECH;
            reqs.indices[write_idx] = m_name_to_idx_cbs.tech_name_to_idx(req_name);
        } else {
            printf("ERROR: DataParserBase unknown req type '%s' in token '%s'\n", type_name, token_raw);
            ++m_error_count;
            continue;
        }
        ++write_idx;
    }
    return reqs;
}

CivTraitStruct DataParserBase::parse_civ_traits (const StringManager& line_items, u16 start_idx) const {
    CivTraitStruct traits = {};
    for (u32 i = 0; i < MAX_CIV_TRAIT_COUNT; ++i) {
        traits.indices[i] = U16_KEY_NULL;
    }
    for (u16 i = 0; i + start_idx < line_items.get_string_count() && i < MAX_CIV_TRAIT_COUNT; ++i) {
        traits.indices[i] = m_name_to_idx_cbs.civ_trait_name_to_idx(line_items.get_string_content(i + start_idx));
    }
    return traits;
}

bool DataParserBase::parse_res_placement (const StringManager& line_items, ResPlacement& plc) const {
    plc = {};
    if (line_items.get_string_count() < 3) {
        printf("ERROR: DataParserBase parse_res_placement: expected name, res_wt, and quad(s)\n");
        ++m_error_count;
        return false;
    }
    StringManager res_wt;
    res_wt.load_cstr_content(line_items.get_string_content(1));
    trim_ws_idx(res_wt, 0);
    if (!plc_u16_tok(res_wt.get_string_content(0), &plc.m_res_wt)) {
        printf("ERROR: DataParserBase parse_res_placement: invalid res_wt '%s'\n", line_items.get_string_content(1));
        ++m_error_count;
        return false;
    }
    for (u16 qi = 2; qi < line_items.get_string_count() && plc.m_quad_n < RES_IO_QUAD_MAX; ++qi) {
        if (!plc_quad_tok(line_items.get_string_content(qi), &plc.m_quads[plc.m_quad_n])) {
            printf("ERROR: DataParserBase parse_res_placement: invalid quad '%s'\n", line_items.get_string_content(qi));
            ++m_error_count;
            return false;
        }
        ++plc.m_quad_n;
    }
    if (plc.m_quad_n == 0) {
        printf("ERROR: DataParserBase parse_res_placement: no quads parsed\n");
        ++m_error_count;
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

