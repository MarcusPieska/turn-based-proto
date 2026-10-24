//================================================================================================================================
// Probe-path blit: (si,sj) subtile grid, dia_corn + tip remap, stamp/splat — no li/lj hop.
//================================================================================================================================

#ifndef DIA_SISJ_H
#define DIA_SISJ_H

#include <string>
#include <vector>
#include <cstdint>

#include "dia_cfg.h"
#include "dia_map.h"
#include "dia_grid.h"
#include "dia_dat15.h"
#include "dia_super.h"

//================================================================================================================================
//=> - Class: DiaSiSj -
//================================================================================================================================

class DiaSiSj {
public:
    DiaSiSj ();
    ~DiaSiSj ();

    bool ld (const std::string &bin_path);
    bool ld_png (const std::string &png_path);
    bool ld_splat (const std::string &root);
    void set_splat (SplatMode mode);
    void set_tips (uint8_t allow);
    int sw () const;
    int sh () const;
    int ox () const;
    int oy () const;

    void fill_corn (SubGrid &sg) const;

    void flat (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg, const DiaGrid &grid) const;
    void splat_flat (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg,
                     const DiaGrid &grid) const;

    void tex (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg, const DiaGrid &grid,
              const DiaDat15 *const *banks, int n_banks) const;
    void splat_tex (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg, const DiaGrid &grid,
                    const DiaDat15 *const *banks, int n_banks) const;

private:
    static const int SP_N = 2;

    bool on (int sx, int sy) const;
    bool splat_on (uint8_t tip, int sx, int sy, int ix) const;
    uint8_t tip_hit (uint8_t corn, int sx, int sy, int ix) const;
    int sp_ix (int x, int y) const;
    bool ld_one_splat (const std::string &path, std::vector<uint8_t> &out) const;
    Rgb stamp_col (int sx, int sy, int tid, bool splat) const;
    Rgb darken (Rgb c, float t) const;

    int m_sw; // stamp width
    int m_sh; // stamp height
    int m_ox; // stamp x of sub_ctr
    int m_oy; // stamp y of sub_ctr
    std::vector<uint8_t> m_mask; // 1 = on
    std::vector<uint8_t> m_rgb; // optional stamp RGB (3 * sw * sh)
    std::vector<uint8_t> m_sn[SP_N]; // N tip splat variants
    std::vector<uint8_t> m_se[SP_N]; // E tip splat variants
    std::vector<uint8_t> m_ss[SP_N]; // S tip splat variants
    std::vector<uint8_t> m_sw_splat[SP_N]; // W tip splat variants
    SplatMode m_splat; // viz mode
    uint8_t m_tips; // allowed CORN_* bits
};

#endif // DIA_SISJ_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
