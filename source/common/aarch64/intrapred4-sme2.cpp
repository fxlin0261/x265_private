#include "common.h"
#include "primitives.h"
#include "coeff-group-mask-sme2.h"
#include <arm_sme.h>
#include <stdlib.h>
namespace X265_NS {
intra_pred_t intraPred4Sme2 = 0;
namespace {
#include "intrapred4-sme2-tables.h"
}
// hotspot6
__arm_locally_streaming
void intraPredAng4_sme2_dual_tbl(pixel *dst, intptr_t stride, const pixel *src, int mode, int filter)
{
    svbool_t all = svptrue_b16();
    svbool_t sixteen = svptrue_pat_b8(SV_VL16);
    svuint8_t refs = svld1_u8(svwhilelt_b8(uint64_t(0), uint64_t(17)), src);
    svuint16_t a = svunpklo_u16(svtbl_u8(refs, svld1_u8(sixteen, index0[mode])));
    svuint16_t b = svunpklo_u16(svtbl_u8(refs, svld1_u8(sixteen, index1[mode])));
    svuint16_t values = svmla_u16_x(all, svlsl_n_u16_x(all, a, 5), svsub_u16_x(all, b, a), svld1_u16(all, fraction[mode]));
    values = svlsr_n_u16_x(all, svadd_n_u16_x(all, values, 16), 5);
    svbool_t four = svptrue_pat_b16(SV_VL4);
    svst1b_u16(four, dst, values);
    svst1b_u16(four, dst+stride, svext_u16(values, values, 4));
    svst1b_u16(four, dst+2*stride, svext_u16(values, values, 8));
    svst1b_u16(four, dst+3*stride, svext_u16(values, values, 12));
    if (filter && (mode == 10 || mode == 26))
    {
        const bool horizontal = mode == 10;
        int top = src[horizontal ? 9 : 1], corner = src[0];
        for (int i = 0; i < 4; ++i)
        {
            int value = top + ((src[(horizontal ? 1 : 9)+i] - corner) >> 1);
            dst[horizontal ? i : i*stride] = (pixel)X265_MIN(255, X265_MAX(0, value));
        }
    }
}

// hotspot6
__arm_locally_streaming
void intraPredAng4_sme2_mla(pixel *dst, intptr_t stride, const pixel *src, int mode, int filter)
{
    svbool_t all = svptrue_b16();
    svuint8_t refs = svld1_u8(svwhilelt_b8(uint64_t(0), uint64_t(17)), src);
    svuint8_t pairs = svtbl_u8(refs, svld1_u8(svptrue_b8(), pairIndex[mode]));
    svuint8_t weights = svld1_u8(svptrue_b8(), pairWeight[mode]);
    svuint16_t values = svmlalt_u16(svmullb_u16(pairs, weights), pairs, weights);
    values = svlsr_n_u16_x(all, svadd_n_u16_x(all, values, 16), 5);
    svbool_t four = svptrue_pat_b16(SV_VL4);
    svst1b_u16(four, dst, values);
    svst1b_u16(four, dst+stride, svext_u16(values, values, 4));
    svst1b_u16(four, dst+2*stride, svext_u16(values, values, 8));
    svst1b_u16(four, dst+3*stride, svext_u16(values, values, 12));
    if (filter && (mode == 10 || mode == 26))
    {
        const bool horizontal = mode == 10;
        int top = src[horizontal ? 9 : 1], corner = src[0];
        for (int i = 0; i < 4; ++i)
        {
            int value = top + ((src[(horizontal ? 1 : 9)+i] - corner) >> 1);
            dst[horizontal ? i : i*stride] = (pixel)X265_MIN(255, X265_MAX(0, value));
        }
    }
}
void setupIntraPred4Primitives_sme2(EncoderPrimitives &)
{
    const char *s = getenv("X265_SME2_INTRA_PRED4");
    const char *v = getenv("X265_SME2_INTRA_PRED4_VARIANT");
    intra_pred_t candidate = v && v[0] == '1' ? intraPredAng4_sme2_dual_tbl : intraPredAng4_sme2_mla;
    intraPred4Sme2 = s && s[0] == '1' && !s[1] && sme2Available() ? candidate : 0;
}
}
