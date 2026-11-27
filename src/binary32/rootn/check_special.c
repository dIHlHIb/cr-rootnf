/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 dIHlHIb
   Developed with AI assistance. */
/* MPFR is linked only into this test. Input lines: hex binary32, signed degree. */
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <fenv.h>
#include <inttypes.h>
#include <math.h>
#include <mpfr.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
extern float cr_rootnf(float, long long);

static uint32_t word(float x) { uint32_t u; memcpy(&u, &x, 4); return u; }
static float value(uint32_t u) { float x; memcpy(&x, &u, 4); return x; }
static const int fm[] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
static const mpfr_rnd_t rm[] = {MPFR_RNDN, MPFR_RNDD, MPFR_RNDU, MPFR_RNDZ};
static uint64_t calls, errors, exceptions;

static void verify(uint32_t x, long long n, uint32_t want, int flags, int err, int mode) {
    const int sticky = (calls & 1) ? FE_DIVBYZERO : 0;
    fesetround(mode);
    feclearexcept(FE_ALL_EXCEPT);
    if (sticky) feraiseexcept(sticky);
    errno = 907;
    uint32_t got = word(cr_rootnf(value(x), n));
    int goterr = errno;
    int gotflags = fetestexcept(FE_ALL_EXCEPT);
    int gotmode = fegetround();
    int nan = (want & 0x7fffffff) > 0x7f800000;
    int value_ok = nan ? (got & 0x7fc00000u) == 0x7fc00000u : got == want;
    if (!value_ok || gotflags != (sticky | flags) || gotmode != mode || goterr != (err ? err : 907)) {
        if (errors++ < 12)
            printf("x=%08x n=%lld mode=%d got=%08x/%x/%d want=%08x/%x/%d\n",
                   x,n,mode,got,gotflags,goterr,want,sticky|flags,err?err:907);
    }
    calls++;
}

static int read_case(int wc, uint32_t *u, long long *n) {
    char line[512];
    while (fgets(line, sizeof line, stdin)) {
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (!*p || *p=='#') continue;
        int end = -1;
        if (wc) {
            float f;
            if (sscanf(p," %a , %lld %n",&f,n,&end) != 2) return -1;
            *u=word(f);
        } else if (sscanf(p,"%" SCNx32 " %lld %n",u,n,&end)!=2) return -1;
        if (end<0 || (p[end] && p[end]!='#')) return -1;
        if ((*u & 0x7fffffff) >= 0x7f800000u || !(*u & 0x7fffffff) ||
            !*n || ((*u>>31) && !(*n & 1))) return -1;
        return 1;
    }
    return ferror(stdin) ? -1 : 0;
}

int main(int argc, char **argv) {
    _Static_assert(LONG_MAX == LLONG_MAX, "MPFR's signed degree must cover long long");
    int wc = argc==2 && !strcmp(argv[1],"--worst");
    if (argc!=1 && !wc) return 2;
    struct { uint32_t x; long long n; uint32_t result; int flags, err; } special[] = {
        {0x7fc01234,0,0x7fc00000,FE_INVALID,EDOM}, {0x7fa00001,0,0x7fc00000,FE_INVALID,EDOM},
        {0x3f800000,0,0x7fc00000,FE_INVALID,EDOM}, {0x80000000,0,0x7fc00000,FE_INVALID,EDOM},
        {0x7fc01234,3,0x7fc00000,0,0}, {0xffa00001,-1,0x7fc00000,FE_INVALID,0},
        {0x7fa00001,2,0x7fc00000,FE_INVALID,0}, {0xffc01234,2,0x7fc00000,0,0},
        {0xff800000,3,0xff800000,0,0}, {0xff800000,-3,0x80000000,0,0},
        {0xff800000,2,0x7fc00000,FE_INVALID,EDOM}, {0x7f800000,-2,0,0,0},
        {0x7f800000,3,0x7f800000,0,0}, {0,3,0,0,0}, {0x80000000,2,0,0,0},
        {0x80000000,3,0x80000000,0,0}, {0,-7,0x7f800000,FE_DIVBYZERO,ERANGE},
        {0x80000000,-2,0x7f800000,FE_DIVBYZERO,ERANGE}, {0x80000000,-3,0xff800000,FE_DIVBYZERO,ERANGE},
        {0xc1100000,-4,0x7fc00000,FE_INVALID,EDOM}, {0xbf800000,2,0x7fc00000,FE_INVALID,EDOM},
        {0xbf800000,4294967296LL,0x7fc00000,FE_INVALID,EDOM},
        {0x80000001,-4294967296LL,0x7fc00000,FE_INVALID,EDOM},
        {0xff800000,-4294967296LL,0x7fc00000,FE_INVALID,EDOM},
        {0x7f800000,-4294967296LL,0,0,0},
        {0x80000000,4294967296LL,0,0,0},
        {0x80000000,-4294967296LL,0x7f800000,FE_DIVBYZERO,ERANGE},
        {0x7fc00001,4294967296LL,0x7fc00000,0,0},
        {0xffa00001,-4294967296LL,0x7fc00000,FE_INVALID,0}
    };
    for (size_t i = 0; i < sizeof(special)/sizeof(*special); i++)
        for (int j=0;j<4;j++) verify(special[i].x,special[i].n,special[i].result,
                                     special[i].flags,special[i].err,fm[j]);
    exceptions = calls;
    mpfr_t x,lo,hi;
    mpfr_inits2(256, x, lo, hi, (mpfr_ptr)0);
    uint32_t u;
    long long n;
    int read;
    while ((read=read_case(wc,&u,&n)) > 0) {
        mpfr_set_flt(x,value(u),MPFR_RNDN);
        mpfr_rootn_si(lo,x,n,MPFR_RNDD);
        mpfr_rootn_si(hi,x,n,MPFR_RNDU);
        for (int j=0;j<4;j++) {
            float v = mpfr_get_flt(lo,rm[j]);
            if (word(v) != word(mpfr_get_flt(hi,rm[j]))) return 3;
            int flags = (mpfr_equal_p(lo,hi) && mpfr_cmp_d(lo,(double)v)==0) ? 0 : FE_INEXACT;
            int err = 0;
            if (n == -1) {
                uint32_t a = u & 0x7fffffff;
                if (a <= 0x00200000u) { err=ERANGE; flags |= FE_OVERFLOW; }
                if (a > 0x7e800000u && a != 0x7f000000u) {
                    err=ERANGE;
                    /* The platform's reciprocal instruction supplies tininess semantics. */
                    fesetround(fm[j]); feclearexcept(FE_ALL_EXCEPT);
                    volatile float denominator = value(u);
                    volatile float reciprocal = 1.0f / denominator;
                    (void)reciprocal;
                    flags |= fetestexcept(FE_UNDERFLOW);
                }
            }
            verify(u,n,word(v),flags,err,fm[j]);
        }
    }
    printf("finite pairs=%" PRIu64 " special calls=%" PRIu64 " total calls=%" PRIu64
           " value/flag/errno/mode mismatches=%" PRIu64 "\n",(calls-exceptions)/4,exceptions,calls,errors);
    mpfr_clears(x,lo,hi,(mpfr_ptr)0);
    return read < 0 || calls == exceptions || errors != 0;
}
