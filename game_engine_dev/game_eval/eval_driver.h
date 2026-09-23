//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EVAL_DRIVER_H
#define EVAL_DRIVER_H

#include "eval_bin.h"
#include "eval_log.h"
#include "eval_need.h"
#include "eval_paths.h"

//================================================================================================================================
//=> - EvalDriver -
//================================================================================================================================
//
//  Abstract eval driver. Derived classes must pass an EvalNeed into this base ctor.
//  go() loads paths, checks log/save needs (full unpack for save(turn); verify-only for save_seq()), then run().
//
//================================================================================================================================

class EvalDriver {
public:
    virtual ~EvalDriver ();
    int go (cstr paths_file);

protected:
    explicit EvalDriver (const EvalNeed& need);
    virtual int run () = 0;

    const EvalNeed& need () const;
    EvalPaths& paths ();
    EvalLog& log ();
    EvalBin& bin ();

private:
    bool chk ();

    EvalNeed m_need;
    EvalPaths m_paths;
    EvalLog m_log;
    EvalBin m_bin;
};

#endif // EVAL_DRIVER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
