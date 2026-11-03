#ifndef X265_COEFF_GROUP_MASK_SME2_H
#define X265_COEFF_GROUP_MASK_SME2_H

#include <stdint.h>

namespace X265_NS {

// hotspot1
inline uint64_t coeffGroupMask_c(const uint16_t *scanCG,
                                     const uint8_t *coeffNum, int lastScanSet)
{
    uint64_t mask = 0;
    for (int idx = 0; idx < lastScanSet; idx++)
    {
        const uint8_t subSet = (uint8_t)scanCG[idx];
        const uint8_t nonZero = (coeffNum[idx] != 0);
        mask |= ((nonZero ? (uint64_t)1 : 0) << subSet);
    }
    return mask;
}

bool coeffGroupMaskSme2Enabled();

bool useCoeffGroupMaskU64();
uint64_t coeffGroupMask_sme2_u64(const uint16_t *, const uint8_t *, int);

bool sme2Available();
void recordCoeffGroupMaskCalls(int lastScanSet);
uint64_t coeffGroupMask_sme2_u32(const uint16_t *scanCG,
                            const uint8_t *coeffNum, int lastScanSet);
}
#endif
