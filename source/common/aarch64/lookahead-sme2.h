#ifndef X265_LOOKAHEAD_SME2_H
#define X265_LOOKAHEAD_SME2_H
namespace X265_NS {
bool lookaheadSme2Enabled();
int bidirSatd8x8_sme2(const pixel *fenc, intptr_t fs, const pixel *a, intptr_t as, const pixel *b, intptr_t bs);
}
#endif
