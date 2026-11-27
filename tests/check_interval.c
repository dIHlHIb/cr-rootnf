/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 dIHlHIb
   Developed with AI assistance. */
/* Test the integer certificate directly, including low-precision refinement. */
#include <inttypes.h>
#include <math.h>
#include <mpfr.h>
#include <stdio.h>
#include "power_interval.h"
static uint64_t word(double x) { uint64_t u; memcpy(&u,&x,8); return u; }
static double val(uint64_t u) { double x; memcpy(&x,&u,8); return x; }
int main(void) {
    mpfr_t x,lo,hi,g;
    mpfr_inits2(384,x,lo,hi,(mpfr_ptr)0);
    mpfr_init2(g,25);
    uint64_t grids=0,uncertain32=0,uncertain64=0,uncertain128=0,bad=0;
    unsigned ux; long long n;
    while (scanf("%x %lld",&ux,&n)==2) {
        if (n<=-0x100000000LL || n>=0x100000000LL || (n>=-1 && n<=1)) continue;
        ux &= 0x7fffffff;
        float xf; memcpy(&xf,&ux,4); double xd=(double)xf;
        mpfr_set_d(x,xd,MPFR_RNDN);
        mpfr_rootn_si(lo,x,n,MPFR_RNDD); mpfr_rootn_si(hi,x,n,MPFR_RNDU);
        mpfr_set(g,lo,MPFR_RNDN);
        uint64_t grid=word(mpfr_get_d(g,MPFR_RNDN));
        for (int d=-1;d<=1;d++) {
            uint64_t gu=grid+(uint64_t)d*(UINT64_C(1)<<28);
            double gd=val(gu);
            int cmp_lo=mpfr_cmp_d(lo,gd), cmp_hi=mpfr_cmp_d(hi,gd);
            int expected=cmp_lo>0 ? 1 : cmp_hi<0 ? -1 : 0;
            if (!expected && (cmp_lo || cmp_hi)) return 3;
            uint32_t scratch[7*4];
            int side=0, scratch_side=0;
            int k1=root_at_precision(word(xd),n,gu,1,scratch,&scratch_side);
            if (!k1) uncertain32++; else if (scratch_side!=expected) bad++;
            int k2=root_at_precision(word(xd),n,gu,2,scratch,&scratch_side);
            if (!k2) uncertain64++; else if (scratch_side!=expected) bad++;
            int k4=root_at_precision(word(xd),n,gu,4,scratch,&scratch_side);
            if (!k4) uncertain128++; else if (scratch_side!=expected) bad++;
            int ok=root_grid_side(word(xd),n,gu,&side); /* compiled to start with 32 bits */
            if (!ok || side!=expected) bad++;
            grids++;
        }
    }
    printf("boundaries=%" PRIu64 " undecided at 32/64/128 bits=%" PRIu64 "/%" PRIu64 "/%" PRIu64
           " wrong certificates or final answers=%" PRIu64 "\n",grids,uncertain32,uncertain64,uncertain128,bad);
    mpfr_clears(x,lo,hi,g,(mpfr_ptr)0);
    return !grids || !uncertain32 || uncertain128 != 0 || bad != 0;
}
