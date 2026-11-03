#include "common.h"
#include "sme2.h"
#include <arm_sme.h>
#if defined(__linux__)
#include <sys/auxv.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#endif

namespace X265_NS {
namespace {
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

bool sme2Available()
{
    static const bool available = hasSME2() && svcntsb() == 32;
    return available;
}
}
