#include "common.h"
#include "primitives.h"
#include "coeff-group-mask-sme2.h"
#include "sme2-profile.h"
#include <arm_sme.h>
#include <stdlib.h>
namespace X265_NS {
// hotspot11
__arm_locally_streaming
uint32_t quant_sme2(const int16_t *coef, const int32_t *quantCoeff, int32_t *deltaU,
                     int16_t *qCoef, int qBits, int add, int numCoeff)
{
    svbool_t all = svptrue_b32();
    svuint32_t count = svdup_u32(0);
    for (int i = 0; i < numCoeff; i += 8)
    {
        {
            svint32_t c = svld1sh_s32(all, coef+i+0);
            svint32_t tmp = svmul_s32_x(all, svabs_s32_x(all, c), svld1_s32(all, quantCoeff+i+0));
            svint32_t level = svasr_s32_x(all, svadd_n_s32_x(all, tmp, add), svdup_u32(qBits));
            svint32_t delta = svsub_s32_x(all, tmp, svlsl_s32_x(all, level, svdup_u32(qBits)));
            svst1_s32(all, deltaU+i+0, svasr_s32_x(all, delta, svdup_u32(qBits-8)));
            svbool_t nonzero = svcmpne_n_s32(all, svlsl_n_s32_x(all, level, 16), 0);
            count = svadd_n_u32_m(nonzero, count, 1);
            level = svsel_s32(svcmplt_n_s32(all, c, 0), svneg_s32_x(all, level), level);
            svst1h_s32(all, qCoef+i+0, level);
        }
    }
    return (uint32_t)svaddv_u32(all, count);
}
// hotspot11
__arm_locally_streaming
uint32_t quant_sme2_unroll(const int16_t *coef, const int32_t *quantCoeff, int32_t *deltaU,
                     int16_t *qCoef, int qBits, int add, int numCoeff)
{
    svbool_t all = svptrue_b32();
    svuint32_t count = svdup_u32(0);
    for (int i = 0; i < numCoeff; i += 16)
    {
        {
            svint32_t c = svld1sh_s32(all, coef+i+0);
            svint32_t tmp = svmul_s32_x(all, svabs_s32_x(all, c), svld1_s32(all, quantCoeff+i+0));
            svint32_t level = svasr_s32_x(all, svadd_n_s32_x(all, tmp, add), svdup_u32(qBits));
            svint32_t delta = svsub_s32_x(all, tmp, svlsl_s32_x(all, level, svdup_u32(qBits)));
            svst1_s32(all, deltaU+i+0, svasr_s32_x(all, delta, svdup_u32(qBits-8)));
            svbool_t nonzero = svcmpne_n_s32(all, svlsl_n_s32_x(all, level, 16), 0);
            count = svadd_n_u32_m(nonzero, count, 1);
            level = svsel_s32(svcmplt_n_s32(all, c, 0), svneg_s32_x(all, level), level);
            svst1h_s32(all, qCoef+i+0, level);
        }
        {
            svint32_t c = svld1sh_s32(all, coef+i+8);
            svint32_t tmp = svmul_s32_x(all, svabs_s32_x(all, c), svld1_s32(all, quantCoeff+i+8));
            svint32_t level = svasr_s32_x(all, svadd_n_s32_x(all, tmp, add), svdup_u32(qBits));
            svint32_t delta = svsub_s32_x(all, tmp, svlsl_s32_x(all, level, svdup_u32(qBits)));
            svst1_s32(all, deltaU+i+8, svasr_s32_x(all, delta, svdup_u32(qBits-8)));
            svbool_t nonzero = svcmpne_n_s32(all, svlsl_n_s32_x(all, level, 16), 0);
            count = svadd_n_u32_m(nonzero, count, 1);
            level = svsel_s32(svcmplt_n_s32(all, c, 0), svneg_s32_x(all, level), level);
            svst1h_s32(all, qCoef+i+8, level);
        }
    }
    return (uint32_t)svaddv_u32(all, count);
}
// hotspot11
__arm_locally_streaming
uint32_t quant_sme2_sign(const int16_t *coef, const int32_t *quantCoeff, int32_t *deltaU,
                     int16_t *qCoef, int qBits, int add, int numCoeff)
{
    svbool_t all = svptrue_b32();
    svuint32_t count = svdup_u32(0);
    for (int i = 0; i < numCoeff; i += 16)
    {
        {
            svint32_t c = svld1sh_s32(all, coef+i+0);
            svint32_t tmp = svmul_s32_x(all, svabs_s32_x(all, c), svld1_s32(all, quantCoeff+i+0));
            svint32_t level = svasr_s32_x(all, svadd_n_s32_x(all, tmp, add), svdup_u32(qBits));
            svint32_t delta = svsub_s32_x(all, tmp, svlsl_s32_x(all, level, svdup_u32(qBits)));
            svst1_s32(all, deltaU+i+0, svasr_s32_x(all, delta, svdup_u32(qBits-8)));
            svbool_t nonzero = svcmpne_n_s32(all, svlsl_n_s32_x(all, level, 16), 0);
            count = svadd_n_u32_m(nonzero, count, 1);
            svint32_t sign=svasr_n_s32_x(all,c,31);
            level=svsub_s32_x(all,sveor_s32_x(all,level,sign),sign);
            svst1h_s32(all, qCoef+i+0, level);
        }
        {
            svint32_t c = svld1sh_s32(all, coef+i+8);
            svint32_t tmp = svmul_s32_x(all, svabs_s32_x(all, c), svld1_s32(all, quantCoeff+i+8));
            svint32_t level = svasr_s32_x(all, svadd_n_s32_x(all, tmp, add), svdup_u32(qBits));
            svint32_t delta = svsub_s32_x(all, tmp, svlsl_s32_x(all, level, svdup_u32(qBits)));
            svst1_s32(all, deltaU+i+8, svasr_s32_x(all, delta, svdup_u32(qBits-8)));
            svbool_t nonzero = svcmpne_n_s32(all, svlsl_n_s32_x(all, level, 16), 0);
            count = svadd_n_u32_m(nonzero, count, 1);
            svint32_t sign=svasr_n_s32_x(all,c,31);
            level=svsub_s32_x(all,sveor_s32_x(all,level,sign),sign);
            svst1h_s32(all, qCoef+i+8, level);
        }
    }
    return (uint32_t)svaddv_u32(all, count);
}
// hotspot11
__attribute__((naked))
uint32_t quant_sme2_asm(const int16_t *,const int32_t *,int32_t *,int16_t *,int,int,int)
{
    __asm__ volatile(
        "sub sp, sp, #64\n"
        "stp d8, d9, [sp, #0]\n"
        "stp d10, d11, [sp, #16]\n"
        "stp d12, d13, [sp, #32]\n"
        "stp d14, d15, [sp, #48]\n"
        "smstart\tsm\n"
        "mov\tz0.s, #0x0\n"
        "cmp\tw6, #0x1\n"
        "b.lt\t.Lquant_sme2_asm_6d30f4\n"
        "sub\tw9, w4, #0x8\n"
        "mov\tz1.s, w5\n"
        "mov\tz2.s, w4\n"
        "mov\tz3.s, w9\n"
        "mov\tz0.s, #0x0\n"
        "mov\tx8, xzr\n"
        "ptrue\tp0.s\n"
        "mov\tw9, w6\n"
        ".Lquant_sme2_asm_6d3098:\n"
        "ld1sh\t{ z4.s }, p0/z, [x0, x8, lsl #1]\n"
        "ld1w\t{ z5.s }, p0/z, [x1, x8, lsl #2]\n"
        "movprfx\tz6, z4\n"
        "abs\tz6.s, p0/m, z4.s\n"
        "cmplt\tp1.s, p0/z, z4.s, #0x0\n"
        "mul\tz5.s, z6.s, z5.s\n"
        "add\tz6.s, z5.s, z1.s\n"
        "asr\tz6.s, p0/m, z6.s, z2.s\n"
        "movprfx\tz7, z6\n"
        "lsl\tz7.s, p0/m, z7.s, z2.s\n"
        "lsl\tz4.s, z6.s, #0x10\n"
        "neg\tz6.s, p1/m, z6.s\n"
        "sub\tz5.s, z5.s, z7.s\n"
        "cmpne\tp2.s, p0/z, z4.s, #0x0\n"
        "st1h\t{ z6.s }, p0, [x3, x8, lsl #1]\n"
        "movprfx\tz4, z5\n"
        "asr\tz4.s, p0/m, z4.s, z3.s\n"
        "mov\tz5.s, #0x1\n"
        "add\tz0.s, p2/m, z0.s, z5.s\n"
        "st1w\t{ z4.s }, p0, [x2, x8, lsl #2]\n"
        "add\tx8, x8, #0x8\n"
        "cmp\tx8, x9\n"
        "b.lo\t.Lquant_sme2_asm_6d3098\n"
        ".Lquant_sme2_asm_6d30f4:\n"
        "ptrue\tp0.s\n"
        "uaddv\td0, p0, z0.s\n"
        "fmov\tw0, s0\n"
        "smstop\tsm\n"
        "ldp d8, d9, [sp, #0]\n"
        "ldp d10, d11, [sp, #16]\n"
        "ldp d12, d13, [sp, #32]\n"
        "ldp d14, d15, [sp, #48]\n"
        "add sp, sp, #64\n"
        "ret\n"
    );
}

namespace {
quant_t original = 0, candidate = 0;
uint32_t dispatchQuant(const int16_t *c, const int32_t *q, int32_t *d, int16_t *o, int bits, int add, int count)
{
    recordSme2Calls(11, count/16, bits);
    return (candidate ? candidate : original)(c,q,d,o,bits,add,count);
}
}
void setupQuantPrimitives_sme2(EncoderPrimitives &p)
{
    const char *s = getenv("X265_SME2_QUANT"), *v = getenv("X265_SME2_QUANT_VARIANT");
    if (s && s[0]=='1' && !s[1] && sme2Available())
        candidate = v && v[0]=='1' ? quant_sme2 : v && v[0]=='3' ? quant_sme2_sign : v && v[0]=='2' ? quant_sme2_unroll : quant_sme2_asm;
    if ((candidate || getenv("X265_SME2_PROFILE")) && p.quant != dispatchQuant)
    {
        original=p.quant; p.quant=dispatchQuant;
    }
}
}
