#include "common.h"
#include "primitives.h"
#include "sme2.h"
#include <arm_sme.h>
#include <stdlib.h>
#include "contexts.h"

namespace X265_NS {

// hotspot2
__arm_locally_streaming
uint32_t costCoeffNxN_sme2_packed(const uint16_t *scan, const coeff_t *coeff, intptr_t trSize,
                          uint16_t *absCoeff, const uint8_t *tabSigCtx, uint32_t scanFlagMask,
                          uint8_t *baseCtx, int offset, int scanPosSigOff, int subPosBase)
{
    uint16_t ordered[16], ctx[16];
    const svbool_t four = svptrue_pat_b16(SV_VL4);
    const svbool_t eight = svptrue_pat_b16(SV_VL8);
    const svbool_t all = svptrue_b16();
    svint16_t rows01 = svsplice_s16(four, svld1_s16(four, coeff), svld1_s16(four, coeff+trSize));
    svint16_t rows23 = svsplice_s16(four, svld1_s16(four, coeff+2*trSize), svld1_s16(four, coeff+3*trSize));
    svuint16_t packed = svreinterpret_u16_s16(svabs_s16_x(all, svsplice_s16(eight, rows01, rows23)));
    svuint16_t indices = svld1_u16(all, scan);
    svst1_u16(all, ordered, svtbl_u16(packed, indices));
    svuint16_t context = svtbl_u16(svld1ub_u16(all, tabSigCtx), indices);
    svst1_u16(all, ctx, svadd_n_u16_x(all, context, (uint16_t)offset));

    uint32_t nonzero = scanPosSigOff < 15 ? 1 : 0;
    absCoeff -= nonzero;
    uint32_t sum = 0;
    do
    {
        uint32_t sig = scanFlagMask & 1;
        scanFlagMask >>= 1;
        if (scanPosSigOff != 0 || subPosBase == 0 || nonzero)
        {
            uint32_t ctxSig = subPosBase + scanPosSigOff ? ctx[scanPosSigOff] : 0;
            uint32_t state = baseCtx[ctxSig];
            uint32_t bits = PFX(entropyStateBits)[state ^ sig];
            uint32_t next = (bits >> 24) + (state & 1);
            if ((state ^ sig) == 1)
                next = sig;
            baseCtx[ctxSig] = (uint8_t)next;
            sum += bits;
        }
        absCoeff[nonzero] = ordered[scanPosSigOff];
        nonzero += sig;
    }
    while (--scanPosSigOff >= 0);
    return sum & 0xffffff;
}

void setupCoeffCostPrimitives_sme2(EncoderPrimitives &p)
{
    const char *s = getenv("X265_SME2_COEFF_COST");
    if (s && s[0] == '1' && !s[1] && sme2Available())
        p.costCoeffNxN = costCoeffNxN_sme2_packed;
}
}
