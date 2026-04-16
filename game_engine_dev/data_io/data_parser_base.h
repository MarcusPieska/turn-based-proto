//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DATA_PARSER_BASE_H
#define DATA_PARSER_BASE_H

#include <string>
#include <vector>

#include "data_reader.h"
#include "game_primitives.h"
#include "item_effects.h"
#include "item_reqs.h"
#include "name_to_idx_callbacks.h"

class ItemEffectHandler;

//================================================================================================================================
//=> - DataParserBase class -
//================================================================================================================================

class DataParserBase {
public:
    DataParserBase (const std::vector<RawItem>& raw_items, const NameToIdxCbs& name_to_idx_cbs);
    virtual ~DataParserBase () = default;

    static void set_item_effect_handler (const NameToIdxCbs* name_to_idx_cbs, const std::vector<RawItem>* effect_defs);
    static void clear_item_effect_handler ();

    u16 name_to_idx (const std::string& name) const;
    std::string idx_to_name (u16 idx) const;
    static void check_errors ();

protected:
    const std::vector<RawItem>& get_raw_items () const;
    const std::vector<std::string> get_line_items (const std::string& line) const;

    u16 parse_u16 (const std::vector<std::string>& line_items, u16 start_idx) const;
    u32 parse_u32 (const std::vector<std::string>& line_items, u16 start_idx) const;
    u16 parse_unit_type (const std::vector<std::string>& line_items, u16 start_idx) const;
    ItemReqsStruct parse_item_reqs (const std::vector<std::string>& line_items, u16 start_idx) const;
    ItemEffectsStruct parse_item_effects (const std::vector<std::string>& line_items, u16 start_idx) const;
    CivTraitStruct parse_civ_traits (const std::vector<std::string>& line_items, u16 start_idx) const;
    
    static u32 get_error_count_for_tests ();
    static void reset_error_count_for_tests ();

    const std::vector<RawItem>& m_raw_items;
    NameToIdxCbs m_name_to_idx_cbs;
    u16 m_item_count; 

private:
    DataParserBase () = delete;
    DataParserBase (const DataParserBase& other) = delete;
    DataParserBase (DataParserBase&& other) = delete;

    static u32 m_error_count;
    static ItemEffectHandler* s_item_effect_handler;
    static const std::vector<RawItem>* s_effect_definition_items;

};

#endif // DATA_PARSER_BASE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

