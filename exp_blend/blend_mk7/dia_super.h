//================================================================================================================================
// Super-subtile stamp: load center-anchored mask and blit onto a diamond grid.
//================================================================================================================================

#ifndef DIA_SUPER_H
#define DIA_SUPER_H

#include <string>
#include <vector>
#include <cstdint>

#include "dia_cfg.h"
#include "dia_map.h"
#include "dia_grid.h"
#include "dia_dat15.h"

//================================================================================================================================
//=> - Splat viz mode -
//================================================================================================================================

enum SplatMode {
    SPLAT_RED = 0, // swapped stamp pixels = solid red (default)
    SPLAT_OTHER = 1, // swapped stamp pixels sample the other tip terrain
    SPLAT_NONE = 2 // no corner splat
};

//================================================================================================================================
//=> - Class: DiaSuper -
//================================================================================================================================

class DiaSuper {
public:
    DiaSuper ();
    ~DiaSuper ();

    bool ld (const std::string &bin_path);
    bool ld_splat (const std::string &root);
    void set_splat (SplatMode mode);
    void set_tips (uint8_t allow);
    int sw () const;
    int sh () const;
    int ox () const;
    int oy () const;

    void flat (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg, const DiaGrid &grid) const;

    void tex (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg, const DiaGrid &grid,
              const DiaDat15 *const *banks, int n_banks) const;

private:
    bool on (int sx, int sy) const;
    bool splat_on (uint8_t tip, int sx, int sy) const;
    uint8_t tip_hit (uint8_t corn, int sx, int sy) const;
    bool ld_one_splat (const std::string &path, std::vector<uint8_t> &out) const;

    int m_sw; // stamp width
    int m_sh; // stamp height
    int m_ox; // stamp x of sub_ctr
    int m_oy; // stamp y of sub_ctr
    std::vector<uint8_t> m_mask; // 1 = on
    std::vector<uint8_t> m_sn; // N tip splat
    std::vector<uint8_t> m_se; // E tip splat
    std::vector<uint8_t> m_ss; // S tip splat
    std::vector<uint8_t> m_sw_splat; // W tip splat
    SplatMode m_splat; // viz mode
    uint8_t m_tips; // allowed CORN_* bits (default all)
};

#endif // DIA_SUPER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
