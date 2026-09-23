//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EVAL_PATHS_H
#define EVAL_PATHS_H

#include "game_primitives.h"

//================================================================================================================================
//=> - EvalPaths -
//================================================================================================================================
//
//  Loads game_eval/eval_paths.txt (OUT_ROOT, SAVES_ROOT, TRACE_PATH, SEED, PLAYERS).
//  Output layout: OUT_ROOT/game-eval/ (images), OUT_ROOT/game-eval/data/<name>/ (data).
//
//================================================================================================================================

class EvalPaths {
public:
    EvalPaths ();

    bool load (cstr path);
    bool ok () const;

    cstr out_root () const;
    cstr saves_root () const;
    cstr trace_path () const;
    u32 seed () const;
    u16 players () const;

    bool eval_dir (char* buf, u32 cap) const;
    bool data_dir (cstr name, char* buf, u32 cap) const;

    bool map_path (u32 turn, char* buf, u32 cap) const;
    bool units_path (u32 turn, char* buf, u32 cap) const;
    bool cities_path (u32 turn, char* buf, u32 cap) const;
    bool players_path (u32 turn, char* buf, u32 cap) const;

    bool scan_saves ();
    u16 save_turn_n () const;
    u32 save_turn_at (u16 i) const;

private:
    bool fill (char* buf, u32 cap, cstr suffix, u32 turn) const;
    bool quartet_ok (u32 turn) const;

    static const u16 SAVE_TURN_MAX = 512;

    char m_out[512];
    char m_saves[512];
    char m_trace[512];
    u32 m_seed;
    u16 m_players;
    bool m_ok;
    u32 m_save_turns[SAVE_TURN_MAX];
    u16 m_save_turn_n;
};

#endif // EVAL_PATHS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
