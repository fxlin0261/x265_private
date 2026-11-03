#include "encoder-subpaths-sme2.h"
#include "common.h"
#include "primitives.h"
#include "contexts.h"
#include "coeff-group-mask-sme2.h"
#include "sme2-profile.h"
#include <arm_sme.h>
#include <stdlib.h>
namespace X265_NS {
namespace {
bool enabledOption(const char *name)
{
    const char *s=getenv(name);
    return s && s[0]=='1' && !s[1] && sme2Available();
}
int variantOption(const char *name,int fallback)
{
    const char *s=getenv(name);
    return s?atoi(s):fallback;
}
}
bool referenceCopySme2Enabled()
{
    static const bool enabled=enabledOption("X265_SME2_REFERENCE_COPY");
    return enabled;
}
bool coefficientParitySme2Enabled()
{
    static const bool enabled=enabledOption("X265_SME2_COEFFICIENT_PARITY");
    return enabled;
}
bool chromaCopySseSme2Enabled()
{
    static const bool enabled=enabledOption("X265_SME2_CHROMA_COPY_SSE");
    return enabled;
}
bool entropyEstimateSme2Enabled()
{
    static const bool enabled=enabledOption("X265_SME2_ENTROPY_ESTIMATE");
    return enabled;
}
bool useReferenceCopyUnroll()
{
    static const bool enabled=variantOption("X265_SME2_REFERENCE_COPY_VARIANT",1)==2;
    return enabled;
}
int coefficientParityVariant()
{
    static const int enabled=variantOption("X265_SME2_COEFFICIENT_PARITY_VARIANT",3);
    return enabled;
}
int chromaCopySseVariant()
{
    static const int enabled=variantOption("X265_SME2_CHROMA_COPY_SSE_VARIANT",4);
    return enabled;
}
bool useEntropyEstimatePacked()
{
    static const bool enabled=variantOption("X265_SME2_ENTROPY_ESTIMATE_VARIANT",2)!=1;
    return enabled;
}
// hotspot13
__arm_locally_streaming
void referenceCopy_sme2(pixel *dst,const pixel *src,unsigned n)
{
    for(unsigned i=0;i<n;i+=32)
    {
        svbool_t pg=svwhilelt_b8(i,n);
        svst1_u8(pg,dst+i,svld1_u8(pg,src+i));
    }
}
// hotspot13
__arm_locally_streaming
void referenceCopy_sme2_unroll(pixel *dst,const pixel *src,unsigned n)
{
    for(unsigned i=0;i<n;i+=64)
    {
        svbool_t pg=svwhilelt_b8(i,n);
        svst1_u8(pg,dst+i,svld1_u8(pg,src+i));
        pg=svwhilelt_b8(i+32,n);
        svst1_u8(pg,dst+i+32,svld1_u8(pg,src+i+32));
    }
}
// hotspot15
__arm_locally_streaming
uint32_t coefficientParity_sme2(const int16_t *c,unsigned stride)
{
    svbool_t pg=svwhilelt_b16((uint64_t)0,(uint64_t)4);
    svint16_t a=svld1_s16(pg,c),b=svld1_s16(pg,c+stride);
    svint16_t d=svld1_s16(pg,c+2*stride),e=svld1_s16(pg,c+3*stride);
    return (uint32_t)svaddv_s16(pg,svadd_s16_x(pg,svadd_s16_x(pg,a,b),svadd_s16_x(pg,d,e)))&1;
}
// hotspot15
__arm_locally_streaming
uint32_t coefficientParity_sme2_wide(const int16_t *c,unsigned stride)
{
    svbool_t pg=svwhilelt_b32((uint64_t)0,(uint64_t)4);
    svint32_t a=svld1sh_s32(pg,c),b=svld1sh_s32(pg,c+stride);
    svint32_t d=svld1sh_s32(pg,c+2*stride),e=svld1sh_s32(pg,c+3*stride);
    return (uint32_t)svaddv_s32(pg,svadd_s32_x(pg,svadd_s32_x(pg,a,b),svadd_s32_x(pg,d,e)))&1;
}
// hotspot17
__arm_locally_streaming
sse_t chromaCopySse_sme2(pixel *dst,intptr_t ds,const pixel *pred,intptr_t ps,const pixel *src,intptr_t ss,unsigned n)
{
    svuint32_t sum=svdup_u32(0);
    for(unsigned y=0;y<n;++y)
        for(unsigned x=0;x<n;x+=16)
        {
            svbool_t pg=svwhilelt_b16(x,n);
            svuint16_t p=svld1ub_u16(pg,pred+y*ps+x),s=svld1ub_u16(pg,src+y*ss+x);
            svst1b_u16(pg,dst+y*ds+x,p);
            svuint16_t d=svabd_u16_z(pg,p,s);
            sum=svmlalb_u32(sum,d,d);
            sum=svmlalt_u32(sum,d,d);
        }
    return (sse_t)svaddv_u32(svptrue_b32(),sum);
}
// hotspot17
__arm_locally_streaming
sse_t chromaCopySse_sme2_wide(pixel *dst,intptr_t ds,const pixel *pred,intptr_t ps,const pixel *src,intptr_t ss,unsigned n)
{
    svuint32_t sum=svdup_u32(0);
    for(unsigned y=0;y<n;++y)
        for(unsigned x=0;x<n;x+=8)
        {
            svbool_t pg=svwhilelt_b32(x,n);
            svuint32_t p=svld1ub_u32(pg,pred+y*ps+x),s=svld1ub_u32(pg,src+y*ss+x);
            svst1b_u32(pg,dst+y*ds+x,p);
            svuint32_t d=svabd_u32_z(pg,p,s);
            sum=svmla_u32_x(svptrue_b32(),sum,d,d);
        }
    return (sse_t)svaddv_u32(svptrue_b32(),sum);
}
template<unsigned N>
__attribute__((always_inline)) static inline
sse_t copySseDot(pixel *dst,intptr_t ds,const pixel *pred,intptr_t ps,const pixel *src,intptr_t ss) __arm_streaming
{
    svbool_t pg=svwhilelt_b8((uint64_t)0,(uint64_t)N);
    svuint32_t sum=svdup_u32(0);
    for(unsigned y=0;y<N;++y)
    {
        svuint8_t p=svld1_u8(pg,pred+y*ps),s=svld1_u8(pg,src+y*ss);
        svst1_u8(pg,dst+y*ds,p);
        svuint8_t d=svabd_u8_x(svptrue_b8(),p,s);
        sum=svdot_u32(sum,d,d);
    }
    return svaddv_u32(svptrue_b32(),sum);
}
// hotspot17
__arm_locally_streaming
sse_t chromaCopySse_sme2_dot(pixel *dst,intptr_t ds,const pixel *pred,intptr_t ps,const pixel *src,intptr_t ss,unsigned n)
{
    switch(n)
    {
        case 4: return copySseDot<4>(dst,ds,pred,ps,src,ss);
        case 8: return copySseDot<8>(dst,ds,pred,ps,src,ss);
        case 16: return copySseDot<16>(dst,ds,pred,ps,src,ss);
        default: return copySseDot<32>(dst,ds,pred,ps,src,ss);
    }
}
// hotspot19
__arm_locally_streaming
uint32_t entropyEstimate_sme2(uint32_t bin,uint8_t &ctx)
{
    uint32_t state=ctx;
    svuint32_t bits=svld1_u32(svptrue_b32(),g_entropyBits+(state&~7u));
    svuint8_t next=svld1_u8(svptrue_b8(),&g_nextState[state&~15u][0]);
    ctx=(uint8_t)svlastb_u8(svptrue_b8(),svtbl_u8(next,svdup_u8(2*(state&15)+bin)));
    return svlastb_u32(svptrue_b32(),svtbl_u32(bits,svdup_u32((state&7)^bin)));
}
// hotspot19
__arm_locally_streaming
uint32_t entropyEstimate_sme2_packed(uint32_t bin,uint8_t &ctx)
{
    uint32_t state=ctx;
    svuint32_t bits=svdup_u32(g_entropyBits[state^bin]);
    ctx=g_nextState[state][bin];
    return svlastb_u32(svptrue_b32(),bits);
}
// hotspot15
__attribute__((naked))
uint32_t coefficientParity_sme2_asm(const int16_t *,unsigned)
{
    __asm__ volatile(
        "sub sp, sp, #64\n"
        "stp d8, d9, [sp, #0]\n"
        "stp d10, d11, [sp, #16]\n"
        "stp d12, d13, [sp, #32]\n"
        "stp d14, d15, [sp, #48]\n"
        "smstart\tsm\n"
        "lsl\tw8, w1, #1\n"
        "ptrue\tp0.h, vl4\n"
        "mov\tw9, w1\n"
        "add\tw10, w8, w1\n"
        "ld1h\t{ z0.h }, p0/z, [x0]\n"
        "ld1h\t{ z1.h }, p0/z, [x0, x9, lsl #1]\n"
        "ld1h\t{ z2.h }, p0/z, [x0, x8, lsl #1]\n"
        "ld1h\t{ z3.h }, p0/z, [x0, x10, lsl #1]\n"
        "add\tz0.h, z0.h, z1.h\n"
        "add\tz1.h, z2.h, z3.h\n"
        "add\tz0.h, z0.h, z1.h\n"
        "saddv\td0, p0, z0.h\n"
        "fmov\tw8, s0\n"
        "smstop\tsm\n"
        "ldp d8, d9, [sp, #0]\n"
        "ldp d10, d11, [sp, #16]\n"
        "ldp d12, d13, [sp, #32]\n"
        "ldp d14, d15, [sp, #48]\n"
        "add sp, sp, #64\n"
        "ret\n"
    );
}

// hotspot17
__attribute__((naked))
sse_t chromaCopySse_sme2_asm(pixel *,intptr_t,const pixel *,intptr_t,const pixel *,intptr_t,unsigned)
{
    __asm__ volatile(
        "sub sp, sp, #96\n"
        "stp x19, x20, [sp, #0]\n"
        "stp x21, x22, [sp, #16]\n"
        "stp d8, d9, [sp, #32]\n"
        "stp d10, d11, [sp, #48]\n"
        "stp d12, d13, [sp, #64]\n"
        "stp d14, d15, [sp, #80]\n"
        "smstart\tsm\n"
        "lsl\tx8, x3, #1\n"
        "lsl\tx9, x5, #1\n"
        "lsl\tx17, x1, #1\n"
        "add\tx16, x2, x3\n"
        "add\tx15, x4, x5\n"
        "add\tx14, x0, x1\n"
        "add\tx13, x2, x8\n"
        "add\tx12, x4, x9\n"
        "add\tx11, x0, x17\n"
        "cmp\tw6, #0x10\n"
        "add\tx10, x16, x8\n"
        "add\tx9, x15, x9\n"
        "add\tx8, x14, x17\n"
        "b.eq\t.LchromaCopySse_sme2_asm_6d45e0\n"
        "cmp\tw6, #0x8\n"
        "b.eq\t.LchromaCopySse_sme2_asm_6d44ec\n"
        "cmp\tw6, #0x4\n"
        "b.ne\t.LchromaCopySse_sme2_asm_6d4808\n"
        "ptrue\tp0.b, vl4\n"
        "ptrue\tp1.b\n"
        "ld1b\t{ z0.b }, p0/z, [x2]\n"
        "ld1b\t{ z1.b }, p0/z, [x4]\n"
        "st1b\t{ z0.b }, p0, [x0]\n"
        "uabd\tz1.b, p1/m, z1.b, z0.b\n"
        "mov\tz0.s, #0x0\n"
        "ld1b\t{ z2.b }, p0/z, [x16]\n"
        "ld1b\t{ z3.b }, p0/z, [x15]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x14]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x13]\n"
        "ld1b\t{ z3.b }, p0/z, [x12]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x11]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x10]\n"
        "ld1b\t{ z3.b }, p0/z, [x9]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "st1b\t{ z2.b }, p0, [x8]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "b\t.LchromaCopySse_sme2_asm_6d4c64\n"
        ".LchromaCopySse_sme2_asm_6d44ec:\n"
        "ptrue\tp0.b, vl8\n"
        "mov\tz0.s, #0x0\n"
        "ptrue\tp1.b\n"
        "ld1b\t{ z1.b }, p0/z, [x2]\n"
        "ld1b\t{ z2.b }, p0/z, [x4]\n"
        "st1b\t{ z1.b }, p0, [x0]\n"
        "uabd\tz1.b, p1/m, z1.b, z2.b\n"
        "ld1b\t{ z2.b }, p0/z, [x16]\n"
        "ld1b\t{ z3.b }, p0/z, [x15]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x14]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x13]\n"
        "ld1b\t{ z3.b }, p0/z, [x12]\n"
        "lsl\tx12, x1, #2\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x11]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "mov\tw11, #0x6\n"
        "ld1b\t{ z2.b }, p0/z, [x10]\n"
        "ld1b\t{ z3.b }, p0/z, [x9]\n"
        "lsl\tx9, x3, #2\n"
        "lsl\tx10, x5, #2\n"
        "mul\tx13, x5, x11\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x8]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "mul\tx8, x3, x11\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x9]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x10]\n"
        "add\tx9, x9, x3\n"
        "add\tx10, x10, x5\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x12]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x9]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x10]\n"
        "mul\tx9, x1, x11\n"
        "add\tx10, x12, x1\n"
        "sub\tx11, x4, x5\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x10]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "lsl\tx10, x5, #3\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x8]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x13]\n"
        "lsl\tx8, x3, #3\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x9]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "sub\tx9, x2, x3\n"
        "ld1b\t{ z2.b }, p0/z, [x9, x8]\n"
        "ld1b\t{ z3.b }, p0/z, [x11, x10]\n"
        "lsl\tx8, x1, #3\n"
        "b\t.LchromaCopySse_sme2_asm_6d4c4c\n"
        ".LchromaCopySse_sme2_asm_6d45e0:\n"
        "ptrue\tp0.b, vl16\n"
        "mov\tz0.s, #0x0\n"
        "ptrue\tp1.b\n"
        "ld1b\t{ z1.b }, p0/z, [x2]\n"
        "ld1b\t{ z2.b }, p0/z, [x4]\n"
        "st1b\t{ z1.b }, p0, [x0]\n"
        "uabd\tz1.b, p1/m, z1.b, z2.b\n"
        "ld1b\t{ z2.b }, p0/z, [x16]\n"
        "ld1b\t{ z3.b }, p0/z, [x15]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x14]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "lsl\tx14, x1, #3\n"
        "ld1b\t{ z2.b }, p0/z, [x13]\n"
        "ld1b\t{ z3.b }, p0/z, [x12]\n"
        "lsl\tx12, x5, #2\n"
        "lsl\tx13, x1, #2\n"
        "sub\tx16, x14, x1\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x11]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x10]\n"
        "ld1b\t{ z3.b }, p0/z, [x9]\n"
        "lsl\tx9, x3, #2\n"
        "mov\tw10, #0x6\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "mul\tx11, x3, x10\n"
        "st1b\t{ z2.b }, p0, [x8]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x9]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x12]\n"
        "mul\tx8, x5, x10\n"
        "add\tx9, x9, x3\n"
        "add\tx12, x12, x5\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "mul\tx10, x1, x10\n"
        "st1b\t{ z2.b }, p0, [x0, x13]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "add\tx13, x13, x1\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x9]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x12]\n"
        "mov\tw9, #0xa\n"
        "mul\tx12, x3, x9\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x13]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "mul\tx13, x5, x9\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x11]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x8]\n"
        "lsl\tx11, x3, #3\n"
        "lsl\tx8, x5, #3\n"
        "mul\tx9, x1, x9\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "sub\tx15, x11, x3\n"
        "st1b\t{ z2.b }, p0, [x0, x10]\n"
        "sub\tx10, x8, x5\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x15]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x10]\n"
        "mov\tw15, #0xb\n"
        "mul\tx10, x3, x15\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x16]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "mul\tx16, x5, x15\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x11]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x8]\n"
        "add\tx11, x11, x3\n"
        "add\tx8, x8, x5\n"
        "mul\tx15, x1, x15\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x14]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "add\tx14, x14, x1\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x11]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x8]\n"
        "mov\tw11, #0xc\n"
        "mul\tx8, x3, x11\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x14]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "mul\tx14, x5, x11\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x12]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x13]\n"
        "mov\tw12, #0xd\n"
        "mul\tx11, x1, x11\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x9]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "mul\tx9, x3, x12\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x10]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x16]\n"
        "mul\tx10, x5, x12\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x15]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "mul\tx12, x1, x12\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x8]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x14]\n"
        "mov\tw8, #0xe\n"
        "mul\tx13, x3, x8\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x11]\n"
        "mul\tx11, x5, x8\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x9]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x10]\n"
        "lsl\tx9, x3, #4\n"
        "mul\tx8, x1, x8\n"
        "lsl\tx10, x5, #4\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x12]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x13]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x11]\n"
        "sub\tx11, x4, x5\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x8]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "sub\tx8, x2, x3\n"
        "ld1b\t{ z2.b }, p0/z, [x8, x9]\n"
        "ld1b\t{ z3.b }, p0/z, [x11, x10]\n"
        "lsl\tx8, x1, #4\n"
        "b\t.LchromaCopySse_sme2_asm_6d4c4c\n"
        ".LchromaCopySse_sme2_asm_6d4808:\n"
        "mov\tw17, #0x20\n"
        "ptrue\tp1.b\n"
        "mov\tz0.s, #0x0\n"
        "whilelo\tp0.b, xzr, x17\n"
        "lsl\tx18, x1, #2\n"
        "mov\tw17, #0x6\n"
        "lsl\tx7, x3, #3\n"
        "lsl\tx20, x5, #3\n"
        "lsl\tx21, x1, #3\n"
        "ld1b\t{ z1.b }, p0/z, [x2]\n"
        "ld1b\t{ z2.b }, p0/z, [x4]\n"
        "mov\tw6, #0xb\n"
        "sub\tx19, x21, x1\n"
        "mov\tw22, #0xe\n"
        "st1b\t{ z1.b }, p0, [x0]\n"
        "uabd\tz1.b, p1/m, z1.b, z2.b\n"
        "ld1b\t{ z3.b }, p0/z, [x16]\n"
        "ld1b\t{ z2.b }, p0/z, [x15]\n"
        "mul\tx16, x3, x17\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "mul\tx15, x1, x17\n"
        "st1b\t{ z3.b }, p0, [x14]\n"
        "uabd\tz2.b, p1/m, z2.b, z3.b\n"
        "ld1b\t{ z1.b }, p0/z, [x13]\n"
        "ld1b\t{ z3.b }, p0/z, [x12]\n"
        "lsl\tx12, x3, #2\n"
        "lsl\tx13, x5, #2\n"
        "mul\tx14, x5, x17\n"
        "udot\tz0.s, z2.b, z2.b\n"
        "st1b\t{ z1.b }, p0, [x11]\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "mov\tw11, #0xa\n"
        "ld1b\t{ z2.b }, p0/z, [x10]\n"
        "ld1b\t{ z3.b }, p0/z, [x9]\n"
        "mul\tx17, x3, x11\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "mul\tx9, x5, x11\n"
        "st1b\t{ z2.b }, p0, [x8]\n"
        "uabd\tz2.b, p1/m, z2.b, z3.b\n"
        "ld1b\t{ z1.b }, p0/z, [x2, x12]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x13]\n"
        "mul\tx8, x1, x11\n"
        "add\tx11, x12, x3\n"
        "add\tx12, x13, x5\n"
        "add\tx13, x18, x1\n"
        "udot\tz0.s, z2.b, z2.b\n"
        "mul\tx10, x3, x6\n"
        "st1b\t{ z1.b }, p0, [x0, x18]\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "mov\tw18, #0xc\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x11]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x12]\n"
        "mul\tx11, x5, x6\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "mul\tx12, x1, x6\n"
        "st1b\t{ z2.b }, p0, [x0, x13]\n"
        "uabd\tz2.b, p1/m, z2.b, z3.b\n"
        "ld1b\t{ z1.b }, p0/z, [x2, x16]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x14]\n"
        "sub\tx16, x7, x3\n"
        "mul\tx13, x3, x18\n"
        "udot\tz0.s, z2.b, z2.b\n"
        "st1b\t{ z1.b }, p0, [x0, x15]\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "mul\tx14, x5, x18\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x16]\n"
        "sub\tx16, x20, x5\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x16]\n"
        "mul\tx15, x1, x18\n"
        "mov\tw18, #0xd\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x19]\n"
        "mul\tx6, x3, x18\n"
        "ld1b\t{ z1.b }, p0/z, [x2, x7]\n"
        "uabd\tz2.b, p1/m, z2.b, z3.b\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x20]\n"
        "add\tx7, x7, x3\n"
        "mul\tx16, x5, x18\n"
        "st1b\t{ z1.b }, p0, [x0, x21]\n"
        "udot\tz0.s, z2.b, z2.b\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x7]\n"
        "add\tx7, x20, x5\n"
        "add\tx20, x21, x1\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x7]\n"
        "mul\tx18, x1, x18\n"
        "lsl\tx21, x1, #4\n"
        "st1b\t{ z2.b }, p0, [x0, x20]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "mul\tx19, x3, x22\n"
        "ld1b\t{ z1.b }, p0/z, [x2, x17]\n"
        "uabd\tz2.b, p1/m, z2.b, z3.b\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x9]\n"
        "mul\tx7, x5, x22\n"
        "mov\tw20, #0x12\n"
        "st1b\t{ z1.b }, p0, [x0, x8]\n"
        "udot\tz0.s, z2.b, z2.b\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x10]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x11]\n"
        "mul\tx17, x1, x22\n"
        "mov\tw22, #0x16\n"
        "mul\tx9, x3, x20\n"
        "st1b\t{ z2.b }, p0, [x0, x12]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "uabd\tz2.b, p1/m, z2.b, z3.b\n"
        "ld1b\t{ z1.b }, p0/z, [x2, x13]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x14]\n"
        "mul\tx8, x5, x20\n"
        "mul\tx10, x1, x20\n"
        "mov\tw20, #0x13\n"
        "st1b\t{ z1.b }, p0, [x0, x15]\n"
        "udot\tz0.s, z2.b, z2.b\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x6]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x16]\n"
        "mul\tx12, x3, x20\n"
        "mov\tw6, #0x14\n"
        "mul\tx11, x5, x20\n"
        "st1b\t{ z2.b }, p0, [x0, x18]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "uabd\tz2.b, p1/m, z2.b, z3.b\n"
        "ld1b\t{ z1.b }, p0/z, [x2, x19]\n"
        "lsl\tx19, x3, #4\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x7]\n"
        "mul\tx13, x1, x20\n"
        "lsl\tx20, x5, #4\n"
        "sub\tx7, x21, x1\n"
        "sub\tx18, x19, x3\n"
        "st1b\t{ z1.b }, p0, [x0, x17]\n"
        "udot\tz0.s, z2.b, z2.b\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x18]\n"
        "sub\tx18, x20, x5\n"
        "mul\tx14, x3, x6\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x18]\n"
        "mul\tx15, x5, x6\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x7]\n"
        "uabd\tz2.b, p1/m, z2.b, z3.b\n"
        "ld1b\t{ z1.b }, p0/z, [x2, x19]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x20]\n"
        "add\tx19, x19, x3\n"
        "add\tx20, x20, x5\n"
        "mul\tx16, x1, x6\n"
        "mov\tw6, #0x15\n"
        "udot\tz0.s, z2.b, z2.b\n"
        "st1b\t{ z1.b }, p0, [x0, x21]\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x19]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x20]\n"
        "add\tx21, x21, x1\n"
        "mul\tx17, x3, x6\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x21]\n"
        "uabd\tz2.b, p1/m, z2.b, z3.b\n"
        "ld1b\t{ z1.b }, p0/z, [x2, x9]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x8]\n"
        "mul\tx18, x5, x6\n"
        "mov\tw21, #0x17\n"
        "mul\tx6, x1, x6\n"
        "udot\tz0.s, z2.b, z2.b\n"
        "st1b\t{ z1.b }, p0, [x0, x10]\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x12]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x11]\n"
        "mul\tx7, x3, x22\n"
        "mov\tw12, #0x18\n"
        "mul\tx19, x5, x22\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x13]\n"
        "uabd\tz2.b, p1/m, z2.b, z3.b\n"
        "ld1b\t{ z1.b }, p0/z, [x2, x14]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x15]\n"
        "mul\tx20, x1, x22\n"
        "mov\tw14, #0x19\n"
        "mul\tx9, x3, x21\n"
        "udot\tz0.s, z2.b, z2.b\n"
        "st1b\t{ z1.b }, p0, [x0, x16]\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x17]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x18]\n"
        "mul\tx8, x5, x21\n"
        "mov\tw17, #0x1a\n"
        "mul\tx10, x1, x21\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x6]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x7]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x19]\n"
        "mul\tx11, x3, x12\n"
        "mul\tx13, x5, x12\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x20]\n"
        "uabd\tz2.b, p1/m, z2.b, z3.b\n"
        "ld1b\t{ z1.b }, p0/z, [x2, x9]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x8]\n"
        "mul\tx12, x1, x12\n"
        "mul\tx15, x3, x14\n"
        "udot\tz0.s, z2.b, z2.b\n"
        "st1b\t{ z1.b }, p0, [x0, x10]\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "mul\tx16, x5, x14\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x11]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x13]\n"
        "mov\tw10, #0x1b\n"
        "mul\tx14, x1, x14\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x12]\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "mul\tx18, x3, x17\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x15]\n"
        "mov\tw12, #0x1c\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x16]\n"
        "mul\tx9, x5, x17\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x14]\n"
        "mul\tx8, x1, x17\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x18]\n"
        "mul\tx11, x3, x10\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x9]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "mul\tx13, x5, x10\n"
        "st1b\t{ z2.b }, p0, [x0, x8]\n"
        "mul\tx10, x1, x10\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x11]\n"
        "mul\tx15, x3, x12\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x13]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "mul\tx14, x5, x12\n"
        "st1b\t{ z2.b }, p0, [x0, x10]\n"
        "mul\tx9, x1, x12\n"
        "mov\tw12, #0x1d\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x15]\n"
        "mul\tx8, x3, x12\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x14]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "mul\tx11, x5, x12\n"
        "st1b\t{ z2.b }, p0, [x0, x9]\n"
        "mul\tx10, x1, x12\n"
        "mov\tw12, #0x1e\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x8]\n"
        "mul\tx13, x3, x12\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x11]\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "sub\tx11, x4, x5\n"
        "mul\tx9, x5, x12\n"
        "st1b\t{ z2.b }, p0, [x0, x10]\n"
        "lsl\tx10, x5, #5\n"
        "mul\tx8, x1, x12\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x2, x13]\n"
        "ld1b\t{ z3.b }, p0/z, [x4, x9]\n"
        "lsl\tx9, x3, #5\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "st1b\t{ z2.b }, p0, [x0, x8]\n"
        "sub\tx8, x2, x3\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "ld1b\t{ z2.b }, p0/z, [x8, x9]\n"
        "ld1b\t{ z3.b }, p0/z, [x11, x10]\n"
        "lsl\tx8, x1, #5\n"
        ".LchromaCopySse_sme2_asm_6d4c4c:\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "movprfx\tz1, z2\n"
        "uabd\tz1.b, p1/m, z1.b, z3.b\n"
        "udot\tz0.s, z1.b, z1.b\n"
        "sub\tx9, x0, x1\n"
        "st1b\t{ z2.b }, p0, [x9, x8]\n"
        ".LchromaCopySse_sme2_asm_6d4c64:\n"
        "ptrue\tp0.s\n"
        "uaddv\td0, p0, z0.s\n"
        "fmov\tw0, s0\n"
        "smstop\tsm\n"
        "ldp x19, x20, [sp, #0]\n"
        "ldp x21, x22, [sp, #16]\n"
        "ldp d8, d9, [sp, #32]\n"
        "ldp d10, d11, [sp, #48]\n"
        "ldp d12, d13, [sp, #64]\n"
        "ldp d14, d15, [sp, #80]\n"
        "add sp, sp, #96\n"
        "ret\n"
    );
}

}
