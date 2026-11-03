#include "common.h"
#include "sme2-profile.h"
#include <stdio.h>
#include <stdlib.h>
namespace X265_NS {
namespace {
uint64_t calls[21][65][256] = {};
const char *path = 0;
void save()
{
    FILE *f = fopen(path, "w");
    if (!f) { perror("SME2 profile"); return; }
    fprintf(f, "operator,a,b,calls\n");
    for (int h=0; h<21; ++h)
        for (int a=0; a<65; ++a)
            for (int b=0; b<256; ++b)
                if (calls[h][a][b]) fprintf(f, "%d,%d,%d,%llu\n", h,a,b,(unsigned long long)calls[h][a][b]);
    fclose(f);
}
}
void recordSme2Calls(int id, int a, int b)
{
    static const bool active = []() {
        path = getenv("X265_SME2_PROFILE");
        if (!path || !*path) return false;
        atexit(save);
        return true;
    }();
    if (active && unsigned(id)<21 && unsigned(a)<65 && unsigned(b)<256)
        __atomic_fetch_add(&calls[id][a][b], uint64_t(1), __ATOMIC_RELAXED);
}
}
