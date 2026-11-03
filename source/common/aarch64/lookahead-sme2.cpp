#include "common.h"
#include "coeff-group-mask-sme2.h"
#include "lookahead-sme2.h"
#include <arm_sme.h>
#include <stdlib.h>
namespace X265_NS {
bool lookaheadSme2Enabled()
{
    static const bool enabled = []() {
        const char *s = getenv("X265_SME2_LOOKAHEAD");
        return s && s[0] == '1' && !s[1] && sme2Available();
    }();
    return enabled;
}
static inline svint16_t loadAverageDifference(const pixel *f, intptr_t fs,
    const pixel *a, intptr_t as, const pixel *b, intptr_t bs, int y) __arm_streaming
{
    svbool_t eight = svptrue_pat_b16(SV_VL8);
    svuint16_t avg0 = svrhadd_u16_x(eight, svld1ub_u16(eight, a+y*as), svld1ub_u16(eight, b+y*bs));
    svuint16_t avg1 = svrhadd_u16_x(eight, svld1ub_u16(eight, a+(y+4)*as), svld1ub_u16(eight, b+(y+4)*bs));
    svint16_t top = svreinterpret_s16_u16(svsub_u16_x(eight, svld1ub_u16(eight, f+y*fs), avg0));
    svint16_t bottom = svreinterpret_s16_u16(svsub_u16_x(eight, svld1ub_u16(eight, f+(y+4)*fs), avg1));
    return svsplice_s16(eight, top, bottom);
}
static inline svint16_t finishPair(svint16_t a, svint16_t b) __arm_streaming
{
    svbool_t all = svptrue_b16();
    svint16_t even = svuzp1_s16(a, b), odd = svuzp2_s16(a, b);
    svint16_t sum = svadd_s16_x(all, even, odd), diff = svsub_s16_x(all, even, odd);
    return svmax_s16_x(all, svabs_s16_x(all, svuzp1_s16(sum, diff)),
                           svabs_s16_x(all, svuzp2_s16(sum, diff)));
}
// hotspot7
__arm_locally_streaming
int bidirSatd8x8_sme2(const pixel *fenc, intptr_t fs, const pixel *a, intptr_t as, const pixel *b, intptr_t bs)
{
    svbool_t all = svptrue_b16();
    svint16_t a0 = loadAverageDifference(fenc, fs, a, as, b, bs, 0);
    svint16_t a1 = loadAverageDifference(fenc, fs, a, as, b, bs, 1);
    svint16_t a2 = loadAverageDifference(fenc, fs, a, as, b, bs, 2);
    svint16_t a3 = loadAverageDifference(fenc, fs, a, as, b, bs, 3);
    svint16_t s0 = svadd_s16_x(all, a0, a1), d0 = svsub_s16_x(all, a0, a1);
    svint16_t s1 = svadd_s16_x(all, a2, a3), d1 = svsub_s16_x(all, a2, a3);
    svint16_t h0 = finishPair(svadd_s16_x(all, s0, s1), svsub_s16_x(all, s0, s1));
    svint16_t h1 = finishPair(svadd_s16_x(all, d0, d1), svsub_s16_x(all, d0, d1));
    return (int)svaddv_s16(all, svadd_s16_x(all, h0, h1));
}
}
