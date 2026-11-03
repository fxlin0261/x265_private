#include "common.h"
#include "primitives.h"
#include "coeff-group-mask-sme2.h"
#include <arm_sme.h>
#include <stdlib.h>
namespace X265_NS {
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

static inline svuint16_t transform16x8(const pixel *a, intptr_t as,
                                     const pixel *b, intptr_t bs) __arm_streaming
{
    svbool_t all = svptrue_b16();
    svint16_t a0 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+0*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+0*bs)));
    svint16_t a1 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+1*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+1*bs)));
    svint16_t a2 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+2*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+2*bs)));
    svint16_t a3 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+3*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+3*bs)));
    svint16_t a4 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+4*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+4*bs)));
    svint16_t a5 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+5*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+5*bs)));
    svint16_t a6 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+6*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+6*bs)));
    svint16_t a7 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+7*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+7*bs)));
    svint16_t b0 = svadd_s16_x(all, a0, a1);
    svint16_t b1 = svsub_s16_x(all, a0, a1);
    svint16_t b2 = svadd_s16_x(all, a2, a3);
    svint16_t b3 = svsub_s16_x(all, a2, a3);
    svint16_t b4 = svadd_s16_x(all, a4, a5);
    svint16_t b5 = svsub_s16_x(all, a4, a5);
    svint16_t b6 = svadd_s16_x(all, a6, a7);
    svint16_t b7 = svsub_s16_x(all, a6, a7);
    svint16_t c0 = svadd_s16_x(all, b0, b2);
    svint16_t c1 = svadd_s16_x(all, b1, b3);
    svint16_t c2 = svsub_s16_x(all, b0, b2);
    svint16_t c3 = svsub_s16_x(all, b1, b3);
    svint16_t c4 = svadd_s16_x(all, b4, b6);
    svint16_t c5 = svadd_s16_x(all, b5, b7);
    svint16_t c6 = svsub_s16_x(all, b4, b6);
    svint16_t c7 = svsub_s16_x(all, b5, b7);
    svint16_t d0 = svadd_s16_x(all, c0, c4);
    svint16_t d1 = svadd_s16_x(all, c1, c5);
    svint16_t d2 = svadd_s16_x(all, c2, c6);
    svint16_t d3 = svadd_s16_x(all, c3, c7);
    svint16_t d4 = svsub_s16_x(all, c0, c4);
    svint16_t d5 = svsub_s16_x(all, c1, c5);
    svint16_t d6 = svsub_s16_x(all, c2, c6);
    svint16_t d7 = svsub_s16_x(all, c3, c7);
    svuint16_t h0 = svreinterpret_u16_s16(finish8(d0, d1));
    svuint16_t h1 = svreinterpret_u16_s16(finish8(d2, d3));
    svuint16_t h2 = svreinterpret_u16_s16(finish8(d4, d5));
    svuint16_t h3 = svreinterpret_u16_s16(finish8(d6, d7));
    return svadd_u16_x(all, svadd_u16_x(all, h0, h1), svadd_u16_x(all, h2, h3));
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

static inline svuint16_t transform16x8Multi(const pixel *a, intptr_t as,
                                     const pixel *b, intptr_t bs) __arm_streaming
{
    svbool_t all = svptrue_b16();
    svint16_t a0 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+0*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+0*bs)));
    svint16_t a1 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+1*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+1*bs)));
    svint16_t a2 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+2*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+2*bs)));
    svint16_t a3 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+3*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+3*bs)));
    svint16_t a4 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+4*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+4*bs)));
    svint16_t a5 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+5*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+5*bs)));
    svint16_t a6 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+6*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+6*bs)));
    svint16_t a7 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), a+7*as)), svreinterpret_s16_u16(svld1ub_u16(svwhilelt_b16((uint64_t)0,(uint64_t)8), b+7*bs)));
    svint16_t b0 = svadd_s16_x(all, a0, a1);
    svint16_t b1 = svsub_s16_x(all, a0, a1);
    svint16_t b2 = svadd_s16_x(all, a2, a3);
    svint16_t b3 = svsub_s16_x(all, a2, a3);
    svint16_t b4 = svadd_s16_x(all, a4, a5);
    svint16_t b5 = svsub_s16_x(all, a4, a5);
    svint16_t b6 = svadd_s16_x(all, a6, a7);
    svint16_t b7 = svsub_s16_x(all, a6, a7);
    svint16_t c0 = svadd_s16_x(all, b0, b2);
    svint16_t c1 = svadd_s16_x(all, b1, b3);
    svint16_t c2 = svsub_s16_x(all, b0, b2);
    svint16_t c3 = svsub_s16_x(all, b1, b3);
    svint16_t c4 = svadd_s16_x(all, b4, b6);
    svint16_t c5 = svadd_s16_x(all, b5, b7);
    svint16_t c6 = svsub_s16_x(all, b4, b6);
    svint16_t c7 = svsub_s16_x(all, b5, b7);
    svint16_t d0 = svadd_s16_x(all, c0, c4);
    svint16_t d1 = svadd_s16_x(all, c1, c5);
    svint16_t d2 = svadd_s16_x(all, c2, c6);
    svint16_t d3 = svadd_s16_x(all, c3, c7);
    svint16_t d4 = svsub_s16_x(all, c0, c4);
    svint16_t d5 = svsub_s16_x(all, c1, c5);
    svint16_t d6 = svsub_s16_x(all, c2, c6);
    svint16_t d7 = svsub_s16_x(all, c3, c7);
    svuint16_t h0 = svreinterpret_u16_s16(finish8Multi(d0, d1));
    svuint16_t h1 = svreinterpret_u16_s16(finish8Multi(d2, d3));
    svuint16_t h2 = svreinterpret_u16_s16(finish8Multi(d4, d5));
    svuint16_t h3 = svreinterpret_u16_s16(finish8Multi(d6, d7));
    return svadd_u16_x(all, svadd_u16_x(all, h0, h1), svadd_u16_x(all, h2, h3));
}


// hotspot18
__arm_locally_streaming
int sa8d8x8_sme2(const pixel *a,intptr_t as,const pixel *b,intptr_t bs)
{
    return (int)((svaddv_u16(svptrue_b16(),transform16x8(a,as,b,bs))+1)>>1);
}
// hotspot18
__arm_locally_streaming
int sa8d8x8_sme2_multi(const pixel *a,intptr_t as,const pixel *b,intptr_t bs)
{
    return (int)((svaddv_u16(svptrue_b16(),transform16x8Multi(a,as,b,bs))+1)>>1);
}

// hotspot18
__attribute__((naked))
int sa8d8x8_sme2_asm(const pixel *,intptr_t,const pixel *,intptr_t)
{
    __asm__ volatile(
        "sub sp, sp, #64\n"
        "stp d8, d9, [sp, #0]\n"
        "stp d10, d11, [sp, #16]\n"
        "stp d12, d13, [sp, #32]\n"
        "stp d14, d15, [sp, #48]\n"
        "smstart\tsm\n"
        "ptrue\tp0.h, vl8\n"
        "lsl\tx8, x1, #1\n"
        "lsl\tx9, x3, #1\n"
        "mov\tw10, #0x6\n"
        "sub\tx12, x2, x3\n"
        "ld1b\t{ z3.h }, p0/z, [x0, x8]\n"
        "add\tx8, x8, x1\n"
        "mul\tx11, x1, x10\n"
        "ld1b\t{ z5.h }, p0/z, [x0, x8]\n"
        "add\tx8, x9, x3\n"
        "ld1b\t{ z4.h }, p0/z, [x2, x9]\n"
        "lsl\tx9, x1, #2\n"
        "ld1b\t{ z6.h }, p0/z, [x2, x8]\n"
        "lsl\tx8, x3, #2\n"
        "mul\tx10, x3, x10\n"
        "ld1b\t{ z0.h }, p0/z, [x0]\n"
        "ld1b\t{ z1.h }, p0/z, [x2]\n"
        "ld1b\t{ z16.h }, p0/z, [x0, x9]\n"
        "add\tx9, x9, x1\n"
        "ld1b\t{ z17.h }, p0/z, [x2, x8]\n"
        "add\tx8, x8, x3\n"
        "ld1b\t{ z18.h }, p0/z, [x0, x9]\n"
        "lsl\tx9, x3, #3\n"
        "ld1b\t{ z19.h }, p0/z, [x2, x8]\n"
        "lsl\tx8, x1, #3\n"
        "ld1b\t{ z20.h }, p0/z, [x0, x11]\n"
        "sub\tx11, x0, x1\n"
        "ld1b\t{ z2.h }, p0/z, [x0, x1]\n"
        "ld1b\t{ z7.h }, p0/z, [x2, x3]\n"
        "ld1b\t{ z21.h }, p0/z, [x2, x10]\n"
        "ld1b\t{ z22.h }, p0/z, [x11, x8]\n"
        "ld1b\t{ z23.h }, p0/z, [x12, x9]\n"
        "sub\tz0.h, z0.h, z1.h\n"
        "ptrue\tp0.h\n"
        "sub\tz1.h, z2.h, z7.h\n"
        "sub\tz2.h, z3.h, z4.h\n"
        "sub\tz3.h, z5.h, z6.h\n"
        "sub\tz4.h, z16.h, z17.h\n"
        "sub\tz5.h, z18.h, z19.h\n"
        "sub\tz6.h, z20.h, z21.h\n"
        "sub\tz7.h, z22.h, z23.h\n"
        "add\tz16.h, z0.h, z1.h\n"
        "sub\tz0.h, z0.h, z1.h\n"
        "add\tz1.h, z2.h, z3.h\n"
        "sub\tz2.h, z2.h, z3.h\n"
        "add\tz3.h, z4.h, z5.h\n"
        "sub\tz4.h, z4.h, z5.h\n"
        "add\tz5.h, z6.h, z7.h\n"
        "sub\tz6.h, z6.h, z7.h\n"
        "add\tz7.h, z16.h, z1.h\n"
        "sub\tz1.h, z16.h, z1.h\n"
        "add\tz17.h, z0.h, z2.h\n"
        "sub\tz0.h, z0.h, z2.h\n"
        "add\tz16.h, z3.h, z5.h\n"
        "add\tz18.h, z4.h, z6.h\n"
        "sub\tz2.h, z3.h, z5.h\n"
        "sub\tz3.h, z4.h, z6.h\n"
        "add\tz4.h, z7.h, z16.h\n"
        "add\tz5.h, z17.h, z18.h\n"
        "add\tz6.h, z1.h, z2.h\n"
        "add\tz19.h, z0.h, z3.h\n"
        "sub\tz7.h, z7.h, z16.h\n"
        "sub\tz16.h, z17.h, z18.h\n"
        "sub\tz1.h, z1.h, z2.h\n"
        "sub\tz0.h, z0.h, z3.h\n"
        "uzp1\tz2.h, z4.h, z5.h\n"
        "uzp2\tz3.h, z4.h, z5.h\n"
        "uzp1\tz4.h, z6.h, z19.h\n"
        "uzp2\tz5.h, z6.h, z19.h\n"
        "uzp1\tz6.h, z7.h, z16.h\n"
        "uzp2\tz7.h, z7.h, z16.h\n"
        "uzp1\tz16.h, z1.h, z0.h\n"
        "uzp2\tz0.h, z1.h, z0.h\n"
        "add\tz1.h, z2.h, z3.h\n"
        "sub\tz2.h, z2.h, z3.h\n"
        "add\tz3.h, z4.h, z5.h\n"
        "sub\tz4.h, z4.h, z5.h\n"
        "add\tz5.h, z6.h, z7.h\n"
        "sub\tz6.h, z6.h, z7.h\n"
        "add\tz7.h, z16.h, z0.h\n"
        "sub\tz0.h, z16.h, z0.h\n"
        "uzp1\tz16.h, z1.h, z2.h\n"
        "uzp2\tz1.h, z1.h, z2.h\n"
        "uzp1\tz2.h, z3.h, z4.h\n"
        "uzp2\tz3.h, z3.h, z4.h\n"
        "uzp1\tz4.h, z5.h, z6.h\n"
        "uzp2\tz5.h, z5.h, z6.h\n"
        "uzp1\tz6.h, z7.h, z0.h\n"
        "uzp2\tz0.h, z7.h, z0.h\n"
        "add\tz7.h, z16.h, z1.h\n"
        "sub\tz1.h, z16.h, z1.h\n"
        "add\tz16.h, z2.h, z3.h\n"
        "sub\tz2.h, z2.h, z3.h\n"
        "add\tz3.h, z4.h, z5.h\n"
        "sub\tz4.h, z4.h, z5.h\n"
        "add\tz5.h, z6.h, z0.h\n"
        "sub\tz0.h, z6.h, z0.h\n"
        "uzp1\tz6.h, z7.h, z1.h\n"
        "uzp2\tz1.h, z7.h, z1.h\n"
        "uzp1\tz7.h, z16.h, z2.h\n"
        "uzp2\tz2.h, z16.h, z2.h\n"
        "uzp1\tz16.h, z3.h, z4.h\n"
        "uzp2\tz3.h, z3.h, z4.h\n"
        "uzp1\tz4.h, z5.h, z0.h\n"
        "uzp2\tz0.h, z5.h, z0.h\n"
        "movprfx\tz5, z6\n"
        "abs\tz5.h, p0/m, z6.h\n"
        "abs\tz1.h, p0/m, z1.h\n"
        "movprfx\tz6, z7\n"
        "abs\tz6.h, p0/m, z7.h\n"
        "abs\tz2.h, p0/m, z2.h\n"
        "movprfx\tz7, z16\n"
        "abs\tz7.h, p0/m, z16.h\n"
        "abs\tz3.h, p0/m, z3.h\n"
        "abs\tz4.h, p0/m, z4.h\n"
        "abs\tz0.h, p0/m, z0.h\n"
        "smax\tz1.h, p0/m, z1.h, z5.h\n"
        "smax\tz2.h, p0/m, z2.h, z6.h\n"
        "smax\tz3.h, p0/m, z3.h, z7.h\n"
        "smax\tz0.h, p0/m, z0.h, z4.h\n"
        "add\tz1.h, z1.h, z2.h\n"
        "add\tz0.h, z3.h, z0.h\n"
        "add\tz0.h, z1.h, z0.h\n"
        "uaddv\td0, p0, z0.h\n"
        "fmov\tx8, d0\n"
        "add\tx8, x8, #0x1\n"
        "lsr\tx0, x8, #1\n"
        "smstop\tsm\n"
        "ldp d8, d9, [sp, #0]\n"
        "ldp d10, d11, [sp, #16]\n"
        "ldp d12, d13, [sp, #32]\n"
        "ldp d14, d15, [sp, #48]\n"
        "add sp, sp, #64\n"
        "ret\n"
    );
}
void setupSa8d8Primitives_sme2(EncoderPrimitives &p)
{
    const char *s=getenv("X265_SME2_SA8D8"),*v=getenv("X265_SME2_SA8D8_VARIANT");
    if(s && s[0]=='1' && !s[1] && sme2Available())
        p.cu[BLOCK_8x8].sa8d=v && v[0]=='2'?sa8d8x8_sme2_multi:v && v[0]=='1'?sa8d8x8_sme2:sa8d8x8_sme2_asm;
}
}
