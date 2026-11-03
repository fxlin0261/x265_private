#include "common.h"
#include "primitives.h"
#include "coeff-group-mask-sme2.h"
#include <arm_sme.h>
#include <stdlib.h>
namespace X265_NS {
namespace {
static const int16_t packedHadamard8[4][16] __attribute__((aligned(32))) = {
    {1,1,1,-1,1,1,1,-1,1,1,1,-1,1,1,1,-1},
    {1,1,1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,-1,1},
    {1,1,1,-1,1,1,1,-1,-1,-1,-1,1,-1,-1,-1,1},
    {1,1,1,-1,-1,-1,-1,1,-1,-1,-1,1,1,1,1,-1}
};

template<bool packed>
__attribute__((always_inline)) static inline
int energyDifferenceMatrix(const pixel *a, intptr_t as, const pixel *b, intptr_t bs) __arm_streaming __arm_inout("za")
{
    svbool_t eight = svptrue_pat_b16(SV_VL8), all16 = svptrue_b16(), all32 = svptrue_b32();
    svzero_za();
    for (int pair = 0; pair < 4; ++pair)
    {
        svint16_t k = svld1_s16(all16, packedHadamard8[pair]);
        svint16_t a0 = svreinterpret_s16_u16(svld1ub_u16(eight, a + 2 * pair * as));
        svint16_t a1 = svreinterpret_s16_u16(svld1ub_u16(eight, a + (2 * pair + 1) * as));
        svint16_t b0 = svreinterpret_s16_u16(svld1ub_u16(eight, b + 2 * pair * bs));
        svint16_t b1 = svreinterpret_s16_u16(svld1ub_u16(eight, b + (2 * pair + 1) * bs));
        svmopa_za32_s16_m(0, all16, all16, k, svzip1_s16(a0, a1));
        svmopa_za32_s16_m(1, all16, all16, k, svzip1_s16(b0, b1));
    }
    if (packed)
    {
        for (int pair = 0; pair < 4; ++pair)
        {
            svint16_t a0 = svqcvtn_s16_s32_x2(svread_ver_za32_s32_vg2(0, 2 * pair));
            svint16_t b0 = svqcvtn_s16_s32_x2(svread_ver_za32_s32_vg2(1, 2 * pair));
            svint16_t k = svld1_s16(all16, packedHadamard8[pair]);
            svmopa_za32_s16_m(2, all16, all16, k, a0);
            svmopa_za32_s16_m(3, all16, all16, k, b0);
        }
        svint16_t a0 = svqcvtn_s16_s32_x2(svread_hor_za32_s32_vg2(2, 0));
        svint16_t b0 = svqcvtn_s16_s32_x2(svread_hor_za32_s32_vg2(3, 0));
        int dc0 = svlastb_s16(svptrue_pat_b16(SV_VL1), a0) >> 2;
        int dc1 = svlastb_s16(svptrue_pat_b16(SV_VL1), b0) >> 2;
        svuint16_t s0 = svreinterpret_u16_s16(svabs_s16_x(all16, a0));
        svuint16_t s1 = svreinterpret_u16_s16(svabs_s16_x(all16, b0));
        for (int r = 2; r < 8; r += 2)
        {
            a0 = svqcvtn_s16_s32_x2(svread_hor_za32_s32_vg2(2, r));
            b0 = svqcvtn_s16_s32_x2(svread_hor_za32_s32_vg2(3, r));
            s0 = svadd_u16_x(all16, s0, svreinterpret_u16_s16(svabs_s16_x(all16, a0)));
            s1 = svadd_u16_x(all16, s1, svreinterpret_u16_s16(svabs_s16_x(all16, b0)));
        }
        int e0 = ((svaddv_u16(all16, s0) + 2) >> 2) - dc0;
        int e1 = ((svaddv_u16(all16, s1) + 2) >> 2) - dc1;
        return abs(e0 - e1);
    }
    svint16_t a0 = svqcvtn_s16_s32_x2(svread_ver_za32_s32_vg2(0, 0));
    svint16_t a1 = svqcvtn_s16_s32_x2(svread_ver_za32_s32_vg2(0, 2));
    svint16_t a2 = svqcvtn_s16_s32_x2(svread_ver_za32_s32_vg2(0, 4));
    svint16_t a3 = svqcvtn_s16_s32_x2(svread_ver_za32_s32_vg2(0, 6));
    svint16_t b0 = svqcvtn_s16_s32_x2(svread_ver_za32_s32_vg2(1, 0));
    svint16_t b1 = svqcvtn_s16_s32_x2(svread_ver_za32_s32_vg2(1, 2));
    svint16_t b2 = svqcvtn_s16_s32_x2(svread_ver_za32_s32_vg2(1, 4));
    svint16_t b3 = svqcvtn_s16_s32_x2(svread_ver_za32_s32_vg2(1, 6));
    svzero_za();
    svint16_t k = svld1_s16(all16, packedHadamard8[0]);
    svmopa_za32_s16_m(0, all16, all16, k, a0);
    svmopa_za32_s16_m(1, all16, all16, k, b0);
    k = svld1_s16(all16, packedHadamard8[1]);
    svmopa_za32_s16_m(0, all16, all16, k, a1);
    svmopa_za32_s16_m(1, all16, all16, k, b1);
    k = svld1_s16(all16, packedHadamard8[2]);
    svmopa_za32_s16_m(0, all16, all16, k, a2);
    svmopa_za32_s16_m(1, all16, all16, k, b2);
    k = svld1_s16(all16, packedHadamard8[3]);
    svmopa_za32_s16_m(0, all16, all16, k, a3);
    svmopa_za32_s16_m(1, all16, all16, k, b3);
    svint32_t s0 = svread_hor_za32_s32_m(svdup_s32(0), all32, 0, 0);
    svint32_t s1 = svread_hor_za32_s32_m(svdup_s32(0), all32, 1, 0);
    int dc0 = svlastb_s32(svptrue_pat_b32(SV_VL1), s0) >> 2;
    int dc1 = svlastb_s32(svptrue_pat_b32(SV_VL1), s1) >> 2;
    s0 = svabs_s32_x(all32, s0);
    s1 = svabs_s32_x(all32, s1);
    for (int r = 1; r < 8; ++r)
    {
        svint32_t v0 = svread_hor_za32_s32_m(svdup_s32(0), all32, 0, r);
        svint32_t v1 = svread_hor_za32_s32_m(svdup_s32(0), all32, 1, r);
        s0 = svadd_s32_x(all32, s0, svabs_s32_x(all32, v0));
        s1 = svadd_s32_x(all32, s1, svabs_s32_x(all32, v1));
    }
    int e0 = ((svaddv_s32(all32, s0) + 2) >> 2) - dc0;
    int e1 = ((svaddv_s32(all32, s1) + 2) >> 2) - dc1;
    return abs(e0 - e1);
}

__attribute__((always_inline)) static inline
svint16_t horizontal8(svint16_t a) __arm_streaming
{
    svbool_t pg=svptrue_b16();
    svuint16_t idx=svindex_u16(0,1);
    {
        svint16_t b=svtbl_s16(a,sveor_n_u16_x(pg,idx,1));
        a=svsel_s16(svcmpeq_n_u16(pg,svand_n_u16_x(pg,idx,1),0),svadd_s16_x(pg,a,b),svsub_s16_x(pg,b,a));
    }
    {
        svint16_t b=svtbl_s16(a,sveor_n_u16_x(pg,idx,2));
        a=svsel_s16(svcmpeq_n_u16(pg,svand_n_u16_x(pg,idx,2),0),svadd_s16_x(pg,a,b),svsub_s16_x(pg,b,a));
    }
    return svmax_s16_x(pg,svabs_s16_x(pg,a),svabs_s16_x(pg,svtbl_s16(a,sveor_n_u16_x(pg,idx,4))));
}
__attribute__((always_inline)) static inline
uint64_t energyPair(const pixel *a,intptr_t as,const pixel *b,intptr_t bs) __arm_streaming
{
    svbool_t pg=svptrue_b16(),lo=svwhilelt_b16((uint64_t)0,(uint64_t)8);
    svbool_t hi=svnot_b_z(pg,lo);
    svint16_t a0=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+0*as),svld1ub_u16(lo,b+0*bs)));
    svint16_t a1=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+1*as),svld1ub_u16(lo,b+1*bs)));
    svint16_t a2=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+2*as),svld1ub_u16(lo,b+2*bs)));
    svint16_t a3=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+3*as),svld1ub_u16(lo,b+3*bs)));
    svint16_t a4=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+4*as),svld1ub_u16(lo,b+4*bs)));
    svint16_t a5=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+5*as),svld1ub_u16(lo,b+5*bs)));
    svint16_t a6=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+6*as),svld1ub_u16(lo,b+6*bs)));
    svint16_t a7=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+7*as),svld1ub_u16(lo,b+7*bs)));
    svint16_t b0=svadd_s16_x(pg,a0,a1),b1=svsub_s16_x(pg,a0,a1);
    svint16_t b2=svadd_s16_x(pg,a2,a3),b3=svsub_s16_x(pg,a2,a3);
    svint16_t b4=svadd_s16_x(pg,a4,a5),b5=svsub_s16_x(pg,a4,a5);
    svint16_t b6=svadd_s16_x(pg,a6,a7),b7=svsub_s16_x(pg,a6,a7);
    svint16_t c0=svadd_s16_x(pg,b0,b2),c2=svsub_s16_x(pg,b0,b2);
    svint16_t c1=svadd_s16_x(pg,b1,b3),c3=svsub_s16_x(pg,b1,b3);
    svint16_t c4=svadd_s16_x(pg,b4,b6),c6=svsub_s16_x(pg,b4,b6);
    svint16_t c5=svadd_s16_x(pg,b5,b7),c7=svsub_s16_x(pg,b5,b7);
    svint16_t d0=svadd_s16_x(pg,c0,c4),d4=svsub_s16_x(pg,c0,c4);
    svint16_t d1=svadd_s16_x(pg,c1,c5),d5=svsub_s16_x(pg,c1,c5);
    svint16_t d2=svadd_s16_x(pg,c2,c6),d6=svsub_s16_x(pg,c2,c6);
    svint16_t d3=svadd_s16_x(pg,c3,c7),d7=svsub_s16_x(pg,c3,c7);
    int dc0=svaddv_s16(lo,d0)>>2,dc1=svaddv_s16(hi,d0)>>2;
    svuint16_t sum=svreinterpret_u16_s16(horizontal8(d0));
    sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(horizontal8(d1)));
    sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(horizontal8(d2)));
    sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(horizontal8(d3)));
    sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(horizontal8(d4)));
    sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(horizontal8(d5)));
    sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(horizontal8(d6)));
    sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(horizontal8(d7)));
    uint32_t e0=((svaddv_u16(lo,sum)+2)>>2)-dc0;
    uint32_t e1=((svaddv_u16(hi,sum)+2)>>2)-dc1;
    return ((uint64_t)e1<<32)|e0;
}
static inline svint16_t finish8(svint16_t a, svint16_t b) __arm_streaming
{
    svbool_t all = svptrue_b16();
    svint16_t even = svuzp1_s16(a, b), odd = svuzp2_s16(a, b);
    svint16_t sum = svadd_s16_x(all, even, odd), diff = svsub_s16_x(all, even, odd);
    even = svuzp1_s16(sum, diff); odd = svuzp2_s16(sum, diff);
    sum = svadd_s16_x(all, even, odd); diff = svsub_s16_x(all, even, odd);
    return svmax_s16_x(all, svabs_s16_x(all, svuzp1_s16(sum, diff)),
                           svabs_s16_x(all, svuzp2_s16(sum, diff)));
}

static inline svint16_t finish8Multi(svint16_t a, svint16_t b) __arm_streaming
{
    svbool_t all = svptrue_b16();
    svint16x2_t pair = svuzp_s16_x2(svcreate2_s16(a, b));
    svint16_t even = svget2_s16(pair, 0), odd = svget2_s16(pair, 1);
    svint16_t sum = svadd_s16_x(all, even, odd), diff = svsub_s16_x(all, even, odd);
    pair = svuzp_s16_x2(svcreate2_s16(sum, diff));
    even = svget2_s16(pair, 0); odd = svget2_s16(pair, 1);
    sum = svadd_s16_x(all, even, odd); diff = svsub_s16_x(all, even, odd);
    pair = svuzp_s16_x2(svcreate2_s16(sum, diff));
    return svmax_s16_x(all, svabs_s16_x(all, svget2_s16(pair, 0)),
                           svabs_s16_x(all, svget2_s16(pair, 1)));
}

__attribute__((always_inline)) static inline
uint64_t energyPairUzp(const pixel *a,intptr_t as,const pixel *b,intptr_t bs) __arm_streaming
{
    svbool_t pg=svptrue_b16(),lo=svwhilelt_b16((uint64_t)0,(uint64_t)8);
    svbool_t hi=svnot_b_z(pg,lo);
    svint16_t a0=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+0*as),svld1ub_u16(lo,b+0*bs)));
    svint16_t a1=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+1*as),svld1ub_u16(lo,b+1*bs)));
    svint16_t a2=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+2*as),svld1ub_u16(lo,b+2*bs)));
    svint16_t a3=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+3*as),svld1ub_u16(lo,b+3*bs)));
    svint16_t a4=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+4*as),svld1ub_u16(lo,b+4*bs)));
    svint16_t a5=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+5*as),svld1ub_u16(lo,b+5*bs)));
    svint16_t a6=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+6*as),svld1ub_u16(lo,b+6*bs)));
    svint16_t a7=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+7*as),svld1ub_u16(lo,b+7*bs)));
    svint16_t b0=svadd_s16_x(pg,a0,a1),b1=svsub_s16_x(pg,a0,a1);
    svint16_t b2=svadd_s16_x(pg,a2,a3),b3=svsub_s16_x(pg,a2,a3);
    svint16_t b4=svadd_s16_x(pg,a4,a5),b5=svsub_s16_x(pg,a4,a5);
    svint16_t b6=svadd_s16_x(pg,a6,a7),b7=svsub_s16_x(pg,a6,a7);
    svint16_t c0=svadd_s16_x(pg,b0,b2),c2=svsub_s16_x(pg,b0,b2);
    svint16_t c1=svadd_s16_x(pg,b1,b3),c3=svsub_s16_x(pg,b1,b3);
    svint16_t c4=svadd_s16_x(pg,b4,b6),c6=svsub_s16_x(pg,b4,b6);
    svint16_t c5=svadd_s16_x(pg,b5,b7),c7=svsub_s16_x(pg,b5,b7);
    svint16_t d0=svadd_s16_x(pg,c0,c4),d4=svsub_s16_x(pg,c0,c4);
    svint16_t d1=svadd_s16_x(pg,c1,c5),d5=svsub_s16_x(pg,c1,c5);
    svint16_t d2=svadd_s16_x(pg,c2,c6),d6=svsub_s16_x(pg,c2,c6);
    svint16_t d3=svadd_s16_x(pg,c3,c7),d7=svsub_s16_x(pg,c3,c7);
    int dc0=svaddv_s16(lo,d0)>>2,dc1=svaddv_s16(hi,d0)>>2;
    svuint16_t sum=svreinterpret_u16_s16(finish8(d0,d1));
    sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(finish8(d2,d3)));
    sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(finish8(d4,d5)));
    sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(finish8(d6,d7)));
    svbool_t even=svcmpeq_n_u16(pg,svand_n_u16_x(pg,svindex_u16(0,1),1),0);
    uint32_t e0=((svaddv_u16(even,sum)+1)>>1)-dc0;
    uint32_t e1=((svaddv_u16(svnot_b_z(pg,even),sum)+1)>>1)-dc1;
    return ((uint64_t)e1<<32)|e0;
}
__attribute__((always_inline)) static inline
svint16x2_t finish8Magnitudes(svint16_t a, svint16_t b) __arm_streaming
{
    svbool_t all = svptrue_b16();
    svint16x2_t pair = svuzp_s16_x2(svcreate2_s16(a, b));
    svint16_t even = svget2_s16(pair, 0), odd = svget2_s16(pair, 1);
    pair = svuzp_s16_x2(svcreate2_s16(svadd_s16_x(all, even, odd), svsub_s16_x(all, even, odd)));
    even = svget2_s16(pair, 0); odd = svget2_s16(pair, 1);
    pair = svuzp_s16_x2(svcreate2_s16(svadd_s16_x(all, even, odd), svsub_s16_x(all, even, odd)));
    return svcreate2_s16(svabs_s16_x(all, svget2_s16(pair, 0)), svabs_s16_x(all, svget2_s16(pair, 1)));
}

template<bool grouped>
__attribute__((always_inline)) static inline
uint64_t energyPairGrouped(const pixel *a,intptr_t as,const pixel *b,intptr_t bs) __arm_streaming
{
    svbool_t pg=svptrue_b16(),lo=svwhilelt_b16((uint64_t)0,(uint64_t)8);
    svbool_t hi=svnot_b_z(pg,lo);
    svint16_t a0=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+0*as),svld1ub_u16(lo,b+0*bs)));
    svint16_t a1=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+1*as),svld1ub_u16(lo,b+1*bs)));
    svint16_t a2=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+2*as),svld1ub_u16(lo,b+2*bs)));
    svint16_t a3=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+3*as),svld1ub_u16(lo,b+3*bs)));
    svint16_t a4=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+4*as),svld1ub_u16(lo,b+4*bs)));
    svint16_t a5=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+5*as),svld1ub_u16(lo,b+5*bs)));
    svint16_t a6=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+6*as),svld1ub_u16(lo,b+6*bs)));
    svint16_t a7=svreinterpret_s16_u16(svsplice_u16(lo,svld1ub_u16(lo,a+7*as),svld1ub_u16(lo,b+7*bs)));
    svint16_t b0=svadd_s16_x(pg,a0,a1),b1=svsub_s16_x(pg,a0,a1);
    svint16_t b2=svadd_s16_x(pg,a2,a3),b3=svsub_s16_x(pg,a2,a3);
    svint16_t b4=svadd_s16_x(pg,a4,a5),b5=svsub_s16_x(pg,a4,a5);
    svint16_t b6=svadd_s16_x(pg,a6,a7),b7=svsub_s16_x(pg,a6,a7);
    svint16_t c0=svadd_s16_x(pg,b0,b2),c2=svsub_s16_x(pg,b0,b2);
    svint16_t c1=svadd_s16_x(pg,b1,b3),c3=svsub_s16_x(pg,b1,b3);
    svint16_t c4=svadd_s16_x(pg,b4,b6),c6=svsub_s16_x(pg,b4,b6);
    svint16_t c5=svadd_s16_x(pg,b5,b7),c7=svsub_s16_x(pg,b5,b7);
    svint16_t d0=svadd_s16_x(pg,c0,c4),d4=svsub_s16_x(pg,c0,c4);
    svint16_t d1=svadd_s16_x(pg,c1,c5),d5=svsub_s16_x(pg,c1,c5);
    svint16_t d2=svadd_s16_x(pg,c2,c6),d6=svsub_s16_x(pg,c2,c6);
    svint16_t d3=svadd_s16_x(pg,c3,c7),d7=svsub_s16_x(pg,c3,c7);
    int dc0=svaddv_s16(lo,d0)>>2,dc1=svaddv_s16(hi,d0)>>2;
    svuint16_t sum;
    if (grouped)
    {
        svint16x2_t p0 = finish8Magnitudes(d0, d1), p1 = finish8Magnitudes(d2, d3);
        svint16x2_t p2 = finish8Magnitudes(d4, d5), p3 = finish8Magnitudes(d6, d7);
        svint16x4_t maxima = svmax_s16_x4(
            svcreate4_s16(svget2_s16(p0, 0), svget2_s16(p1, 0), svget2_s16(p2, 0), svget2_s16(p3, 0)),
            svcreate4_s16(svget2_s16(p0, 1), svget2_s16(p1, 1), svget2_s16(p2, 1), svget2_s16(p3, 1)));
        sum = svadd_u16_x(pg, svreinterpret_u16_s16(svget4_s16(maxima, 0)), svreinterpret_u16_s16(svget4_s16(maxima, 1)));
        sum = svadd_u16_x(pg, sum, svreinterpret_u16_s16(svget4_s16(maxima, 2)));
        sum = svadd_u16_x(pg, sum, svreinterpret_u16_s16(svget4_s16(maxima, 3)));
    }
    else
    {
        sum=svreinterpret_u16_s16(finish8Multi(d0,d1));
        sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(finish8Multi(d2,d3)));
        sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(finish8Multi(d4,d5)));
        sum=svadd_u16_x(pg,sum,svreinterpret_u16_s16(finish8Multi(d6,d7)));
    }
    svbool_t even=svcmpeq_n_u16(pg,svand_n_u16_x(pg,svindex_u16(0,1),1),0);
    uint32_t e0=((svaddv_u16(even,sum)+1)>>1)-dc0;
    uint32_t e1=((svaddv_u16(svnot_b_z(pg,even),sum)+1)>>1)-dc1;
    return ((uint64_t)e1<<32)|e0;
}
}
// hotspot14
__arm_locally_streaming
int psyCost32_sme2(const pixel *a,intptr_t as,const pixel *b,intptr_t bs)
{
    int sum=0;
    for(int y=0;y<32;y+=8)
        for(int x=0;x<32;x+=8)
        {
            uint64_t e=energyPair(a+y*as+x,as,b+y*bs+x,bs);
            sum+=abs((int)(uint32_t)e-(int)(e>>32));
        }
    return sum;
}
// hotspot14
__arm_locally_streaming
int psyCost32_sme2_separate(const pixel *a,intptr_t as,const pixel *b,intptr_t bs)
{
    int sum=0;
    for(int y=0;y<32;y+=8)
        for(int x=0;x<32;x+=8)
        {
            uint64_t ea=energyPair(a+y*as+x,as,a+y*as+x,as);
            uint64_t eb=energyPair(b+y*bs+x,bs,b+y*bs+x,bs);
            sum+=abs((int)(uint32_t)ea-(int)(uint32_t)eb);
        }
    return sum;
}
// hotspot20
__arm_locally_streaming
int psyCost64_sme2(const pixel *a,intptr_t as,const pixel *b,intptr_t bs)
{
    int sum=0;
    for(int y=0;y<64;y+=8)
        for(int x=0;x<64;x+=8)
        {
            uint64_t e=energyPair(a+y*as+x,as,b+y*bs+x,bs);
            sum+=abs((int)(uint32_t)e-(int)(e>>32));
        }
    return sum;
}
// hotspot20
__arm_locally_streaming
int psyCost64_sme2_separate(const pixel *a,intptr_t as,const pixel *b,intptr_t bs)
{
    int sum=0;
    for(int y=0;y<64;y+=8)
        for(int x=0;x<64;x+=8)
        {
            uint64_t ea=energyPair(a+y*as+x,as,a+y*as+x,as);
            uint64_t eb=energyPair(b+y*bs+x,bs,b+y*bs+x,bs);
            sum+=abs((int)(uint32_t)ea-(int)(uint32_t)eb);
        }
    return sum;
}
// hotspot14
__arm_locally_streaming
int psyCost32_sme2_uzp(const pixel *a,intptr_t as,const pixel *b,intptr_t bs)
{
    int sum=0;
    for(int y=0;y<32;y+=8)
        for(int x=0;x<32;x+=8)
        {
            uint64_t e=energyPairUzp(a+y*as+x,as,b+y*bs+x,bs);
            sum+=abs((int)(uint32_t)e-(int)(e>>32));
        }
    return sum;
}
// hotspot14
__arm_locally_streaming
int psyCost32_sme2_multi(const pixel *a,intptr_t as,const pixel *b,intptr_t bs)
{
    int sum=0;
    for(int y=0;y<32;y+=8)
        for(int x=0;x<32;x+=8)
        {
            uint64_t e=energyPairGrouped<false>(a+y*as+x,as,b+y*bs+x,bs);
            sum+=abs((int)(uint32_t)e-(int)(e>>32));
        }
    return sum;
}
// hotspot20
__arm_locally_streaming
int psyCost64_sme2_uzp(const pixel *a,intptr_t as,const pixel *b,intptr_t bs)
{
    int sum=0;
    for(int y=0;y<64;y+=8)
        for(int x=0;x<64;x+=8)
        {
            uint64_t e=energyPairUzp(a+y*as+x,as,b+y*bs+x,bs);
            sum+=abs((int)(uint32_t)e-(int)(e>>32));
        }
    return sum;
}
// hotspot20
__arm_locally_streaming
int psyCost64_sme2_multi(const pixel *a,intptr_t as,const pixel *b,intptr_t bs)
{
    int sum=0;
    for(int y=0;y<64;y+=8)
        for(int x=0;x<64;x+=8)
        {
            uint64_t e=energyPairGrouped<false>(a+y*as+x,as,b+y*bs+x,bs);
            sum+=abs((int)(uint32_t)e-(int)(e>>32));
        }
    return sum;
}
// hotspot14
__arm_new("za") __arm_locally_streaming
int psyCost32_sme2_matrix(const pixel *a, intptr_t as, const pixel *b, intptr_t bs)
{
    int sum = 0;
    for (int y = 0; y < 32; y += 8)
        for (int x = 0; x < 32; x += 8)
            sum += energyDifferenceMatrix<false>(a + y * as + x, as, b + y * bs + x, bs);
    return sum;
}

// hotspot20
__arm_new("za") __arm_locally_streaming
int psyCost64_sme2_matrix(const pixel *a, intptr_t as, const pixel *b, intptr_t bs)
{
    int sum = 0;
    for (int y = 0; y < 64; y += 8)
        for (int x = 0; x < 64; x += 8)
            sum += energyDifferenceMatrix<false>(a + y * as + x, as, b + y * bs + x, bs);
    return sum;
}

// hotspot14
__arm_locally_streaming
int psyCost32_sme2_wide(const pixel *a, intptr_t as, const pixel *b, intptr_t bs)
{
    int sum = 0;
    for (int y = 0; y < 32; y += 8)
        for (int x = 0; x < 32; x += 8)
        {
            uint64_t energy = energyPairGrouped<true>(a + y * as + x, as, b + y * bs + x, bs);
            sum += abs((int)(uint32_t)energy - (int)(energy >> 32));
        }
    return sum;
}

// hotspot20
__arm_locally_streaming
int psyCost64_sme2_wide(const pixel *a, intptr_t as, const pixel *b, intptr_t bs)
{
    int sum = 0;
    for (int y = 0; y < 64; y += 8)
        for (int x = 0; x < 64; x += 8)
        {
            uint64_t energy = energyPairGrouped<true>(a + y * as + x, as, b + y * bs + x, bs);
            sum += abs((int)(uint32_t)energy - (int)(energy >> 32));
        }
    return sum;
}

// hotspot14
__arm_new("za") __arm_locally_streaming
int psyCost32_sme2_matrix_packed(const pixel *a, intptr_t as, const pixel *b, intptr_t bs)
{
    int sum = 0;
    for (int y = 0; y < 32; y += 8)
        for (int x = 0; x < 32; x += 8)
            sum += energyDifferenceMatrix<true>(a + y * as + x, as, b + y * bs + x, bs);
    return sum;
}

// hotspot20
__arm_new("za") __arm_locally_streaming
int psyCost64_sme2_matrix_packed(const pixel *a, intptr_t as, const pixel *b, intptr_t bs)
{
    int sum = 0;
    for (int y = 0; y < 64; y += 8)
        for (int x = 0; x < 64; x += 8)
            sum += energyDifferenceMatrix<true>(a + y * as + x, as, b + y * bs + x, bs);
    return sum;
}

void setupPsyCostPrimitives_sme2(EncoderPrimitives &p)
{
    const char *s=getenv("X265_SME2_PSY_COST"),*v=getenv("X265_SME2_PSY_COST_VARIANT");
    if(s && s[0]=='1' && !s[1] && sme2Available())
    {
        p.cu[BLOCK_32x32].psy_cost_pp=v && v[0]=='7'?psyCost32_sme2_matrix_packed:v && v[0]=='6'?psyCost32_sme2_wide:v && v[0]=='5'?psyCost32_sme2_matrix:v && v[0]=='1'?psyCost32_sme2:v && v[0]=='2'?psyCost32_sme2_separate:v && v[0]=='4'?psyCost32_sme2_multi:psyCost32_sme2_uzp;
        p.cu[BLOCK_64x64].psy_cost_pp=v && v[0]=='7'?psyCost64_sme2_matrix_packed:v && v[0]=='6'?psyCost64_sme2_wide:v && v[0]=='5'?psyCost64_sme2_matrix:v && v[0]=='1'?psyCost64_sme2:v && v[0]=='2'?psyCost64_sme2_separate:v && v[0]=='4'?psyCost64_sme2_multi:psyCost64_sme2_uzp;
    }
}
}
