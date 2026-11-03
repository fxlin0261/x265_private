#include "common.h"
#include "primitives.h"
#include "coeff-group-mask-sme2.h"
#include <arm_sme.h>
#include <stdlib.h>

namespace X265_NS {

static inline svint16_t loadRowPair(const pixel *a, intptr_t as,
                                    const pixel *b, intptr_t bs, int y) __arm_streaming
{
    svbool_t eight = svptrue_pat_b16(SV_VL8);
    svint16_t top = svsub_s16_x(eight, svreinterpret_s16_u16(svld1ub_u16(eight, a+y*as)),
                                     svreinterpret_s16_u16(svld1ub_u16(eight, b+y*bs)));
    svint16_t bottom = svsub_s16_x(eight, svreinterpret_s16_u16(svld1ub_u16(eight, a+(y+4)*as)),
                                        svreinterpret_s16_u16(svld1ub_u16(eight, b+(y+4)*bs)));
    return svsplice_s16(eight, top, bottom);
}

static inline svint16_t horizontal(svint16_t v, svuint16_t swap1,
                                  svuint16_t swap2, svbool_t even) __arm_streaming
{
    svbool_t all = svptrue_b16();
    svint16_t other = svtbl_s16(v, swap1);
    v = svsel_s16(even, svadd_s16_x(all, v, other), svsub_s16_x(all, v, other));
    v = svabs_s16_x(all, v);

    return svmax_s16_x(all, v, svtbl_s16(v, swap2));
}

// hotspot3
__arm_locally_streaming
int satd8x8_sme2_tbl(const pixel *a, intptr_t as, const pixel *b, intptr_t bs)
{
    svbool_t all = svptrue_b16();
    svint16_t a0 = loadRowPair(a, as, b, bs, 0);
    svint16_t a1 = loadRowPair(a, as, b, bs, 1);
    svint16_t a2 = loadRowPair(a, as, b, bs, 2);
    svint16_t a3 = loadRowPair(a, as, b, bs, 3);
    svint16_t s0 = svadd_s16_x(all, a0, a1);
    svint16_t d0 = svsub_s16_x(all, a0, a1);
    svint16_t s1 = svadd_s16_x(all, a2, a3);
    svint16_t d1 = svsub_s16_x(all, a2, a3);
    svuint16_t index = svindex_u16(0, 1);
    svuint16_t swap1 = sveor_n_u16_x(all, index, 1);
    svuint16_t swap2 = sveor_n_u16_x(all, index, 2);
    svbool_t even = svcmpeq_n_u16(all, svand_n_u16_x(all, index, 1), 0);
    svint16_t h0 = horizontal(svadd_s16_x(all, s0, s1), swap1, swap2, even);
    svint16_t h1 = horizontal(svsub_s16_x(all, s0, s1), swap1, swap2, even);
    svint16_t h2 = horizontal(svadd_s16_x(all, d0, d1), swap1, swap2, even);
    svint16_t h3 = horizontal(svsub_s16_x(all, d0, d1), swap1, swap2, even);

    return (int)(svaddv_s16(all, svadd_s16_x(all, svadd_s16_x(all, h0, h1), svadd_s16_x(all, h2, h3))) >> 1);
}

static inline svint16_t finishPair(svint16_t a, svint16_t b) __arm_streaming
{
    svbool_t all = svptrue_b16();
    svint16_t even = svuzp1_s16(a, b), odd = svuzp2_s16(a, b);
    svint16_t sum = svadd_s16_x(all, even, odd), diff = svsub_s16_x(all, even, odd);
    return svmax_s16_x(all, svabs_s16_x(all, svuzp1_s16(sum, diff)),
                           svabs_s16_x(all, svuzp2_s16(sum, diff)));
}

// hotspot3
__arm_locally_streaming
int satd8x8_sme2_uzp(const pixel *a, intptr_t as, const pixel *b, intptr_t bs)
{
    svbool_t all = svptrue_b16();
    svint16_t a0 = loadRowPair(a, as, b, bs, 0), a1 = loadRowPair(a, as, b, bs, 1);
    svint16_t a2 = loadRowPair(a, as, b, bs, 2), a3 = loadRowPair(a, as, b, bs, 3);
    svint16_t s0 = svadd_s16_x(all, a0, a1), d0 = svsub_s16_x(all, a0, a1);
    svint16_t s1 = svadd_s16_x(all, a2, a3), d1 = svsub_s16_x(all, a2, a3);
    svint16_t h0 = finishPair(svadd_s16_x(all, s0, s1), svsub_s16_x(all, s0, s1));
    svint16_t h1 = finishPair(svadd_s16_x(all, d0, d1), svsub_s16_x(all, d0, d1));
    return (int)svaddv_s16(all, svadd_s16_x(all, h0, h1));
}

static inline svint16_t finishPairMulti(svint16_t a, svint16_t b) __arm_streaming
{
    svbool_t all = svptrue_b16();
    svint16x2_t pair = svuzp_s16_x2(svcreate2_s16(a, b));
    svint16_t even = svget2_s16(pair, 0), odd = svget2_s16(pair, 1);
    svint16_t sum = svadd_s16_x(all, even, odd), diff = svsub_s16_x(all, even, odd);
    pair = svuzp_s16_x2(svcreate2_s16(sum, diff));
    return svmax_s16_x(all, svabs_s16_x(all, svget2_s16(pair, 0)),
                           svabs_s16_x(all, svget2_s16(pair, 1)));
}

// hotspot3
__arm_locally_streaming
int satd8x8_sme2_multi_uzp(const pixel *a, intptr_t as, const pixel *b, intptr_t bs)
{
    svbool_t all = svptrue_b16();
    svint16_t a0 = loadRowPair(a, as, b, bs, 0), a1 = loadRowPair(a, as, b, bs, 1);
    svint16_t a2 = loadRowPair(a, as, b, bs, 2), a3 = loadRowPair(a, as, b, bs, 3);
    svint16_t s0 = svadd_s16_x(all, a0, a1), d0 = svsub_s16_x(all, a0, a1);
    svint16_t s1 = svadd_s16_x(all, a2, a3), d1 = svsub_s16_x(all, a2, a3);
    svint16_t h0 = finishPairMulti(svadd_s16_x(all, s0, s1), svsub_s16_x(all, s0, s1));
    svint16_t h1 = finishPairMulti(svadd_s16_x(all, d0, d1), svsub_s16_x(all, d0, d1));
    return (int)svaddv_s16(all, svadd_s16_x(all, h0, h1));
}

// hotspot3
__attribute__((naked))
int satd8x8_sme2_asm(const pixel *, intptr_t, const pixel *, intptr_t)
{
    __asm__ volatile(
        "sub sp, sp, #64\n"
        "stp d8, d9, [sp]\n"
        "stp d10, d11, [sp, #16]\n"
        "stp d12, d13, [sp, #32]\n"
        "stp d14, d15, [sp, #48]\n"
        "smstart sm\n"
        "ptrue    p0.h, vl8\n"
        "lsl    x8, x1, #2\n"
        "mov    w9, #0x6\n"
        "lsl    x10, x3, #2\n"
        "mul    x11, x1, x9\n"
        "sub    x12, x2, x3\n"
        "ld1b    { z2.h }, p0/z, [x0, x8]\n"
        "add    x8, x8, x1\n"
        "ld1b    { z0.h }, p0/z, [x0]\n"
        "ld1b    { z3.h }, p0/z, [x2, x10]\n"
        "add    x10, x10, x3\n"
        "ld1b    { z4.h }, p0/z, [x0, x8]\n"
        "lsl    x8, x1, #1\n"
        "mul    x9, x3, x9\n"
        "ld1b    { z5.h }, p0/z, [x2, x10]\n"
        "lsl    x10, x3, #1\n"
        "ld1b    { z1.h }, p0/z, [x2]\n"
        "ld1b    { z6.h }, p0/z, [x0, x1]\n"
        "ld1b    { z16.h }, p0/z, [x0, x8]\n"
        "add    x8, x8, x1\n"
        "ld1b    { z18.h }, p0/z, [x0, x11]\n"
        "ld1b    { z17.h }, p0/z, [x2, x10]\n"
        "ld1b    { z19.h }, p0/z, [x0, x8]\n"
        "add    x8, x10, x3\n"
        "lsl    x10, x1, #3\n"
        "lsl    x11, x3, #3\n"
        "ld1b    { z7.h }, p0/z, [x2, x3]\n"
        "ld1b    { z20.h }, p0/z, [x2, x8]\n"
        "sub    x8, x0, x1\n"
        "ld1b    { z21.h }, p0/z, [x2, x9]\n"
        "ld1b    { z22.h }, p0/z, [x8, x10]\n"
        "ld1b    { z23.h }, p0/z, [x12, x11]\n"
        "sub    z0.h, z0.h, z1.h\n"
        "sub    z1.h, z2.h, z3.h\n"
        "sub    z2.h, z6.h, z7.h\n"
        "sub    z6.h, z16.h, z17.h\n"
        "sub    z16.h, z19.h, z20.h\n"
        "sub    z3.h, z4.h, z5.h\n"
        "sub    z7.h, z18.h, z21.h\n"
        "sub    z17.h, z22.h, z23.h\n"
        "splice    z0.h, p0, { z0.h, z1.h }\n"
        "splice    z1.h, p0, { z2.h, z3.h }\n"
        "splice    z2.h, p0, { z6.h, z7.h }\n"
        "splice    z3.h, p0, { z16.h, z17.h }\n"
        "ptrue    p0.h\n"
        "add    z4.h, z0.h, z1.h\n"
        "sub    z0.h, z0.h, z1.h\n"
        "add    z5.h, z2.h, z3.h\n"
        "sub    z1.h, z2.h, z3.h\n"
        "add    z2.h, z4.h, z5.h\n"
        "sub    z3.h, z4.h, z5.h\n"
        "add    z4.h, z0.h, z1.h\n"
        "sub    z0.h, z0.h, z1.h\n"
        "uzp1    z1.h, z2.h, z3.h\n"
        "uzp2    z2.h, z2.h, z3.h\n"
        "uzp1    z3.h, z4.h, z0.h\n"
        "uzp2    z0.h, z4.h, z0.h\n"
        "add    z4.h, z1.h, z2.h\n"
        "sub    z1.h, z1.h, z2.h\n"
        "add    z2.h, z3.h, z0.h\n"
        "sub    z0.h, z3.h, z0.h\n"
        "uzp1    z3.h, z4.h, z1.h\n"
        "uzp2    z1.h, z4.h, z1.h\n"
        "uzp1    z4.h, z2.h, z0.h\n"
        "uzp2    z0.h, z2.h, z0.h\n"
        "abs    z3.h, p0/m, z3.h\n"
        "abs    z1.h, p0/m, z1.h\n"
        "abs    z4.h, p0/m, z4.h\n"
        "abs    z0.h, p0/m, z0.h\n"
        "smax    z1.h, p0/m, z1.h, z3.h\n"
        "smax    z0.h, p0/m, z0.h, z4.h\n"
        "add    z0.h, z1.h, z0.h\n"
        "saddv    d0, p0, z0.h\n"
        "fmov    w0, s0\n"
        "smstop sm\n"
        "ldp d8, d9, [sp]\n"
        "ldp d10, d11, [sp, #16]\n"
        "ldp d12, d13, [sp, #32]\n"
        "ldp d14, d15, [sp, #48]\n"
        "add sp, sp, #64\n"
        "ret\n"
    );
}

void setupSatdPrimitives_sme2(EncoderPrimitives &p)
{
    const char *setting = getenv("X265_SME2_SATD");
    if (!setting || setting[0] != '1' || setting[1] || !sme2Available()) return;
    const char *variant = getenv("X265_SME2_SATD_VARIANT");
    pixelcmp_t candidate = variant && variant[0] == '1' ? satd8x8_sme2_tbl :
                           variant && variant[0] == '2' ? satd8x8_sme2_uzp :
                           variant && variant[0] == '4' ? satd8x8_sme2_multi_uzp : satd8x8_sme2_asm;
    pixelcmp_t original = p.pu[LUMA_8x8].satd;
    for (int i = 0; i < NUM_PU_SIZES; ++i)
        if (p.pu[i].satd == original) p.pu[i].satd = candidate;
    for (int c = 0; c < 3; ++c)
        for (int i = 0; i < NUM_PU_SIZES; ++i)
            if (p.chroma[c].pu[i].satd == original) p.chroma[c].pu[i].satd = candidate;
}
}
