#include "common.h"
#include "coeff-group-mask-sme2.h"
#include "motion-search-sme2.h"
#include <arm_sme.h>
#include <stdlib.h>

namespace X265_NS {
bool motionSearchSme2Enabled()
{
    static const bool enabled = []() {
        const char *s = getenv("X265_SME2_MOTION_SEARCH");
        return s && s[0] == '1' && !s[1] && sme2Available();
    }();
    return enabled;
}

// hotspot4
__arm_locally_streaming
int selectSquareCandidates_sme2(const int *costs, int y, int ymin, int ymax, int best)
{
    static const int32_t dy[8] = {-1, 1, 0, 0, -1, 1, -1, 1};
    svbool_t all = svptrue_b32();
    svint32_t ys = svadd_n_s32_x(all, svld1_s32(all, dy), y);
    svbool_t valid = svorr_b_z(all, svcmpeq_n_s32(all, svld1_s32(all, dy), 0),
                             svand_b_z(all, svcmpge_n_s32(all, ys, ymin), svcmple_n_s32(all, ys, ymax)));
    svint32_t encoded = svadd_s32_x(all, svlsl_n_s32_x(all, svmin_n_s32_x(all, svld1_s32(all, costs), best), 4), svindex_s32(1, 1));
    encoded = svsel_s32(valid, encoded, svdup_s32(0x7fffffff));
    return X265_MIN(svminv_s32(all, encoded), best << 4);
}
}
