//================================================================================================================================
// Soften texture borders: rim thick N, own weight 100-(N-d+1)*EDGE_WT_DELTA at dist d (outermost strongest).
//================================================================================================================================

#ifndef DIA_EDGE_H
#define DIA_EDGE_H

#include <string>
#include <cstdint>

#include "dia_map.h"
#include "dia_grid.h"
#include "dia_rim.h"

//================================================================================================================================
//=> - Class: DiaEdge -
//================================================================================================================================

class DiaEdge {
public:
    DiaEdge ();
    ~DiaEdge ();

    bool ld (const std::string &bin_path);
    bool ld_splat (const std::string &root);
    void set_tips (uint8_t allow);
    void prep ();

    void apply (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg,
                const DiaGrid &grid) const;

private:
    DiaRim m_rim; // ownership + rim source
};

#endif // DIA_EDGE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
