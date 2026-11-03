#include "common.h"
#include "primitives.h"
#include "coeff-group-mask-sme2.h"
#include <arm_sme.h>
#include <stdlib.h>
namespace X265_NS {
intra_pred_t intraPred8Sme2 = 0;
namespace {
#include "intrapred8-sme2-tables.h"
}
// hotspot8
__arm_locally_streaming
void intraPredAng8_sme2_tbl2(pixel *dst, intptr_t stride, const pixel *src, int mode, int filter)
{
    svbool_t all = svptrue_b16();
    svuint8x2_t refs = svcreate2_u8(svld1_u8(svptrue_b8(), src),
                                   svld1_u8(svptrue_pat_b8(SV_VL1), src+32));
    svbool_t eight = svptrue_pat_b16(SV_VL8);
    for (int row = 0; row < 8; row += 2)
    {
        svuint8_t pairs = svtbl2_u8(refs, svld1_u8(svptrue_b8(), pairIndex[mode]+16*row));
        svuint8_t weights = svld1_u8(svptrue_b8(), pairWeight[mode]+16*row);
        svuint16_t values = svmlalt_u16(svmullb_u16(pairs, weights), pairs, weights);
        values = svlsr_n_u16_x(all, svadd_n_u16_x(all, values, 16), 5);
        svst1b_u16(eight, dst+row*stride, values);
        svst1b_u16(eight, dst+(row+1)*stride, svext_u16(values, values, 8));
    }
    if (filter && (mode == 10 || mode == 26))
    {
        const bool horizontal = mode == 10;
        int top = src[horizontal ? 17 : 1], corner = src[0];
        for (int i = 0; i < 8; ++i)
        {
            int value = top + ((src[(horizontal ? 1 : 17)+i] - corner) >> 1);
            dst[horizontal ? i : i*stride] = (pixel)X265_MIN(255, X265_MAX(0, value));
        }
    }
}

// hotspot8
__arm_locally_streaming
void intraPredAng8_sme2_tbl(pixel *dst, intptr_t stride, const pixel *src, int mode, int filter)
{
    svbool_t all = svptrue_b16();
    svuint8_t refs = svld1_u8(svptrue_b8(), src);
    svbool_t eight = svptrue_pat_b16(SV_VL8);
    for (int row = 0; row < 8; row += 2)
    {
        svuint8_t pairs = svtbl_u8(refs, svld1_u8(svptrue_b8(), pairIndex[mode]+16*row));
        svuint8_t weights = svld1_u8(svptrue_b8(), pairWeight[mode]+16*row);
        svuint16_t values = svmlalt_u16(svmullb_u16(pairs, weights), pairs, weights);
        values = svlsr_n_u16_x(all, svadd_n_u16_x(all, values, 16), 5);
        svst1b_u16(eight, dst+row*stride, values);
        svst1b_u16(eight, dst+(row+1)*stride, svext_u16(values, values, 8));
    }

    if (mode == 2) dst[7*stride+7] = src[32];
    if (filter && (mode == 10 || mode == 26))
    {
        const bool horizontal = mode == 10;
        int top = src[horizontal ? 17 : 1], corner = src[0];
        for (int i = 0; i < 8; ++i)
        {
            int value = top + ((src[(horizontal ? 1 : 17)+i] - corner) >> 1);
            dst[horizontal ? i : i*stride] = (pixel)X265_MIN(255, X265_MAX(0, value));
        }
    }
}
void setupIntraPred8Primitives_sme2(EncoderPrimitives &)
{
    const char *s = getenv("X265_SME2_INTRA_PRED8");
    const char *v = getenv("X265_SME2_INTRA_PRED8_VARIANT");
    intra_pred_t candidate = v && v[0] == '1' ? intraPredAng8_sme2_tbl2 : intraPredAng8_sme2_tbl;
    intraPred8Sme2 = s && s[0] == '1' && !s[1] && sme2Available() ? candidate : 0;
}
}
