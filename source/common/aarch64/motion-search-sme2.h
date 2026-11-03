#ifndef X265_MOTION_SEARCH_SME2_H
#define X265_MOTION_SEARCH_SME2_H
namespace X265_NS {
bool motionSearchSme2Enabled();
int selectSquareCandidates_sme2(const int *costs, int y, int ymin, int ymax, int best);
}
#endif
