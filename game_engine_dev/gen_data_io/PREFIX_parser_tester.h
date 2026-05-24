//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef [MACRO_TAG]_PARSER_TESTER_H
#define [MACRO_TAG]_PARSER_TESTER_H

#include "[FILE_TAG]_parser.h"
#include "[FILE_TAG]_static_data.h"
#include "path_mng.h"
#include "name_to_idx_callbacks.h"
#include "item_effects.h"

[DEP_SD_INCLUDES_TAG]

//================================================================================================================================
//=> - [CLASS_TAG]ParserTester class -
//================================================================================================================================

class [CLASS_TAG]ParserTester {
public:
    [CLASS_TAG]ParserTester ();
    void set_plvl (int lvl);
    int run ();
    void open_writer ();
    void close_writer ();
    void pr_item (const [STRUCT_TAG]& item);
    
    [DEP_SD_SETTERS_DECL_TAG]

private:
    typedef const char* cstr;
    int m_plvl;
    FILE* m_out;
    FILE* out () const;
    
    [DEP_SD_MEMBERS_TAG]

    [DEP_PSR_MEMBERS_TAG]

    static [CLASS_TAG]ParserTester* s_inst;

    [DEP_N2I_DECL_TAG]

    bool ld_sm (StringManager& sm, cstr path);
    void pr_u16 (cstr label, u16 value);
    void pr_u32 (cstr label, u32 value);
    void pr_reqs (cstr label, const ItemReqsStruct& reqs);
    void pr_fx (cstr label, const ItemEffectsStruct& e);
    void pr_traits (cstr label, const CivTraitStruct& traits);
};

#endif // [MACRO_TAG]_PARSER_TESTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
