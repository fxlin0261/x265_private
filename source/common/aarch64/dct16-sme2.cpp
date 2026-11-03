#include "common.h"
#include "primitives.h"
#include "sme2.h"
#include <arm_sme.h>
#include <stdlib.h>

namespace X265_NS {
namespace {
#include "dct16-sme2-tables.h"
#include "dct-multi-sme2.h"
}

// hotspot16
__arm_new("za") __arm_locally_streaming
void dct16_sme2_multi_unrolled(const int16_t *src, int16_t *dst, intptr_t stride)
{
    int16_t packed[2][8][16], next[2][8][16];
    packDctRowsMulti<16, 8>(src, stride, packed);
    transformDctMulti<16, 3, true, 8>(packed, &next[0][0][0], packedDct16);
    transformDctMulti<16, 10, false, 8>(next, dst, packedDct16);
}

void setupDct16Primitives_sme2(EncoderPrimitives &p)
{
    const char *s = getenv("X265_SME2_DCT16");
    if (s && s[0] == '1' && !s[1] && sme2Available())
        p.cu[BLOCK_16x16].dct = dct16_sme2_multi_unrolled;
}
}
