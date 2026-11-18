#include "dct-prim.h"
#include "dct16-sme2-tables.h"
#include "dct32-sme2-tables.h"
#include <arm_sme.h>

#define X265_PRAGMA(text)       _Pragma(#text)
#if defined(__clang__)
#define X265_PRAGMA_UNROLL(n)   X265_PRAGMA(unroll(n))
#elif defined(__GNUC__)
#define X265_PRAGMA_UNROLL(n)   X265_PRAGMA(GCC unroll (n))
#else
#define X265_PRAGMA_UNROLL(n)
#endif

namespace
{

// Transpose eight rows at a time, keeping adjacent samples paired for SMOPA.
template<int size, int unrollFactor>
__attribute__((always_inline)) static inline
void packDctRows(const int16_t *src, intptr_t stride, int16_t *packed) __arm_streaming __arm_inout("za")
{
    const svbool_t all16 = svptrue_b16();
    const svbool_t all32 = svptrue_b32();
    X265_PRAGMA_UNROLL(unrollFactor)
    for (int rowGroup = 0; rowGroup < size / 8; ++rowGroup)
    {
        const int16_t *rows = src + 8 * rowGroup * stride;
        int16_t *group = packed + 8 * rowGroup * size;
        X265_PRAGMA_UNROLL(unrollFactor)
        for (int r = 0; r < 8; ++r)
        {
            const int16_t *row = rows + r * stride;
            svwrite_hor_za32_s32_m(0, r, all32, svreinterpret_s32_s16(svld1_s16(all16, row)));
            if (size == 32)
                svwrite_hor_za32_s32_m(1, r, all32, svreinterpret_s32_s16(svld1_s16(all16, row + 16)));
        }
        X265_PRAGMA_UNROLL(unrollFactor)
        for (int c = 0; c < 8; c += 2)
        {
            svint32x2_t v = svread_ver_za32_s32_vg2(0, c);
            svst1_s16_x2(svptrue_c16(), group + c * 16,
                svcreate2_s16(svreinterpret_s16_s32(svget2_s32(v, 0)), svreinterpret_s16_s32(svget2_s32(v, 1))));
            if (size == 32)
            {
                v = svread_ver_za32_s32_vg2(1, c);
                svst1_s16_x2(svptrue_c16(), group + (c + 8) * 16,
                    svcreate2_s16(svreinterpret_s16_s32(svget2_s32(v, 0)), svreinterpret_s16_s32(svget2_s32(v, 1))));
            }
        }
    }
}

template<int tile, int shift>
__attribute__((always_inline)) static inline
svint16_t readDctPair(uint32_t slice) __arm_streaming __arm_in("za")
{
    return svqrshrn_n_s16_s32_x2(svread_ver_za32_s32_vg2(tile, slice), shift);
}

template<int size, int shift, bool packedOutput, int unrollFactor>
__attribute__((always_inline)) static inline
void transformDctPass(const int16_t *packed, int16_t *dst,
                     const int16_t coeff[size / 8][size / 2][16]) __arm_streaming __arm_inout("za")
{
    const svbool_t all16 = svptrue_b16();
    const svbool_t all32 = svptrue_b32();
    X265_PRAGMA_UNROLL(unrollFactor)
    for (int coeffGroup = 0; coeffGroup < size / 8; coeffGroup += 2)
    {
        X265_PRAGMA_UNROLL(unrollFactor)
        for (int inputGroup = 0; inputGroup < size / 8; inputGroup += 2)
        {
            const int16_t *input0 = packed + inputGroup * 8 * size;
            const int16_t *input1 = input0 + 8 * size;
            svzero_za();
            X265_PRAGMA_UNROLL(unrollFactor)
            for (int pair = 0; pair < size / 2; ++pair)
            {
                svint16_t coeff0 = svld1_s16(all16, coeff[coeffGroup][pair]);
                svint16_t coeff1 = svld1_s16(all16, coeff[coeffGroup + 1][pair]);
                svint16_t samples0 = svld1_s16(all16, input0 + 16 * pair);
                svint16_t samples1 = svld1_s16(all16, input1 + 16 * pair);
                svmopa_za32_s16_m(0, all16, all16, coeff0, samples0);
                svmopa_za32_s16_m(1, all16, all16, coeff0, samples1);
                svmopa_za32_s16_m(2, all16, all16, coeff1, samples0);
                svmopa_za32_s16_m(3, all16, all16, coeff1, samples1);
            }
            if (packedOutput)
            {
                // Transpose and pack the first pass for the second SMOPA pass.
                int16_t *out0 = dst + (coeffGroup * (size / 2) + 4 * inputGroup) * 16;
                int16_t *out1 = out0 + 8 * size;
                X265_PRAGMA_UNROLL(unrollFactor)
                for (int c = 0; c < 4; ++c)
                {
                    svst1_s16(all16, out0 + c * 16, readDctPair<0, shift>(2 * c));
                    svst1_s16(all16, out0 + (4 + c) * 16, readDctPair<1, shift>(2 * c));
                    svst1_s16(all16, out1 + c * 16, readDctPair<2, shift>(2 * c));
                    svst1_s16(all16, out1 + (4 + c) * 16, readDctPair<3, shift>(2 * c));
                }
            }
            else
            {
                X265_PRAGMA_UNROLL(unrollFactor)
                for (int r = 0; r < 8; ++r)
                {
                    svint32_t v0 = svread_hor_za32_s32_m(svdup_s32(0), all32, 0, r);
                    svint32_t v1 = svread_hor_za32_s32_m(svdup_s32(0), all32, 1, r);
                    svint32_t v2 = svread_hor_za32_s32_m(svdup_s32(0), all32, 2, r);
                    svint32_t v3 = svread_hor_za32_s32_m(svdup_s32(0), all32, 3, r);
                    svst1_s16(all16, dst + (8 * coeffGroup + r) * size + 8 * inputGroup,
                        svqrshr_n_s16_s32_x2(svcreate2_s32(v0, v1), shift));
                    svst1_s16(all16, dst + (8 * (coeffGroup + 1) + r) * size + 8 * inputGroup,
                        svqrshr_n_s16_s32_x2(svcreate2_s32(v2, v3), shift));
                }
            }
        }
    }
}

__arm_new("za") __arm_locally_streaming
void dct16_sme2(const int16_t *src, int16_t *dst, intptr_t stride)
{
    int16_t packed[16 * 16];
    int16_t intermediate[16 * 16];
    packDctRows<16, 8>(src, stride, packed);
    transformDctPass<16, 3, true, 8>(packed, intermediate, packedDct16);
    transformDctPass<16, 10, false, 8>(intermediate, dst, packedDct16);
}

__arm_new("za") __arm_locally_streaming
void dct32_sme2(const int16_t *src, int16_t *dst, intptr_t stride)
{
    int16_t packed[32 * 32];
    int16_t intermediate[32 * 32];
    packDctRows<32, 2>(src, stride, packed);
    transformDctPass<32, 4, true, 2>(packed, intermediate, packedDct32);
    transformDctPass<32, 11, false, 2>(intermediate, dst, packedDct32);
}
}

namespace X265_NS
{
void setupDCTPrimitives_sme2(EncoderPrimitives &p)
{
    // The packing uses eight 32-bit lanes per ZA tile row.
    if (svcntsb() != 32)
        return;

    p.cu[BLOCK_16x16].dct = dct16_sme2;
    p.cu[BLOCK_32x32].dct = dct32_sme2;
}
}
