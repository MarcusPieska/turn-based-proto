//================================================================================================================================
// Texture-border pixel rims between subtilles (base stamp + per-tip splat outlines).
//================================================================================================================================

#ifndef DIA_RIM_H
#define DIA_RIM_H

#include <string>
#include <vector>
#include <cstdint>

#include "dia_cfg.h"
#include "dia_map.h"
#include "dia_grid.h"
#include "dia_super.h"

//================================================================================================================================
//=> - Class: DiaRim -
//================================================================================================================================

class DiaRim {
public:
    DiaRim ();
    ~DiaRim ();

    bool ld (const std::string &bin_path);
    bool ld_splat (const std::string &root);
    void set_tips (uint8_t allow);
    void prep ();

    int sw () const;
    int sh () const;
    const uint8_t *stamp_rim () const;
    const uint8_t *splat_rim (uint8_t tip) const;

    void paint_ids (std::vector<int> &ids, int dw, int dh, const BlendMap &map, const SubGrid &sg,
                    const DiaGrid &grid) const;

    void mark (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg,
               const DiaGrid &grid) const;

private:
    static const int SP_N = 2;

    bool on (int sx, int sy) const;
    bool splat_on (uint8_t tip, int sx, int sy, int ix) const;
    uint8_t tip_hit (uint8_t corn, int sx, int sy, int ix) const;
    int sp_ix (int x, int y) const;
    bool ld_one_splat (const std::string &path, std::vector<uint8_t> &out) const;
    void rim_of (const std::vector<uint8_t> &src, std::vector<uint8_t> &out) const;

    int m_sw; // stamp width
    int m_sh; // stamp height
    int m_ox; // stamp x of sub_ctr
    int m_oy; // stamp y of sub_ctr
    std::vector<uint8_t> m_mask; // 1 = on
    std::vector<uint8_t> m_sn[SP_N]; // N tip splat variants
    std::vector<uint8_t> m_se[SP_N]; // E tip splat variants
    std::vector<uint8_t> m_ss[SP_N]; // S tip splat variants
    std::vector<uint8_t> m_sw_splat[SP_N]; // W tip splat variants
    std::vector<uint8_t> m_stamp_rim; // outer rim of base stamp
    std::vector<uint8_t> m_rim_n; // outer rim of N splat within stamp
    std::vector<uint8_t> m_rim_e; // outer rim of E splat
    std::vector<uint8_t> m_rim_s; // outer rim of S splat
    std::vector<uint8_t> m_rim_w; // outer rim of W splat
    uint8_t m_tips; // allowed CORN_* bits
};

#endif // DIA_RIM_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
