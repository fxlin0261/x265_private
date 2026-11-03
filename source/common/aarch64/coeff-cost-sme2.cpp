#include "common.h"
#include "primitives.h"
#include "contexts.h"
#include "coeff-group-mask-sme2.h"
#include <arm_sme.h>
#include <stdio.h>
#include <stdlib.h>

namespace X265_NS {

// hotspot2
__arm_locally_streaming
uint32_t costCoeffNxN_sme2_buffered(const uint16_t *scan, const coeff_t *coeff, intptr_t trSize,
                          uint16_t *absCoeff, const uint8_t *tabSigCtx, uint32_t scanFlagMask,
                          uint8_t *baseCtx, int offset, int scanPosSigOff, int subPosBase)
{
    uint16_t tmp[16], ordered[16], ctx[16];
    const svbool_t four = svptrue_pat_b16(SV_VL4);
    for (int y = 0; y < 4; ++y)
        svst1_u16(four, tmp + 4*y, svreinterpret_u16_s16(svabs_s16_x(four, svld1_s16(four, coeff + y*trSize))));
    const svbool_t all = svptrue_b16();
    svuint16_t indices = svld1_u16(all, scan);
    svst1_u16(all, ordered, svtbl_u16(svld1_u16(all, tmp), indices));
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

namespace {
costCoeffNxN_t originalCost = 0;
bool enabled = false;
costCoeffNxN_t candidateCost = costCoeffNxN_sme2_packed;
const char *profilePath = 0;
uint64_t histogram[4][16][2][2] = {};

void saveHistogram()
{
    FILE *f = fopen(profilePath, "w");
    if (!f) { perror("coefficient-cost profile"); return; }
    fprintf(f, "trSize,scanPosSigOff,subPosZero,priorNonzero,calls\n");
    for (int t = 0; t < 4; ++t)
        for (int p = 0; p < 16; ++p)
            for (int z = 0; z < 2; ++z)
                for (int n = 0; n < 2; ++n)
                    if (histogram[t][p][z][n])
                        fprintf(f, "%d,%d,%d,%d,%llu\n", 4<<t, p, z, n,
                                (unsigned long long)histogram[t][p][z][n]);
    fclose(f);
}

uint32_t dispatch(const uint16_t *scan, const coeff_t *coeff, intptr_t trSize,
                  uint16_t *absCoeff, const uint8_t *tabSigCtx, uint32_t scanFlagMask,
                  uint8_t *baseCtx, int offset, int scanPosSigOff, int subPosBase)
{
    if (profilePath)
    {
        int t = trSize == 4 ? 0 : trSize == 8 ? 1 : trSize == 16 ? 2 : 3;
        int prior = (scanFlagMask & ((1u << scanPosSigOff)-1)) != 0;
        __atomic_fetch_add(&histogram[t][scanPosSigOff][subPosBase == 0][prior], uint64_t(1), __ATOMIC_RELAXED);
    }
    return (enabled ? candidateCost : originalCost)
        (scan, coeff, trSize, absCoeff, tabSigCtx, scanFlagMask, baseCtx, offset, scanPosSigOff, subPosBase);
}
}

void setupCoeffCostPrimitives_sme2(EncoderPrimitives &p)
{
    const char *setting = getenv("X265_SME2_COEFF_COST");
    enabled = setting && setting[0] == '1' && setting[1] == '\0' && sme2Available();
    const char *variant = getenv("X265_SME2_COEFF_COST_VARIANT");
    candidateCost = variant && variant[0] == '1' ? costCoeffNxN_sme2_buffered : costCoeffNxN_sme2_packed;
    profilePath = getenv("X265_SME2_COEFF_COST_PROFILE");
    if (profilePath && !profilePath[0]) profilePath = 0;
    if (profilePath) atexit(saveHistogram);
    if ((enabled || profilePath) && p.costCoeffNxN != dispatch)
    {
        originalCost = p.costCoeffNxN;
        p.costCoeffNxN = dispatch;
    }
}
}
