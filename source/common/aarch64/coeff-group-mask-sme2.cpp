#include "common.h"
#include "coeff-group-mask-sme2.h"
#include <arm_sme.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(__linux__)
#include <sys/auxv.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#endif

namespace X265_NS {
namespace {
uint64_t histogram[64] = {};
const char *profilePath = 0;

void saveHistogram()
{
    FILE *f = fopen(profilePath, "w");
    if (!f)
    {
        perror("coefficient-group profile");
        return;
    }
    fprintf(f, "lastScanSet,calls\n");
    for (int i = 0; i < 64; ++i)
        fprintf(f, "%d,%llu\n", i, (unsigned long long)histogram[i]);
    fclose(f);
}

bool hasSME2()
{
#if defined(__linux__)
    const unsigned long required = (1UL << 23) | (1UL << 37);
    return (getauxval(AT_HWCAP2) & required) == required;
#elif defined(__APPLE__)
    int supported = 0;
    size_t size = sizeof(supported);
    return sysctlbyname("hw.optional.arm.FEAT_SME2", &supported, &size, 0, 0) == 0 && supported;
#else
    return false;
#endif
}
}

bool coeffGroupMaskSme2Enabled()
{
    static const bool enabled = []() {
        const char *setting = getenv("X265_SME2_COEFF_GROUP_MASK");
        return setting && setting[0] == '1' && setting[1] == '\0' && sme2Available();
    }();
    return enabled;
}

bool sme2Available()
{
    static const bool available = hasSME2() && svcntsb() == 32;
    return available;
}

void recordCoeffGroupMaskCalls(int lastScanSet)
{
    static const bool recording = []() {
        profilePath = getenv("X265_SME2_COEFF_GROUP_MASK_PROFILE");
        if (!profilePath || !profilePath[0])
            return false;
        atexit(saveHistogram);
        return true;
    }();
    if (recording && lastScanSet >= 0 && lastScanSet < 64)
        __atomic_fetch_add(histogram + lastScanSet, uint64_t(1), __ATOMIC_RELAXED);
}

// hotspot1
__arm_locally_streaming
uint64_t coeffGroupMask_sme2_u32(const uint16_t *scanCG,
                            const uint8_t *coeffNum, int lastScanSet)
{
    svuint32_t low = svdup_u32(0), high = svdup_u32(0);
    for (uint64_t i = 0; i < (uint64_t)lastScanSet; i += svcntw())
    {
        svbool_t pg = svwhilelt_b32(i, (uint64_t)lastScanSet);
        svuint32_t positions = svld1uh_u32(pg, scanCG + i);
        svbool_t nonzero = svcmpne_n_u32(pg, svld1ub_u32(pg, coeffNum + i), 0);
        svuint32_t ones = svsel_u32(nonzero, svdup_u32(1), svdup_u32(0));
        low = svorr_u32_x(svptrue_b32(), low, svlsl_u32_x(svptrue_b32(), ones, positions));
        high = svorr_u32_x(svptrue_b32(), high,
                          svlsl_u32_x(svptrue_b32(), ones, svsub_n_u32_x(svptrue_b32(), positions, 32)));
    }
    return uint64_t(svorv_u32(svptrue_b32(), low)) |
           (uint64_t(svorv_u32(svptrue_b32(), high)) << 32);
}

// hotspot1
__arm_locally_streaming
uint64_t coeffGroupMask_sme2_u64(const uint16_t *scanCG, const uint8_t *coeffNum, int lastScanSet)
{
    svuint64_t mask = svdup_u64(0);
    for (uint64_t i = 0; i < (uint64_t)lastScanSet; i += svcntd())
    {
        svbool_t pg = svwhilelt_b64(i, (uint64_t)lastScanSet);
        svuint64_t positions = svld1uh_u64(pg, scanCG+i);
        svbool_t nonzero = svcmpne_n_u64(pg, svld1ub_u64(pg, coeffNum+i), 0);
        svuint64_t ones = svsel_u64(nonzero, svdup_u64(1), svdup_u64(0));
        mask = svorr_u64_x(svptrue_b64(), mask, svlsl_u64_x(svptrue_b64(), ones, positions));
    }
    return svorv_u64(svptrue_b64(), mask);
}

bool useCoeffGroupMaskU64()
{
    static const bool use = []() {
        const char *v = getenv("X265_SME2_COEFF_GROUP_MASK_VARIANT");
        return !(v && v[0] == '1' && !v[1]);
    }();
    return use;
}

}
