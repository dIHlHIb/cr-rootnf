/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 dIHlHIb
   Developed with AI assistance. */
/* Bound the error in the scanner's double-double log table with directed MPFR. */
#include <fenv.h>
#include <mpfr.h>
#include <stdio.h>
#include <stdlib.h>
#include "ddlib.h"

int main(void) {
    if (fesetround(FE_TONEAREST)) return 2;
    mpfr_t arg,low,high,actual,worst,err;
    mpfr_inits2(192,arg,low,high,actual,worst,err,(mpfr_ptr)0);
    mpfr_set_zero(worst,0);
    unsigned worst_i=0;
    double previous=-1;
    for (unsigned i=0;i<(1u<<24);i++) {
        double x=1+ldexp((double)i,-24), h,l;
        dd_log2(x,&h,&l);
        if (!isfinite(h) || !isfinite(l) || !(h>previous) || fabs(h)>1 || fabs(l)>0x1p-52) return 3;
        previous=h;
        mpfr_set_d(arg,x,MPFR_RNDN);
        /* A single directed log and its next 192-bit neighbour enclose the truth. */
        mpfr_log2(low,arg,MPFR_RNDD);
        mpfr_set(high,low,MPFR_RNDN);
        if (i) mpfr_nextabove(high);
        /* h+l must be represented exactly for the comparison below. */
        mpfr_set_d(actual,h,MPFR_RNDN);
        if (mpfr_add_d(actual,actual,l,MPFR_RNDN)) return 4;
        mpfr_sub(err,high,actual,MPFR_RNDU);
        mpfr_sub(low,actual,low,MPFR_RNDU);
        if (mpfr_cmp(low,err)>0) mpfr_set(err,low,MPFR_RNDN);
        if (mpfr_cmp(err,worst)>0) {mpfr_set(worst,err,MPFR_RNDN);worst_i=i;}
    }
    mpfr_printf("entries=%u strictly increasing high words; max |low word| <= 2^-52\n"
                "maximum certified absolute error <= %.12RUA at i=%u, required < 2^-105\n",
                1u<<24,worst,worst_i);
    int status=mpfr_cmp_ui_2exp(worst,1,-105)>=0;
    mpfr_clears(arg,low,high,actual,worst,err,(mpfr_ptr)0);
    return status;
}
