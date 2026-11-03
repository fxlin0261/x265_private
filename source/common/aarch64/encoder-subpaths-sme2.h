#ifndef X265_ENCODER_SUBPATHS_SME2_H
#define X265_ENCODER_SUBPATHS_SME2_H
#include "common.h"
namespace X265_NS {
bool referenceCopySme2Enabled();
bool useReferenceCopyUnroll();
int coefficientParityVariant();
int chromaCopySseVariant();
bool useEntropyEstimatePacked();
void referenceCopy_sme2(pixel*,const pixel*,unsigned);
void referenceCopy_sme2_unroll(pixel*,const pixel*,unsigned);
bool coefficientParitySme2Enabled();
uint32_t coefficientParity_sme2_asm(const int16_t*,unsigned);
uint32_t coefficientParity_sme2(const int16_t*,unsigned);
uint32_t coefficientParity_sme2_wide(const int16_t*,unsigned);
bool chromaCopySseSme2Enabled();
sse_t chromaCopySse_sme2_asm(pixel*,intptr_t,const pixel*,intptr_t,const pixel*,intptr_t,unsigned);
sse_t chromaCopySse_sme2(pixel*,intptr_t,const pixel*,intptr_t,const pixel*,intptr_t,unsigned);
sse_t chromaCopySse_sme2_dot(pixel*,intptr_t,const pixel*,intptr_t,const pixel*,intptr_t,unsigned);
sse_t chromaCopySse_sme2_wide(pixel*,intptr_t,const pixel*,intptr_t,const pixel*,intptr_t,unsigned);
bool entropyEstimateSme2Enabled();
uint32_t entropyEstimate_sme2(uint32_t,uint8_t&);
uint32_t entropyEstimate_sme2_packed(uint32_t,uint8_t&);
}
#endif
