#include "common.h"
#include "primitives.h"
#include "sme2.h"
#include <arm_sme.h>
#include <stdlib.h>

namespace X265_NS {

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

void setupSaoStatsPrimitives_sme2(EncoderPrimitives &p)
{
    const char *s = getenv("X265_SME2_SAO_STATS");
    if (s && s[0] == '1' && !s[1] && sme2Available())
        p.saoCuStatsBO = saoCuStatsBO_sme2_banks2;
}
}
