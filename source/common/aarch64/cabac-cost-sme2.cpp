#include "common.h"
#include "primitives.h"
#include "contexts.h"
#include "coeff-group-mask-sme2.h"
#include "sme2-profile.h"
#include <arm_sme.h>
#include <stdlib.h>
namespace X265_NS {
// hotspot12
__arm_locally_streaming
uint32_t costC1C2_sme2(uint16_t *absCoeff, intptr_t numC1Flag, uint8_t *baseCtxMod, intptr_t ctxOffset)
{
    uint16_t one[8],two[8];
    svbool_t pg=svwhilelt_b16((uint64_t)0,(uint64_t)numC1Flag);
    svuint16_t c=svld1_u16(pg, absCoeff);
    svst1_u16(pg, one, svdup_n_u16_z(svcmpgt_n_u16(pg,c,1),1));
    svst1_u16(pg, two, svdup_n_u16_z(svcmpgt_n_u16(pg,c,2),1));
    uint32_t sum = 0;
    uint32_t c1 = 1;
    uint32_t firstC2Idx = 8;
    uint32_t firstC2Flag = 2;
    uint32_t c1Next = 0xFFFFFFFE;

    int idx = 0;
    do
    {
        uint32_t symbol1 = one[idx];
        uint32_t symbol2 = two[idx];

        {
            const uint32_t mstate = baseCtxMod[c1];
            baseCtxMod[c1] = sbacNext(mstate, symbol1);
            sum += sbacGetEntropyBits(mstate, symbol1);
        }

        if (symbol1)
            c1Next = 0;

        if (symbol1 + firstC2Flag == 3)
            firstC2Flag = symbol2;

        if (symbol1 + firstC2Idx == 9)
            firstC2Idx  = idx;

        c1 = (c1Next & 3);
        c1Next >>= 2;
        idx++;
    }
    while(idx < numC1Flag);

    if (!c1)
    {

        baseCtxMod += ctxOffset;


        {
            const uint32_t mstate = baseCtxMod[0];
            baseCtxMod[0] = sbacNext(mstate, firstC2Flag);
            sum += sbacGetEntropyBits(mstate, firstC2Flag);
        }
    }
    return (sum & 0x00FFFFFF) + (c1 << 26) + (firstC2Idx << 28);
}
// hotspot12
__arm_locally_streaming
uint32_t costC1C2_sme2_masks(uint16_t *absCoeff, intptr_t numC1Flag, uint8_t *baseCtxMod, intptr_t ctxOffset)
{
    svbool_t pg=svwhilelt_b16((uint64_t)0,(uint64_t)numC1Flag);
    svuint16_t c=svld1_u16(pg,absCoeff);
    svuint16_t weights=svlsl_u16_x(pg,svdup_u16(1),svindex_u16(0,1));
    uint32_t one=svaddv_u16(svcmpgt_n_u16(pg,c,1),weights);
    uint32_t two=svaddv_u16(svcmpgt_n_u16(pg,c,2),weights);
    uint32_t sum = 0;
    uint32_t c1 = 1;
    uint32_t firstC2Idx = 8;
    uint32_t firstC2Flag = 2;
    uint32_t c1Next = 0xFFFFFFFE;

    int idx = 0;
    do
    {
        uint32_t symbol1 = ((one>>idx)&1);
        uint32_t symbol2 = ((two>>idx)&1);

        {
            const uint32_t mstate = baseCtxMod[c1];
            baseCtxMod[c1] = sbacNext(mstate, symbol1);
            sum += sbacGetEntropyBits(mstate, symbol1);
        }

        if (symbol1)
            c1Next = 0;

        if (symbol1 + firstC2Flag == 3)
            firstC2Flag = symbol2;

        if (symbol1 + firstC2Idx == 9)
            firstC2Idx  = idx;

        c1 = (c1Next & 3);
        c1Next >>= 2;
        idx++;
    }
    while(idx < numC1Flag);

    if (!c1)
    {

        baseCtxMod += ctxOffset;


        {
            const uint32_t mstate = baseCtxMod[0];
            baseCtxMod[0] = sbacNext(mstate, firstC2Flag);
            sum += sbacGetEntropyBits(mstate, firstC2Flag);
        }
    }
    return (sum & 0x00FFFFFF) + (c1 << 26) + (firstC2Idx << 28);
}
// hotspot12
__arm_locally_streaming
uint32_t costC1C2_sme2_clamp(uint16_t *absCoeff, intptr_t numC1Flag, uint8_t *baseCtxMod, intptr_t ctxOffset)
{
    uint16_t one[8],two[8];
    svbool_t pg=svwhilelt_b16((uint64_t)0,(uint64_t)numC1Flag);
    svuint16_t c=svld1_u16(pg, absCoeff);
    svst1_u16(pg, one, svmin_n_u16_x(pg,svqsub_n_u16(c,1),1));
    svst1_u16(pg, two, svmin_n_u16_x(pg,svqsub_n_u16(c,2),1));
    uint32_t sum = 0;
    uint32_t c1 = 1;
    uint32_t firstC2Idx = 8;
    uint32_t firstC2Flag = 2;
    uint32_t c1Next = 0xFFFFFFFE;

    int idx = 0;
    do
    {
        uint32_t symbol1 = one[idx];
        uint32_t symbol2 = two[idx];

        {
            const uint32_t mstate = baseCtxMod[c1];
            baseCtxMod[c1] = sbacNext(mstate, symbol1);
            sum += sbacGetEntropyBits(mstate, symbol1);
        }

        if (symbol1)
            c1Next = 0;

        if (symbol1 + firstC2Flag == 3)
            firstC2Flag = symbol2;

        if (symbol1 + firstC2Idx == 9)
            firstC2Idx  = idx;

        c1 = (c1Next & 3);
        c1Next >>= 2;
        idx++;
    }
    while(idx < numC1Flag);

    if (!c1)
    {

        baseCtxMod += ctxOffset;


        {
            const uint32_t mstate = baseCtxMod[0];
            baseCtxMod[0] = sbacNext(mstate, firstC2Flag);
            sum += sbacGetEntropyBits(mstate, firstC2Flag);
        }
    }
    return (sum & 0x00FFFFFF) + (c1 << 26) + (firstC2Idx << 28);
}
namespace {
costC1C2Flag_t originalC1C2=0, candidateC1C2=0;
uint32_t dispatchC1C2(uint16_t *c,intptr_t n,uint8_t *ctx,intptr_t offset)
{
    if (getenv("X265_SME2_PROFILE"))
    {
        unsigned mask=0;
        for(int i=0;i<n;++i) mask|=(unsigned)(c[i]>1)<<i;
        recordSme2Calls(12,n,mask);
    }
    return (candidateC1C2?candidateC1C2:originalC1C2)(c,n,ctx,offset);
}
}
void setupC1C2Primitives_sme2(EncoderPrimitives &p)
{
    const char *s=getenv("X265_SME2_C1C2"),*v=getenv("X265_SME2_C1C2_VARIANT");
    if(s && s[0]=='1' && !s[1] && sme2Available()) candidateC1C2=v && v[0]=='2'?costC1C2_sme2_clamp:v && v[0]=='3'?costC1C2_sme2_masks:costC1C2_sme2;
    if(candidateC1C2 || getenv("X265_SME2_PROFILE")) { originalC1C2=p.costC1C2Flag; p.costC1C2Flag=dispatchC1C2; }
}
}
