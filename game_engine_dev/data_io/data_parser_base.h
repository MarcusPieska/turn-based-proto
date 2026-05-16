//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DATA_PARSER_BASE_H
#define DATA_PARSER_BASE_H

#include <string>

#include "game_primitives.h"
#include "item_effects.h"
#include "item_reqs.h"
#include "name_to_idx_callbacks.h"
#include "opt_str_mng.h"

class ItemEffectHandler;

//================================================================================================================================
//=> - DataParserBase class -
//================================================================================================================================

class DataParserBase {
public:
    DataParserBase (const StringManager& raw_lines, const NameToIdxCbs& name_to_idx_cbs);
    virtual ~DataParserBase () = default;

    static void set_item_effect_handler (const NameToIdxCbs* name_to_idx_cbs, const StringManager* effect_defs);
    static void clear_item_effect_handler ();

    u16 name_to_idx (cstr name) const;
    std::string idx_to_name (u16 idx) const;
    static void check_errors ();

protected:
    const StringManager& get_raw_lines () const;
    const StringManager& get_names () const;
    void get_line_items (cstr line, StringManager& out_items) const;
    void derive_names_from_raw_lines (const StringManager& raw_lines, StringManager& out_names) const;

    u16 parse_u16 (const StringManager& line_items, u16 start_idx) const;
    u32 parse_u32 (const StringManager& line_items, u16 start_idx) const;
    u16 parse_unit_type (const StringManager& line_items, u16 start_idx) const;
    ItemReqsStruct parse_item_reqs (const StringManager& line_items, u16 start_idx) const;
    ItemEffectsStruct parse_item_effects (const StringManager& line_items, u16 start_idx) const;
    CivTraitStruct parse_civ_traits (const StringManager& line_items, u16 start_idx) const;
    
    static u32 get_error_count_for_tests ();
    static void reset_error_count_for_tests ();

    StringManager m_raw_lines;
    StringManager m_names;
    NameToIdxCbs m_name_to_idx_cbs;
    u16 m_item_count; 

private:
    DataParserBase () = delete;
    DataParserBase (const DataParserBase& other) = delete;
    DataParserBase (DataParserBase&& other) = delete;

    static u32 m_error_count;
    static ItemEffectHandler* s_item_effect_handler;
    static const StringManager* s_effect_definition_items;

};

#endif // DATA_PARSER_BASE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

