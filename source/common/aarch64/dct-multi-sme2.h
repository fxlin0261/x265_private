#ifndef X265_DCT_MULTI_SME2_H
#define X265_DCT_MULTI_SME2_H

template<int size, int unroll>
__attribute__((always_inline)) static inline
void packDctRowsMulti(const int16_t *src, intptr_t stride, int16_t packed[size / 8][size / 2][16]) __arm_streaming __arm_inout("za")
{
    svbool_t all16 = svptrue_b16(), all32 = svptrue_b32();
    #pragma clang loop unroll_count(unroll)
    for (int g = 0; g < size / 8; ++g)
    {
        #pragma clang loop unroll_count(unroll)
        for (int r = 0; r < 8; ++r)
        {
            svwrite_hor_za32_s32_m(0, r, all32, svreinterpret_s32_s16(svld1_s16(all16, src + (8 * g + r) * stride)));
            if (size == 32)
                svwrite_hor_za32_s32_m(1, r, all32, svreinterpret_s32_s16(svld1_s16(all16, src + (8 * g + r) * stride + 16)));
        }
        #pragma clang loop unroll_count(unroll)
        for (int c = 0; c < 8; c += 2)
        {
            svint32x2_t v = svread_ver_za32_s32_vg2(0, c);
            svst1_s16_x2(svptrue_c16(), packed[g][c],
                svcreate2_s16(svreinterpret_s16_s32(svget2_s32(v, 0)), svreinterpret_s16_s32(svget2_s32(v, 1))));
            if (size == 32)
            {
                v = svread_ver_za32_s32_vg2(1, c);
                svst1_s16_x2(svptrue_c16(), packed[g][c + 8],
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

template<int size, int shift, bool packedOutput, int unroll>
__attribute__((always_inline)) static inline
void transformDctMulti(const int16_t packed[size / 8][size / 2][16], int16_t *dst,
                       const int16_t coeff[size / 8][size / 2][16]) __arm_streaming __arm_inout("za")
{
    svbool_t all16 = svptrue_b16(), all32 = svptrue_b32();
    #pragma clang loop unroll_count(unroll)
    for (int kg = 0; kg < size / 8; kg += 2)
        #pragma clang loop unroll_count(unroll)
        for (int jg = 0; jg < size / 8; jg += 2)
        {
            svzero_za();
            #pragma clang loop unroll_count(unroll)
            for (int pair = 0; pair < size / 2; ++pair)
            {
                svint16_t k0 = svld1_s16(all16, coeff[kg][pair]);
                svint16_t k1 = svld1_s16(all16, coeff[kg + 1][pair]);
                svint16_t j0 = svld1_s16(all16, packed[jg][pair]);
                svint16_t j1 = svld1_s16(all16, packed[jg + 1][pair]);
                svmopa_za32_s16_m(0, all16, all16, k0, j0);
                svmopa_za32_s16_m(1, all16, all16, k0, j1);
                svmopa_za32_s16_m(2, all16, all16, k1, j0);
                svmopa_za32_s16_m(3, all16, all16, k1, j1);
            }
            if (packedOutput)
            {
                #pragma clang loop unroll_count(unroll)
                for (int c = 0; c < 4; ++c)
                {
                    svst1_s16(all16, dst + (kg * (size / 2) + 4 * jg + c) * 16, readDctPair<0, shift>(2 * c));
                    svst1_s16(all16, dst + (kg * (size / 2) + 4 * jg + 4 + c) * 16, readDctPair<1, shift>(2 * c));
                    svst1_s16(all16, dst + ((kg + 1) * (size / 2) + 4 * jg + c) * 16, readDctPair<2, shift>(2 * c));
                    svst1_s16(all16, dst + ((kg + 1) * (size / 2) + 4 * jg + 4 + c) * 16, readDctPair<3, shift>(2 * c));
                }
            }
            else
            {
                #pragma clang loop unroll_count(unroll)
                for (int r = 0; r < 8; ++r)
                {
                    svint32_t v0 = svread_hor_za32_s32_m(svdup_s32(0), all32, 0, r);
                    svint32_t v1 = svread_hor_za32_s32_m(svdup_s32(0), all32, 1, r);
                    svint32_t v2 = svread_hor_za32_s32_m(svdup_s32(0), all32, 2, r);
                    svint32_t v3 = svread_hor_za32_s32_m(svdup_s32(0), all32, 3, r);
                    svst1_s16(all16, dst + (8 * kg + r) * size + 8 * jg,
                        svqrshr_n_s16_s32_x2(svcreate2_s32(v0, v1), shift));
                    svst1_s16(all16, dst + (8 * (kg + 1) + r) * size + 8 * jg,
                        svqrshr_n_s16_s32_x2(svcreate2_s32(v2, v3), shift));
                }
            }
        }
}

#endif
