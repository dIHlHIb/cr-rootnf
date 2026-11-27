/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 dIHlHIb
   Developed with AI assistance. */
/* Directed enclosures for both reported minima over the supplied candidate list. */
#include <math.h>
#include <mpfr.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    mpfr_t lo,hi;
    long n;
    unsigned word;
} minimum;

static void init(minimum *m) {
    mpfr_inits2(256,m->lo,m->hi,(mpfr_ptr)0);
    mpfr_set_inf(m->lo,1);
    mpfr_set_inf(m->hi,1);
}

static int update(minimum *m,mpfr_ptr l,mpfr_ptr h,long n,unsigned x) {
    if (mpfr_sgn(h)<0) {
        mpfr_swap(l,h);
        mpfr_neg(l,l,MPFR_RNDN);
        mpfr_neg(h,h,MPFR_RNDN);
    }
    if (mpfr_sgn(l)<=0 || mpfr_cmp(l,h)>0) return 0;
    if (mpfr_cmp(l,m->hi)>0) return 1;
    if (mpfr_cmp(h,m->lo)>=0) return 0;  /* Cannot order these two intervals. */
    mpfr_set(m->lo,l,MPFR_RNDN);
    mpfr_set(m->hi,h,MPFR_RNDN);
    m->n=n; m->word=x;
    return 1;
}

static void show(minimum *m,const char *metric) {
    mpfr_log2(m->lo,m->lo,MPFR_RNDD);
    mpfr_log2(m->hi,m->hi,MPFR_RNDU);
    mpfr_printf("%s xbits=%08x n=%ld log2(min) in [%.12RDf, %.12RUf]\n",
                metric,m->word,m->n,m->lo,m->hi);
}

int main(void) {
    mpfr_t x,g,l,h,blo,bhi;
    mpfr_inits2(256,x,g,l,h,blo,bhi,(mpfr_ptr)0);
    minimum residual,relative;
    init(&residual);init(&relative);
    long n;
    unsigned u,i,rows=0;
    int e,fields;
    while ((fields=scanf("%ld %x %d %u",&n,&u,&e,&i))==4) {
        float xf; memcpy(&xf,&u,4);
        mpfr_set_d(x,xf,MPFR_RNDN);
        mpfr_set_d(g,ldexp(1.0+ldexp((double)i,-24),e),MPFR_RNDN);
        mpfr_pow_si(blo,g,n,MPFR_RNDD);
        mpfr_pow_si(bhi,g,n,MPFR_RNDU);
        mpfr_div(l,x,bhi,MPFR_RNDD);mpfr_div(h,x,blo,MPFR_RNDU);
        mpfr_log2(l,l,MPFR_RNDD);mpfr_log2(h,h,MPFR_RNDU);
        if (!update(&residual,l,h,n,u)) return 2;
        mpfr_rootn_si(l,x,n,MPFR_RNDD);mpfr_rootn_si(h,x,n,MPFR_RNDU);
        mpfr_div(l,l,g,MPFR_RNDD);mpfr_div(h,h,g,MPFR_RNDU);
        mpfr_sub_ui(l,l,1,MPFR_RNDD);mpfr_sub_ui(h,h,1,MPFR_RNDU);
        if (!update(&relative,l,h,n,u)) return 3;
        rows++;
    }
    if (fields!=EOF || !rows || mpfr_cmp_ui_2exp(residual.lo,1,-65)<=0) return 4;
    printf("nonexact candidate grids=%u; each |log2(x/g^n)| > 2^-65\n",rows);
    show(&residual,"log residual");show(&relative,"relative root distance");
    mpfr_clears(x,g,l,h,blo,bhi,residual.lo,residual.hi,relative.lo,relative.hi,(mpfr_ptr)0);
}
