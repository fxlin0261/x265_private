#include "common.h"
#include "primitives.h"
#include "coeff-group-mask-sme2.h"
#include "sme2-profile.h"
#include <arm_sme.h>
#include <stdlib.h>
namespace X265_NS {
// hotspot10
__arm_locally_streaming
void saoCuStatsBO_sme2_banks8(const int16_t *diff, const pixel *rec, intptr_t stride,
                       int width, int height, int32_t *stats, int32_t *count)
{
    int32_t st[8][32], ct[8][32];
    svbool_t all = svptrue_b32();
    for (int bank=0; bank<8; ++bank)
        for (int bin=0; bin<32; bin+=8)
        {
            svst1_s32(all, st[bank]+bin, svdup_s32(0));
            svst1_s32(all, ct[bank]+bin, svdup_s32(0));
        }
    for (int y=0; y<height; ++y)
    {
        for (int x=0; x<width; ++x)
        {
            int bin = rec[x] >> 3, bank = x & 7;
            st[bank][bin] += diff[x];
            ct[bank][bin]++;
        }
        rec += stride; diff += MAX_CU_SIZE;
    }
    for (int bin=0; bin<32; bin+=8)
    {
        svint32_t sum = svld1_s32(all, stats+bin), num = svld1_s32(all, count+bin);
        for (int bank=0; bank<8; ++bank)
        {
            sum = svadd_s32_x(all, sum, svld1_s32(all, st[bank]+bin));
            num = svadd_s32_x(all, num, svld1_s32(all, ct[bank]+bin));
        }
        svst1_s32(all, stats+bin, sum); svst1_s32(all, count+bin, num);
    }
}

// hotspot10
__arm_locally_streaming
void saoCuStatsBO_sme2_banks4(const int16_t *diff, const pixel *rec, intptr_t stride,
                       int width, int height, int32_t *stats, int32_t *count)
{
    int32_t st[4][32], ct[4][32];
    svbool_t all = svptrue_b32();
    for (int bank=0; bank<4; ++bank)
        for (int bin=0; bin<32; bin+=8)
        {
            svst1_s32(all, st[bank]+bin, svdup_s32(0));
            svst1_s32(all, ct[bank]+bin, svdup_s32(0));
        }
    for (int y=0; y<height; ++y)
    {
        int x=0;
        for (; x+8<=width; x+=8)
        {
            #pragma clang loop unroll(full)
            for (int i=0; i<8; ++i)
            {
                int bin = rec[x+i] >> 3;
                st[i&3][bin] += diff[x+i];
                ct[i&3][bin]++;
            }
        }
        for (int i=0; x+i<width; ++i)
        {
            int bin = rec[x+i] >> 3;
            st[i&3][bin] += diff[x+i];
            ct[i&3][bin]++;
        }
        rec += stride; diff += MAX_CU_SIZE;
    }
    for (int bin=0; bin<32; bin+=8)
    {
        svint32_t sum = svld1_s32(all, stats+bin), num = svld1_s32(all, count+bin);
        for (int bank=0; bank<4; ++bank)
        {
            sum = svadd_s32_x(all, sum, svld1_s32(all, st[bank]+bin));
            num = svadd_s32_x(all, num, svld1_s32(all, ct[bank]+bin));
        }
        svst1_s32(all, stats+bin, sum); svst1_s32(all, count+bin, num);
    }
}

// hotspot10
__arm_locally_streaming
void saoCuStatsBO_sme2_banks2(const int16_t *diff, const pixel *rec, intptr_t stride,
                       int width, int height, int32_t *stats, int32_t *count)
{
    int32_t st[2][32], ct[2][32];
    svbool_t all = svptrue_b32();
    for (int bank=0; bank<2; ++bank)
        for (int bin=0; bin<32; bin+=8)
        {
            svst1_s32(all, st[bank]+bin, bank ? svdup_s32(0) : svld1_s32(all, stats+bin));
            svst1_s32(all, ct[bank]+bin, bank ? svdup_s32(0) : svld1_s32(all, count+bin));
        }
    for (int y=0; y<height; ++y)
    {
        int x=0;
        for (; x+8<=width; x+=8)
        {
            uint64_t pixels;
            memcpy(&pixels, rec+x, sizeof(pixels));
            #pragma clang loop unroll(full)
            for (int i=0; i<8; ++i)
            {
                int bin = (pixels >> (8*i+3)) & 31;
                st[i&1][bin] += diff[x+i];
                ct[i&1][bin]++;
            }
        }
        for (int i=0; x+i<width; ++i)
        {
            int bin = rec[x+i] >> 3;
            st[i&1][bin] += diff[x+i];
            ct[i&1][bin]++;
        }
        rec += stride; diff += MAX_CU_SIZE;
    }
    for (int bin=0; bin<32; bin+=8)
    {
        svint32_t sum = svdup_s32(0), num = svdup_s32(0);
        for (int bank=0; bank<2; ++bank)
        {
            sum = svadd_s32_x(all, sum, svld1_s32(all, st[bank]+bin));
            num = svadd_s32_x(all, num, svld1_s32(all, ct[bank]+bin));
        }
        svst1_s32(all, stats+bin, sum); svst1_s32(all, count+bin, num);
    }
}

// hotspot10
__arm_locally_streaming
void saoCuStatsBO_sme2_predicated(const int16_t *diff, const pixel *rec, intptr_t stride,
                         int width, int height, int32_t *stats, int32_t *count)
{
    svbool_t all = svptrue_b32();
    for (int bin=0; bin<32; ++bin)
    {
        svint32_t sum = svdup_s32(0), num = svdup_s32(0);
        for (int y=0; y<height; ++y)
            for (int x=0; x<width; x+=8)
            {
                svbool_t pg = svwhilelt_b32(x, width);
                svuint32_t classes = svlsr_n_u32_x(pg, svld1ub_u32(pg, rec+y*stride+x), 3);
                svbool_t match = svcmpeq_n_u32(pg, classes, bin);
                sum = svadd_s32_m(match, sum, svld1sh_s32(pg, diff+y*MAX_CU_SIZE+x));
                num = svadd_n_s32_m(match, num, 1);
            }
        stats[bin] += (int32_t)svaddv_s32(all, sum);
        count[bin] += (int32_t)svaddv_s32(all, num);
    }
}
namespace {
typedef void (*bo_t)(const int16_t *, const pixel *, intptr_t, int, int, int32_t *, int32_t *);
bo_t original = 0, candidate = 0;
void dispatch(const int16_t *diff, const pixel *rec, intptr_t stride,
              int width, int height, int32_t *stats, int32_t *count)
{
    recordSme2Calls(10,width,height);
    (candidate ? candidate : original)(diff,rec,stride,width,height,stats,count);
}
}
void setupSaoStatsPrimitives_sme2(EncoderPrimitives &p)
{
    const char *s = getenv("X265_SME2_SAO_STATS"), *v = getenv("X265_SME2_SAO_STATS_VARIANT");
    if (s && s[0] == '1' && !s[1] && sme2Available())
        candidate = v && v[0] == '1' ? saoCuStatsBO_sme2_banks8 :
                    v && v[0] == '2' ? saoCuStatsBO_sme2_predicated :
                    v && v[0] == '3' ? saoCuStatsBO_sme2_banks4 : saoCuStatsBO_sme2_banks2;
    if ((candidate || getenv("X265_SME2_PROFILE")) && p.saoCuStatsBO != dispatch)
    {
        original = p.saoCuStatsBO;
        p.saoCuStatsBO = dispatch;
    }
}
}
