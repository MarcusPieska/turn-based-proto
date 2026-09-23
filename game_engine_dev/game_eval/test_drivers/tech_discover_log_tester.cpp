//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "eval_driver.h"
#include "eval_need.h"

//================================================================================================================================
//=> - TechDiscoverLogTester -
//================================================================================================================================

class TechDiscoverLogTester : public EvalDriver {
public:
    TechDiscoverLogTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.trace();
    n.logs().m_player_tech_discover = 1;
    return n;
}

TechDiscoverLogTester::TechDiscoverLogTester ()
    : EvalDriver(make_need()) {
}

int TechDiscoverLogTester::run () {
    u32 shown = 0;
    u32 hits = 0;
    for (u32 i = 0; i < log().line_n(); ++i) {
        u16 player = 0;
        u16 tech = 0;
        if (!log().parse_tech_discover(i, &player, &tech)) {
            continue;
        }
        hits = hits + 1u;
        if (shown < 12u) {
            std::printf("tech_discover player=%u tech=%u\n", static_cast<u32>(player), static_cast<u32>(tech));
            shown = shown + 1u;
        }
    }
    std::printf("tech_discover total=%u\n", hits);
    return hits > 0u ? 0 : 1;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    TechDiscoverLogTester t;
    return t.go("/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt");
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
