#include "common.h"
#include "primitives.h"
#include "sme2.h"
#include <arm_sme.h>
#include <stdlib.h>

namespace X265_NS {
namespace {
#include "dct32-sme2-tables.h"
#include "dct-multi-sme2.h"
}

// hotspot9
__arm_new("za") __arm_locally_streaming
void dct32_sme2_multi_unrolled(const int16_t *src, int16_t *dst, intptr_t stride)
{
    int16_t packed[4][16][16], next[4][16][16];
    packDctRowsMulti<32, 2>(src, stride, packed);
    transformDctMulti<32, 4, true, 2>(packed, &next[0][0][0], packedDct32);
    transformDctMulti<32, 11, false, 2>(next, dst, packedDct32);
}

void setupDct32Primitives_sme2(EncoderPrimitives &p)
{
    const char *s = getenv("X265_SME2_DCT32");
    if (s && s[0] == '1' && !s[1] && sme2Available())
        p.cu[BLOCK_32x32].dct = dct32_sme2_multi_unrolled;
}
}
