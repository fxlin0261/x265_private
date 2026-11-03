#include "common.h"
#include "primitives.h"
#include "coeff-group-mask-sme2.h"
#include <arm_sme.h>
#include <stdlib.h>

namespace X265_NS {
pixelcmp_t sa8d16x16Sme2 = 0;

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
    svint16_t a0 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+0*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+0*bs)));
    svint16_t a1 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+1*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+1*bs)));
    svint16_t a2 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+2*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+2*bs)));
    svint16_t a3 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+3*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+3*bs)));
    svint16_t a4 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+4*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+4*bs)));
    svint16_t a5 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+5*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+5*bs)));
    svint16_t a6 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+6*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+6*bs)));
    svint16_t a7 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+7*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+7*bs)));
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

// hotspot5
__arm_locally_streaming
int sa8d16x16_sme2_uzp(const pixel *a, intptr_t as, const pixel *b, intptr_t bs)
{
    svbool_t all = svptrue_b16();
    svuint16_t top = transform16x8(a, as, b, bs);
    svuint16_t bottom = transform16x8(a+8*as, as, b+8*bs, bs);

    return (int)((svaddv_u16(all, top) + svaddv_u16(all, bottom) + 1) >> 1);
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
    svint16_t a0 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+0*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+0*bs)));
    svint16_t a1 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+1*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+1*bs)));
    svint16_t a2 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+2*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+2*bs)));
    svint16_t a3 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+3*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+3*bs)));
    svint16_t a4 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+4*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+4*bs)));
    svint16_t a5 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+5*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+5*bs)));
    svint16_t a6 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+6*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+6*bs)));
    svint16_t a7 = svsub_s16_x(all, svreinterpret_s16_u16(svld1ub_u16(all, a+7*as)), svreinterpret_s16_u16(svld1ub_u16(all, b+7*bs)));
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

// hotspot5
__arm_locally_streaming
int sa8d16x16_sme2_multi_uzp(const pixel *a, intptr_t as, const pixel *b, intptr_t bs)
{
    svbool_t all = svptrue_b16();
    svuint16_t top = transform16x8Multi(a, as, b, bs);
    svuint16_t bottom = transform16x8Multi(a+8*as, as, b+8*bs, bs);

    return (int)((svaddv_u16(all, top) + svaddv_u16(all, bottom) + 1) >> 1);
}

// hotspot5
__attribute__((naked))
int sa8d16x16_sme2_asm(const pixel *, intptr_t, const pixel *, intptr_t)
{
    __asm__ volatile(
        "sub sp, sp, #64\n"
        "stp d8, d9, [sp]\n"
        "stp d10, d11, [sp, #16]\n"
        "stp d12, d13, [sp, #32]\n"
        "stp d14, d15, [sp, #48]\n"
        "smstart sm\n"
        "lsl    x8, x1, #1\n"
        "lsl    x9, x3, #1\n"
        "lsl    x10, x1, #2\n"
        "lsl    x11, x3, #2\n"
        "lsl    x5, x1, #3\n"
        "lsl    x6, x3, #3\n"
        "add    x12, x8, x1\n"
        "add    x13, x9, x3\n"
        "ptrue    p0.h\n"
        "lsl    x16, x12, #1\n"
        "lsl    x17, x13, #1\n"
        "add    x14, x10, x1\n"
        "add    x15, x11, x3\n"
        "sub    x7, x5, x1\n"
        "sub    x4, x6, x3\n"
        "ld1b    { z0.h }, p0/z, [x0]\n"
        "ld1b    { z1.h }, p0/z, [x2]\n"
        "ld1b    { z2.h }, p0/z, [x0, x1]\n"
        "ld1b    { z3.h }, p0/z, [x2, x3]\n"
        "ld1b    { z4.h }, p0/z, [x0, x8]\n"
        "ld1b    { z5.h }, p0/z, [x2, x9]\n"
        "ld1b    { z6.h }, p0/z, [x0, x12]\n"
        "ld1b    { z7.h }, p0/z, [x2, x13]\n"
        "ld1b    { z16.h }, p0/z, [x0, x10]\n"
        "ld1b    { z17.h }, p0/z, [x2, x11]\n"
        "ld1b    { z18.h }, p0/z, [x0, x14]\n"
        "ld1b    { z19.h }, p0/z, [x2, x15]\n"
        "ld1b    { z20.h }, p0/z, [x0, x16]\n"
        "ld1b    { z21.h }, p0/z, [x2, x17]\n"
        "ld1b    { z22.h }, p0/z, [x0, x7]\n"
        "ld1b    { z23.h }, p0/z, [x2, x4]\n"
        "sub    z0.h, z0.h, z1.h\n"
        "sub    z1.h, z2.h, z3.h\n"
        "sub    z2.h, z4.h, z5.h\n"
        "sub    z3.h, z6.h, z7.h\n"
        "sub    z4.h, z16.h, z17.h\n"
        "sub    z5.h, z18.h, z19.h\n"
        "sub    z6.h, z20.h, z21.h\n"
        "ld1b    { z21.h }, p0/z, [x2, x6]\n"
        "sub    z7.h, z22.h, z23.h\n"
        "add    z16.h, z0.h, z1.h\n"
        "sub    z1.h, z0.h, z1.h\n"
        "add    z0.h, z2.h, z3.h\n"
        "sub    z17.h, z2.h, z3.h\n"
        "add    x2, x2, x6\n"
        "add    z18.h, z4.h, z5.h\n"
        "sub    z5.h, z4.h, z5.h\n"
        "ld1b    { z22.h }, p0/z, [x2, x13]\n"
        "add    z4.h, z6.h, z7.h\n"
        "sub    z19.h, z6.h, z7.h\n"
        "ld1b    { z24.h }, p0/z, [x2, x11]\n"
        "add    z2.h, z16.h, z0.h\n"
        "add    z3.h, z1.h, z17.h\n"
        "sub    z0.h, z16.h, z0.h\n"
        "sub    z1.h, z1.h, z17.h\n"
        "ld1b    { z26.h }, p0/z, [x2, x15]\n"
        "ld1b    { z28.h }, p0/z, [x2, x17]\n"
        "add    z6.h, z18.h, z4.h\n"
        "add    z7.h, z5.h, z19.h\n"
        "sub    z4.h, z18.h, z4.h\n"
        "sub    z5.h, z5.h, z19.h\n"
        "ld1b    { z30.h }, p0/z, [x2, x4]\n"
        "add    z16.h, z2.h, z6.h\n"
        "add    z17.h, z3.h, z7.h\n"
        "add    z18.h, z0.h, z4.h\n"
        "add    z19.h, z1.h, z5.h\n"
        "sub    z2.h, z2.h, z6.h\n"
        "sub    z3.h, z3.h, z7.h\n"
        "sub    z4.h, z0.h, z4.h\n"
        "sub    z5.h, z1.h, z5.h\n"
        "uzp1    z20.h, z16.h, z17.h\n"
        "uzp2    z16.h, z16.h, z17.h\n"
        "uzp1    z17.h, z18.h, z19.h\n"
        "uzp2    z18.h, z18.h, z19.h\n"
        "uzp1    z1.h, z2.h, z3.h\n"
        "uzp2    z2.h, z2.h, z3.h\n"
        "add    z19.h, z20.h, z16.h\n"
        "sub    z16.h, z20.h, z16.h\n"
        "ld1b    { z20.h }, p0/z, [x0, x5]\n"
        "add    z6.h, z17.h, z18.h\n"
        "sub    z7.h, z17.h, z18.h\n"
        "add    x0, x0, x5\n"
        "ld1b    { z0.h }, p0/z, [x0, x1]\n"
        "ld1b    { z23.h }, p0/z, [x0, x10]\n"
        "ld1b    { z25.h }, p0/z, [x0, x14]\n"
        "uzp1    z18.h, z19.h, z16.h\n"
        "uzp2    z16.h, z19.h, z16.h\n"
        "sub    z17.h, z20.h, z21.h\n"
        "uzp1    z3.h, z6.h, z7.h\n"
        "uzp2    z6.h, z6.h, z7.h\n"
        "ld1b    { z19.h }, p0/z, [x0, x8]\n"
        "ld1b    { z20.h }, p0/z, [x2, x9]\n"
        "ld1b    { z21.h }, p0/z, [x0, x12]\n"
        "ld1b    { z27.h }, p0/z, [x0, x16]\n"
        "add    z7.h, z18.h, z16.h\n"
        "sub    z16.h, z18.h, z16.h\n"
        "ld1b    { z18.h }, p0/z, [x2, x3]\n"
        "ld1b    { z29.h }, p0/z, [x0, x7]\n"
        "sub    z19.h, z19.h, z20.h\n"
        "sub    z20.h, z21.h, z22.h\n"
        "sub    z21.h, z23.h, z24.h\n"
        "sub    z18.h, z0.h, z18.h\n"
        "sub    z22.h, z25.h, z26.h\n"
        "sub    z24.h, z27.h, z28.h\n"
        "sub    z25.h, z29.h, z30.h\n"
        "add    z23.h, z1.h, z2.h\n"
        "sub    z2.h, z1.h, z2.h\n"
        "uzp1    z0.h, z7.h, z16.h\n"
        "uzp2    z1.h, z7.h, z16.h\n"
        "add    z16.h, z3.h, z6.h\n"
        "add    z26.h, z17.h, z18.h\n"
        "sub    z17.h, z17.h, z18.h\n"
        "add    z18.h, z19.h, z20.h\n"
        "sub    z19.h, z19.h, z20.h\n"
        "add    z20.h, z21.h, z22.h\n"
        "sub    z21.h, z21.h, z22.h\n"
        "add    z22.h, z24.h, z25.h\n"
        "sub    z24.h, z24.h, z25.h\n"
        "uzp1    z7.h, z23.h, z2.h\n"
        "uzp2    z2.h, z23.h, z2.h\n"
        "sub    z3.h, z3.h, z6.h\n"
        "add    z6.h, z26.h, z18.h\n"
        "add    z23.h, z17.h, z19.h\n"
        "sub    z18.h, z26.h, z18.h\n"
        "sub    z17.h, z17.h, z19.h\n"
        "add    z25.h, z20.h, z22.h\n"
        "add    z27.h, z21.h, z24.h\n"
        "sub    z19.h, z20.h, z22.h\n"
        "add    z28.h, z7.h, z2.h\n"
        "sub    z7.h, z7.h, z2.h\n"
        "sub    z20.h, z21.h, z24.h\n"
        "uzp1    z2.h, z16.h, z3.h\n"
        "uzp2    z3.h, z16.h, z3.h\n"
        "uzp1    z16.h, z4.h, z5.h\n"
        "add    z21.h, z6.h, z25.h\n"
        "add    z22.h, z23.h, z27.h\n"
        "uzp2    z24.h, z4.h, z5.h\n"
        "uzp1    z4.h, z28.h, z7.h\n"
        "uzp2    z5.h, z28.h, z7.h\n"
        "add    z7.h, z18.h, z19.h\n"
        "sub    z6.h, z6.h, z25.h\n"
        "sub    z23.h, z23.h, z27.h\n"
        "sub    z18.h, z18.h, z19.h\n"
        "uzp1    z26.h, z21.h, z22.h\n"
        "uzp2    z21.h, z21.h, z22.h\n"
        "add    z22.h, z17.h, z20.h\n"
        "sub    z17.h, z17.h, z20.h\n"
        "add    z25.h, z16.h, z24.h\n"
        "sub    z16.h, z16.h, z24.h\n"
        "abs    z0.h, p0/m, z0.h\n"
        "abs    z1.h, p0/m, z1.h\n"
        "abs    z2.h, p0/m, z2.h\n"
        "uzp1    z19.h, z7.h, z22.h\n"
        "uzp2    z7.h, z7.h, z22.h\n"
        "add    z20.h, z26.h, z21.h\n"
        "sub    z21.h, z26.h, z21.h\n"
        "uzp1    z22.h, z6.h, z23.h\n"
        "uzp2    z6.h, z6.h, z23.h\n"
        "uzp1    z23.h, z18.h, z17.h\n"
        "uzp2    z17.h, z18.h, z17.h\n"
        "uzp1    z18.h, z25.h, z16.h\n"
        "uzp2    z16.h, z25.h, z16.h\n"
        "add    z24.h, z19.h, z7.h\n"
        "sub    z7.h, z19.h, z7.h\n"
        "add    z19.h, z22.h, z6.h\n"
        "sub    z6.h, z22.h, z6.h\n"
        "uzp1    z22.h, z20.h, z21.h\n"
        "uzp2    z20.h, z20.h, z21.h\n"
        "add    z21.h, z23.h, z17.h\n"
        "sub    z17.h, z23.h, z17.h\n"
        "uzp1    z23.h, z24.h, z7.h\n"
        "uzp2    z7.h, z24.h, z7.h\n"
        "add    z24.h, z18.h, z16.h\n"
        "sub    z16.h, z18.h, z16.h\n"
        "uzp1    z18.h, z19.h, z6.h\n"
        "uzp2    z6.h, z19.h, z6.h\n"
        "uzp1    z19.h, z21.h, z17.h\n"
        "uzp2    z17.h, z21.h, z17.h\n"
        "add    z21.h, z22.h, z20.h\n"
        "sub    z20.h, z22.h, z20.h\n"
        "add    z22.h, z23.h, z7.h\n"
        "sub    z7.h, z23.h, z7.h\n"
        "add    z23.h, z18.h, z6.h\n"
        "sub    z6.h, z18.h, z6.h\n"
        "uzp1    z18.h, z24.h, z16.h\n"
        "uzp2    z16.h, z24.h, z16.h\n"
        "add    z24.h, z19.h, z17.h\n"
        "sub    z17.h, z19.h, z17.h\n"
        "uzp1    z19.h, z21.h, z20.h\n"
        "uzp2    z20.h, z21.h, z20.h\n"
        "uzp1    z21.h, z22.h, z7.h\n"
        "uzp2    z7.h, z22.h, z7.h\n"
        "uzp1    z22.h, z23.h, z6.h\n"
        "uzp2    z6.h, z23.h, z6.h\n"
        "uzp1    z23.h, z24.h, z17.h\n"
        "uzp2    z17.h, z24.h, z17.h\n"
        "abs    z3.h, p0/m, z3.h\n"
        "abs    z4.h, p0/m, z4.h\n"
        "abs    z5.h, p0/m, z5.h\n"
        "abs    z18.h, p0/m, z18.h\n"
        "abs    z16.h, p0/m, z16.h\n"
        "abs    z19.h, p0/m, z19.h\n"
        "abs    z20.h, p0/m, z20.h\n"
        "abs    z21.h, p0/m, z21.h\n"
        "abs    z7.h, p0/m, z7.h\n"
        "abs    z22.h, p0/m, z22.h\n"
        "abs    z6.h, p0/m, z6.h\n"
        "abs    z23.h, p0/m, z23.h\n"
        "abs    z17.h, p0/m, z17.h\n"
        "smax    z0.h, p0/m, z0.h, z1.h\n"
        "movprfx    z1, z2\n"
        "smax    z1.h, p0/m, z1.h, z3.h\n"
        "movprfx    z2, z4\n"
        "smax    z2.h, p0/m, z2.h, z5.h\n"
        "movprfx    z3, z18\n"
        "smax    z3.h, p0/m, z3.h, z16.h\n"
        "movprfx    z4, z19\n"
        "smax    z4.h, p0/m, z4.h, z20.h\n"
        "movprfx    z5, z21\n"
        "smax    z5.h, p0/m, z5.h, z7.h\n"
        "movprfx    z7, z23\n"
        "smax    z7.h, p0/m, z7.h, z17.h\n"
        "smax    z6.h, p0/m, z6.h, z22.h\n"
        "add    z0.h, z0.h, z1.h\n"
        "add    z1.h, z2.h, z3.h\n"
        "add    z2.h, z4.h, z5.h\n"
        "add    z3.h, z6.h, z7.h\n"
        "add    z0.h, z0.h, z1.h\n"
        "add    z1.h, z2.h, z3.h\n"
        "uaddv    d0, p0, z0.h\n"
        "uaddv    d1, p0, z1.h\n"
        "fmov    x8, d0\n"
        "fmov    x9, d1\n"
        "add    x8, x8, x9\n"
        "add    x8, x8, #0x1\n"
        "lsr    x0, x8, #1\n"
        "smstop sm\n"
        "ldp d8, d9, [sp]\n"
        "ldp d10, d11, [sp, #16]\n"
        "ldp d12, d13, [sp, #32]\n"
        "ldp d14, d15, [sp, #48]\n"
        "add sp, sp, #64\n"
        "ret\n"
    );
}

void setupSa8dPrimitives_sme2(EncoderPrimitives &)
{
    const char *s = getenv("X265_SME2_SA8D");
    const char *variant = getenv("X265_SME2_SA8D_VARIANT");
    pixelcmp_t candidate = variant && variant[0] == '3' ? sa8d16x16_sme2_asm :
                           variant && variant[0] == '2' ? sa8d16x16_sme2_multi_uzp : sa8d16x16_sme2_uzp;
    sa8d16x16Sme2 = s && s[0] == '1' && !s[1] && sme2Available() ? candidate : 0;
}
}
