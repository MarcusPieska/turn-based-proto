#include "unit_action_static_data.h"
#include "data_parser_base.h"

bool UnitActionStaticData::load_names_from (const DataParserBase& psr, u16 n) {
    if (n == 0) {
        return false;
    }
    cstr* names = new cstr[n];
    for (u16 i = 0; i < n; ++i) {
        names[i] = psr.idx_to_name(i);
    }
    bool ok = load_names(names, n);
    delete[] names;
    return ok;
}
