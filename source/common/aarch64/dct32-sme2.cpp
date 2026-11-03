#include "common.h"
#include "primitives.h"
#include "coeff-group-mask-sme2.h"
#include <arm_sme.h>
#include <stdlib.h>
namespace X265_NS {
namespace {
#include "dct32-sme2-tables.h"

__attribute__((always_inline)) static inline
void packRows(const int16_t *src, intptr_t stride, int16_t packed[4][16][16]) __arm_streaming __arm_inout("za")
{
    svbool_t all16 = svptrue_b16(), all32 = svptrue_b32();
    for (int g = 0; g < 4; ++g)
    {
        for (int r = 0; r < 8; ++r)
        {
            svwrite_hor_za32_s32_m(0, r, all32, svreinterpret_s32_s16(svld1_s16(all16, src+(8*g+r)*stride)));
            svwrite_hor_za32_s32_m(1, r, all32, svreinterpret_s32_s16(svld1_s16(all16, src+(8*g+r)*stride+16)));
        }
        for (int c = 0; c < 8; ++c)
        {
            svst1_s16(all16, packed[g][c], svreinterpret_s16_s32(svread_ver_za32_s32_m(svdup_s32(0), all32, 0, c)));
            svst1_s16(all16, packed[g][c+8], svreinterpret_s16_s32(svread_ver_za32_s32_m(svdup_s32(0), all32, 1, c)));
        }
    }
}

template<int shift>
__attribute__((always_inline)) static inline
void transform(const int16_t packed[4][16][16], int16_t *dst) __arm_streaming __arm_inout("za")
{
    svbool_t all16 = svptrue_b16(), all32 = svptrue_b32();
    for (int kg = 0; kg < 4; kg += 2)
        for (int jg = 0; jg < 4; jg += 2)
        {
            svzero_za();
            for (int pair = 0; pair < 16; ++pair)
            {
                svint16_t k0 = svld1_s16(all16, packedDct32[kg][pair]);
                svint16_t k1 = svld1_s16(all16, packedDct32[kg+1][pair]);
                svint16_t j0 = svld1_s16(all16, packed[jg][pair]);
                svint16_t j1 = svld1_s16(all16, packed[jg+1][pair]);
                svmopa_za32_s16_m(0, all16, all16, k0, j0);
                svmopa_za32_s16_m(1, all16, all16, k0, j1);
                svmopa_za32_s16_m(2, all16, all16, k1, j0);
                svmopa_za32_s16_m(3, all16, all16, k1, j1);
            }
            for (int r = 0; r < 8; ++r)
            {
                svint32_t v0 = svread_hor_za32_s32_m(svdup_s32(0), all32, 0, r);
                svint32_t v1 = svread_hor_za32_s32_m(svdup_s32(0), all32, 1, r);
                svint32_t v2 = svread_hor_za32_s32_m(svdup_s32(0), all32, 2, r);
                svint32_t v3 = svread_hor_za32_s32_m(svdup_s32(0), all32, 3, r);
                svst1h_s32(all32, dst+(8*kg+r)*32+8*jg, svasr_n_s32_x(all32, svadd_n_s32_x(all32, v0, 1<<(shift-1)), shift));
                svst1h_s32(all32, dst+(8*kg+r)*32+8*(jg+1), svasr_n_s32_x(all32, svadd_n_s32_x(all32, v1, 1<<(shift-1)), shift));
                svst1h_s32(all32, dst+(8*(kg+1)+r)*32+8*jg, svasr_n_s32_x(all32, svadd_n_s32_x(all32, v2, 1<<(shift-1)), shift));
                svst1h_s32(all32, dst+(8*(kg+1)+r)*32+8*(jg+1), svasr_n_s32_x(all32, svadd_n_s32_x(all32, v3, 1<<(shift-1)), shift));
            }
        }
}

__attribute__((always_inline)) static inline
void packRowsCompact(const int16_t *src, intptr_t stride, int16_t packed[4][16][16]) __arm_streaming __arm_inout("za")
{
    svbool_t all16 = svptrue_b16(), all32 = svptrue_b32();
    #pragma clang loop unroll(disable)
    for (int g = 0; g < 4; ++g)
    {
        #pragma clang loop unroll(disable)
        for (int r = 0; r < 8; ++r)
        {
            svwrite_hor_za32_s32_m(0, r, all32, svreinterpret_s32_s16(svld1_s16(all16, src+(8*g+r)*stride)));
            svwrite_hor_za32_s32_m(1, r, all32, svreinterpret_s32_s16(svld1_s16(all16, src+(8*g+r)*stride+16)));
        }
        #pragma clang loop unroll(disable)
        for (int c = 0; c < 8; ++c)
        {
            svst1_s16(all16, packed[g][c], svreinterpret_s16_s32(svread_ver_za32_s32_m(svdup_s32(0), all32, 0, c)));
            svst1_s16(all16, packed[g][c+8], svreinterpret_s16_s32(svread_ver_za32_s32_m(svdup_s32(0), all32, 1, c)));
        }
    }
}

template<int shift>
__attribute__((always_inline)) static inline
void transformCompact(const int16_t packed[4][16][16], int16_t *dst) __arm_streaming __arm_inout("za")
{
    svbool_t all16 = svptrue_b16(), all32 = svptrue_b32();
    #pragma clang loop unroll(disable)
    for (int kg = 0; kg < 4; kg += 2)
        #pragma clang loop unroll(disable)
        for (int jg = 0; jg < 4; jg += 2)
        {
            svzero_za();
            #pragma clang loop unroll(disable)
            for (int pair = 0; pair < 16; ++pair)
            {
                svint16_t k0 = svld1_s16(all16, packedDct32[kg][pair]);
                svint16_t k1 = svld1_s16(all16, packedDct32[kg+1][pair]);
                svint16_t j0 = svld1_s16(all16, packed[jg][pair]);
                svint16_t j1 = svld1_s16(all16, packed[jg+1][pair]);
                svmopa_za32_s16_m(0, all16, all16, k0, j0);
                svmopa_za32_s16_m(1, all16, all16, k0, j1);
                svmopa_za32_s16_m(2, all16, all16, k1, j0);
                svmopa_za32_s16_m(3, all16, all16, k1, j1);
            }
            #pragma clang loop unroll(disable)
            for (int r = 0; r < 8; ++r)
            {
                svint32_t v0 = svread_hor_za32_s32_m(svdup_s32(0), all32, 0, r);
                svint32_t v1 = svread_hor_za32_s32_m(svdup_s32(0), all32, 1, r);
                svint32_t v2 = svread_hor_za32_s32_m(svdup_s32(0), all32, 2, r);
                svint32_t v3 = svread_hor_za32_s32_m(svdup_s32(0), all32, 3, r);
                svst1h_s32(all32, dst+(8*kg+r)*32+8*jg, svasr_n_s32_x(all32, svadd_n_s32_x(all32, v0, 1<<(shift-1)), shift));
                svst1h_s32(all32, dst+(8*kg+r)*32+8*(jg+1), svasr_n_s32_x(all32, svadd_n_s32_x(all32, v1, 1<<(shift-1)), shift));
                svst1h_s32(all32, dst+(8*(kg+1)+r)*32+8*jg, svasr_n_s32_x(all32, svadd_n_s32_x(all32, v2, 1<<(shift-1)), shift));
                svst1h_s32(all32, dst+(8*(kg+1)+r)*32+8*(jg+1), svasr_n_s32_x(all32, svadd_n_s32_x(all32, v3, 1<<(shift-1)), shift));
            }
        }
}

template<int shift>
__attribute__((always_inline)) static inline
void transformPacked(const int16_t packed[4][16][16], int16_t dst[4][16][16]) __arm_streaming __arm_inout("za")
{
    svbool_t all16 = svptrue_b16(), all32 = svptrue_b32();
    for (int kg = 0; kg < 4; kg += 2)
        for (int jg = 0; jg < 4; jg += 2)
        {
            svzero_za();
            for (int pair = 0; pair < 16; ++pair)
            {
                svint16_t k0 = svld1_s16(all16, packedDct32[kg][pair]);
                svint16_t k1 = svld1_s16(all16, packedDct32[kg+1][pair]);
                svint16_t j0 = svld1_s16(all16, packed[jg][pair]);
                svint16_t j1 = svld1_s16(all16, packed[jg+1][pair]);
                svmopa_za32_s16_m(0, all16, all16, k0, j0);
                svmopa_za32_s16_m(1, all16, all16, k0, j1);
                svmopa_za32_s16_m(2, all16, all16, k1, j0);
                svmopa_za32_s16_m(3, all16, all16, k1, j1);
            }
            for (int c = 0; c < 4; ++c)
            {
                svint32_t a0 = svread_ver_za32_s32_m(svdup_s32(0), all32, 0, 2*c);
                svint32_t b0 = svread_ver_za32_s32_m(svdup_s32(0), all32, 0, 2*c+1);
                a0 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, a0, 1<<(shift-1)), shift);
                b0 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, b0, 1<<(shift-1)), shift);
                svint16_t result0 = svqxtnt_s32(svqxtnb_s32(a0), b0);
                svst1_s16(all16, dst[kg][4*jg+c], result0);
                svint32_t a1 = svread_ver_za32_s32_m(svdup_s32(0), all32, 1, 2*c);
                svint32_t b1 = svread_ver_za32_s32_m(svdup_s32(0), all32, 1, 2*c+1);
                a1 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, a1, 1<<(shift-1)), shift);
                b1 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, b1, 1<<(shift-1)), shift);
                svint16_t result1 = svqxtnt_s32(svqxtnb_s32(a1), b1);
                svst1_s16(all16, dst[kg][4*jg+4+c], result1);
                svint32_t a2 = svread_ver_za32_s32_m(svdup_s32(0), all32, 2, 2*c);
                svint32_t b2 = svread_ver_za32_s32_m(svdup_s32(0), all32, 2, 2*c+1);
                a2 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, a2, 1<<(shift-1)), shift);
                b2 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, b2, 1<<(shift-1)), shift);
                svint16_t result2 = svqxtnt_s32(svqxtnb_s32(a2), b2);
                svst1_s16(all16, dst[kg+1][4*jg+c], result2);
                svint32_t a3 = svread_ver_za32_s32_m(svdup_s32(0), all32, 3, 2*c);
                svint32_t b3 = svread_ver_za32_s32_m(svdup_s32(0), all32, 3, 2*c+1);
                a3 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, a3, 1<<(shift-1)), shift);
                b3 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, b3, 1<<(shift-1)), shift);
                svint16_t result3 = svqxtnt_s32(svqxtnb_s32(a3), b3);
                svst1_s16(all16, dst[kg+1][4*jg+4+c], result3);
            }
        }
}

template<int shift>
__attribute__((always_inline)) static inline
void transformPackedCompact(const int16_t packed[4][16][16], int16_t dst[4][16][16]) __arm_streaming __arm_inout("za")
{
    svbool_t all16 = svptrue_b16(), all32 = svptrue_b32();
    #pragma clang loop unroll(disable)
    for (int kg = 0; kg < 4; kg += 2)
        #pragma clang loop unroll(disable)
        for (int jg = 0; jg < 4; jg += 2)
        {
            svzero_za();
            #pragma clang loop unroll(disable)
            for (int pair = 0; pair < 16; ++pair)
            {
                svint16_t k0 = svld1_s16(all16, packedDct32[kg][pair]);
                svint16_t k1 = svld1_s16(all16, packedDct32[kg+1][pair]);
                svint16_t j0 = svld1_s16(all16, packed[jg][pair]);
                svint16_t j1 = svld1_s16(all16, packed[jg+1][pair]);
                svmopa_za32_s16_m(0, all16, all16, k0, j0);
                svmopa_za32_s16_m(1, all16, all16, k0, j1);
                svmopa_za32_s16_m(2, all16, all16, k1, j0);
                svmopa_za32_s16_m(3, all16, all16, k1, j1);
            }
            #pragma clang loop unroll(disable)
            for (int c = 0; c < 4; ++c)
            {
                svint32_t a0 = svread_ver_za32_s32_m(svdup_s32(0), all32, 0, 2*c);
                svint32_t b0 = svread_ver_za32_s32_m(svdup_s32(0), all32, 0, 2*c+1);
                a0 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, a0, 1<<(shift-1)), shift);
                b0 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, b0, 1<<(shift-1)), shift);
                svint16_t result0 = svqxtnt_s32(svqxtnb_s32(a0), b0);
                svst1_s16(all16, dst[kg][4*jg+c], result0);
                svint32_t a1 = svread_ver_za32_s32_m(svdup_s32(0), all32, 1, 2*c);
                svint32_t b1 = svread_ver_za32_s32_m(svdup_s32(0), all32, 1, 2*c+1);
                a1 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, a1, 1<<(shift-1)), shift);
                b1 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, b1, 1<<(shift-1)), shift);
                svint16_t result1 = svqxtnt_s32(svqxtnb_s32(a1), b1);
                svst1_s16(all16, dst[kg][4*jg+4+c], result1);
                svint32_t a2 = svread_ver_za32_s32_m(svdup_s32(0), all32, 2, 2*c);
                svint32_t b2 = svread_ver_za32_s32_m(svdup_s32(0), all32, 2, 2*c+1);
                a2 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, a2, 1<<(shift-1)), shift);
                b2 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, b2, 1<<(shift-1)), shift);
                svint16_t result2 = svqxtnt_s32(svqxtnb_s32(a2), b2);
                svst1_s16(all16, dst[kg+1][4*jg+c], result2);
                svint32_t a3 = svread_ver_za32_s32_m(svdup_s32(0), all32, 3, 2*c);
                svint32_t b3 = svread_ver_za32_s32_m(svdup_s32(0), all32, 3, 2*c+1);
                a3 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, a3, 1<<(shift-1)), shift);
                b3 = svasr_n_s32_x(all32, svadd_n_s32_x(all32, b3, 1<<(shift-1)), shift);
                svint16_t result3 = svqxtnt_s32(svqxtnb_s32(a3), b3);
                svst1_s16(all16, dst[kg+1][4*jg+4+c], result3);
            }
        }
}

}
// hotspot9
__arm_new("za") __arm_locally_streaming
void dct32_sme2(const int16_t *src, int16_t *dst, intptr_t stride)
{
    int16_t packed[4][16][16], tmp[32*32];
    packRows(src, stride, packed);
    transform<4>(packed, tmp);
    packRows(tmp, 32, packed);
    transform<11>(packed, dst);
}

// hotspot9
__arm_new("za") __arm_locally_streaming
void dct32_sme2_compact(const int16_t *src, int16_t *dst, intptr_t stride)
{
    int16_t packed[4][16][16], tmp[32*32];
    packRowsCompact(src, stride, packed);
    transformCompact<4>(packed, tmp);
    packRowsCompact(tmp, 32, packed);
    transformCompact<11>(packed, dst);
}
// hotspot9
__arm_new("za") __arm_locally_streaming
void dct32_sme2_packed(const int16_t *src, int16_t *dst, intptr_t stride)
{
    int16_t packed[4][16][16], next[4][16][16];
    packRows(src, stride, packed);

    transformPacked<4>(packed, next);
    transform<11>(next, dst);
}

// hotspot9
__arm_new("za") __arm_locally_streaming
void dct32_sme2_packed_compact(const int16_t *src, int16_t *dst, intptr_t stride)
{
    int16_t packed[4][16][16], next[4][16][16];
    packRowsCompact(src, stride, packed);
    transformPackedCompact<4>(packed, next);
    transformCompact<11>(next, dst);
}
void setupDct32Primitives_sme2(EncoderPrimitives &p)
{
    const char *s = getenv("X265_SME2_DCT32");
    const char *v = getenv("X265_SME2_DCT32_VARIANT");
    if (s && s[0] == '1' && !s[1] && sme2Available())
        p.cu[BLOCK_32x32].dct = v && v[0] == '1' ? dct32_sme2 :
                                  v && v[0] == '2' ? dct32_sme2_compact :
                                  v && v[0] == '3' ? dct32_sme2_packed : dct32_sme2_packed_compact;
}
}
