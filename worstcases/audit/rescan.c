/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 dIHlHIb
   Developed with AI assistance. */
/* Independent checks, using direct powers, exact cubes and a fixed-point
   recurrence. No lookup table or filter from the original search is used. */
#include <inttypes.h>
#include <math.h>
#include <mpfr.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef __uint128_t U;
static const uint32_t m24 = 1u<<24;
static mpfr_t grid,lo,hi,left,right,tmp,pad,scaled;
static mpz_t whole;
static uint64_t visits,hits;
static void require(int ok, const char *s) { if (!ok) {fprintf(stderr,"%s\n",s); exit(2);} }
static int bits(U v) {
    uint64_t h=(uint64_t)(v>>64);
    return h ? 128-__builtin_clzll(h) : 64-__builtin_clzll((uint64_t)v);
}
static uint32_t fword(float x) {uint32_t u; memcpy(&u,&x,4); return u;}

static void power(double g, long n) {
    mpfr_set_d(grid,g,MPFR_RNDN);
    mpfr_pow_si(lo,grid,n,MPFR_RNDD);
    mpfr_pow_si(hi,grid,n,MPFR_RNDU);
}
/* On entry lo <= g^n <= hi. Bound both sides of the strict log threshold. */
static void decide(double g,long n,float xf) {
    mpfr_d_div(left,(double)xf,hi,MPFR_RNDD);
    mpfr_d_div(right,(double)xf,lo,MPFR_RNDU);
    mpfr_log2(left,left,MPFR_RNDD); mpfr_log2(right,right,MPFR_RNDU);
    const double t=0x1p-50;
    if (mpfr_cmp_d(left,t)>=0 || mpfr_cmp_d(right,-t)<=0) return;
    require(mpfr_cmp_d(left,-t)>0 && mpfr_cmp_d(right,t)<0, "ambiguous threshold");
    int e;
    double frac=frexp(g,&e);
    uint32_t i=(uint32_t)ldexp(2*frac-1,24);
    printf("%ld %08x %d %u\n",n,fword(xf),e-1,i);
    hits++;
}

/* Test each distinct 25-bit mantissa exactly once. Scaling g by 2^e
   scales its cube by 2^(3e), so the relative proximity test is invariant. */
static void cubes(void) {
    for (uint32_t m=m24;m<2*m24;m++) {
        U z=(U)m*m*m;
        int shift=bits(z)-24;
        U j=(z+((U)1<<(shift-1)))>>shift;
        U rounded=j<<shift, diff=z>rounded ? z-rounded : rounded-z;
        visits++;
        if (diff>(z>>49)+1) continue;
        for (int e=-51;e<=43;e++) {
            double x=ldexp((double)(uint32_t)j,shift+3*(e-24));
            float xf=(float)x;
            if (!(xf>0) || !isfinite(xf) || (double)xf!=x) continue;
            double g=ldexp((double)m,e-24);
            if (g==1) continue;
            power(g,3); decide(g,3,xf);
        }
    }
}

static void test_float(double g,long n,float xf) {
    if (!(xf>0) || !isfinite(xf)) return;
    mpfr_div_2ui(pad,hi,49,MPFR_RNDU);
    mpfr_sub_d(tmp,lo,(double)xf,MPFR_RNDD);
    if (mpfr_cmp(tmp,pad)>0) return;
    mpfr_d_sub(tmp,(double)xf,hi,MPFR_RNDD);
    if (mpfr_cmp(tmp,pad)>0) return;
    decide(g,n,xf);
}

static void single_degree(long n) {
    long an=labs(n);
    require(an>=1024,"single-degree grid requires |n| >= 1024");
    for (int above=0;above<2;above++) {
        int scale=above ? 24 : 25;
        long bound=(((long)256<<scale)+an-1)/an;
        double end=1+ldexp((above?1.0:-1.0)*(bound+1),-scale);
        power(end,n);
        if (above==(n>0)) require(mpfr_cmp_ui_2exp(lo,1,129)>0,"upper bound");
        else require(mpfr_cmp_ui_2exp(hi,1,-150)<0,"lower bound");
        for (long k=1;k<=bound;k++) {
            double g=1+ldexp((above?1.0:-1.0)*k,-scale);
            power(g,n);
            float a=mpfr_get_flt(lo,MPFR_RNDN), b=mpfr_get_flt(hi,MPFR_RNDN);
            test_float(g,n,a);
            if (a!=b) test_float(g,n,b);
            visits++;
        }
    }
}

static U endpoint(mpfr_srcptr x,int s,mpfr_rnd_t dir) {
    mpfr_mul_2si(scaled,x,-s,dir); mpfr_get_z(whole,scaled,dir);
    require(mpz_sizeinbase(whole,2)<=96,"fixed-point endpoint overflow");
    uint64_t low=mpz_get_ui(whole);
    mpz_fdiv_q_2exp(whole,whole,64);
    return ((U)mpz_get_ui(whole)<<64)|low;
}

/* Across this real 2^27-degree shard, monotonicity leaves a single
   non-unit boundary. Carry an interval for its power at each degree. */
static void high_shard(void) {
    const long first=2818572288L,last=2952790015L,block=8192;
    const double g=1-0x1p-25;
    power(1-0x1p-24,first); require(mpfr_cmp_ui_2exp(hi,1,-150)<0,"lower grid still viable");
    power(1+0x1p-24,first); require(mpfr_cmp_ui_2exp(lo,1,129)>0,"upper grid still viable");
    for (long base=first;base<=last;base+=block) {
        power(g,base);
        int s=(int)mpfr_get_exp(hi)-95;
        U l=endpoint(lo,s,MPFR_RNDD), h=endpoint(hi,s,MPFR_RNDU);
        for (long n=base;n<base+block && n<=last;n++) {
            U widen=(h>>49)+1;
            U start=l-widen,stop=h+widen;
            int spacing=bits(start)-1-23;
            if (spacing < -149-s) spacing=-149-s;
            require(spacing>=0 && spacing<120,"invalid lattice scale");
            U step=(U)1<<spacing;
            U a=(start+step-1)>>spacing, b=stop>>spacing;
            for (U ix=a;ix<=b;ix++) {
                double xd=ldexp((double)(uint64_t)ix,s+spacing);
                float xf=(float)xd;
                if (xf>0 && isfinite(xf) && (double)xf==xd) {
                    power(g,n); decide(g,n,xf);
                }
            }
            l-=(l+(((U)1<<25)-1))>>25;
            h-=h>>25;
            visits++;
        }
    }
}

int main(int argc,char **argv) {
    require(sizeof(long)==8 && argc==2,"expected LP64 and one job");
    mpfr_inits2(192,grid,lo,hi,left,right,tmp,pad,scaled,(mpfr_ptr)0);
    mpz_init(whole);
    if (!strcmp(argv[1],"cubes")) cubes();
    else if (!strcmp(argv[1],"shard")) high_shard();
    else {char *end;long n=strtol(argv[1],&end,10);require(!*end,"invalid degree");single_degree(n);}
    fprintf(stderr,"%s visits=%" PRIu64 " hits=%" PRIu64 "\n",argv[1],visits,hits);
    mpz_clear(whole);mpfr_clears(grid,lo,hi,left,right,tmp,pad,scaled,(mpfr_ptr)0);
}
