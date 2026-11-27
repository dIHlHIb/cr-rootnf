/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 dIHlHIb
   Developed with AI assistance. */
#include <errno.h>
#include <fenv.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
extern float rootnf(float, long long);
extern float cr_rootnf(float, long long);
static const int modes[] = {FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
#ifdef ONLY_CANDIDATE
#define FIRST_LIB 1
#else
#define FIRST_LIB 0
#endif
int main(void) {
    unsigned x,expected,required,allowed,policy,mode,rows=0;
    unsigned mismatches[2][4]={{0}}, flagsbad[2]={0}, errbad[2]={0};
    long long n;
    while (scanf("%x %lld %u %x %u %u %u",&x,&n,&mode,&expected,&required,&allowed,&policy)==7) {
        float magnitude;
        memcpy(&magnitude,&x,4);
        for (int i=FIRST_LIB;i<2;i++) {
            fesetround(modes[mode]); feclearexcept(FE_ALL_EXCEPT);
            errno=0;
#ifdef ONLY_CANDIDATE
            float answer=cr_rootnf(magnitude,n);
#else
            float answer=i?cr_rootnf(magnitude,n):rootnf(magnitude,n);
#endif
            int err=errno, f=fetestexcept(FE_ALL_EXCEPT);
            uint32_t observed; memcpy(&observed,&answer,4);
            unsigned mapped=!!(f&FE_INEXACT) | ((unsigned)!!(f&FE_UNDERFLOW)<<1) |
                ((unsigned)!!(f&FE_OVERFLOW)<<2) | ((unsigned)!!(f&(FE_INVALID|FE_DIVBYZERO))<<3);
            if ((mapped & required)!=required || (mapped & ~allowed)) flagsbad[i]++;
            if ((policy==0 && err) || (policy==1 && err && err!=ERANGE) || (policy==2 && err!=ERANGE))
                errbad[i]++;
            if (observed!=expected) {
                if (mismatches[i][mode]++==0)
                    printf("first fn=%d mode=%u x=%08x n=%lld got=%08x published=%08x\n",i,mode,x,n,observed,expected);
            }
        }
        rows++;
    }
    for (int i=FIRST_LIB;i<2;i++)
        printf("fn=%s rows=%u bit mismatches RN/RD/RU/RZ=%u/%u/%u/%u flag mismatches=%u errno mismatches=%u\n",
               i?"cr_rootnf":"glibc", rows,mismatches[i][0],mismatches[i][1],mismatches[i][2],mismatches[i][3],flagsbad[i],errbad[i]);
    return rows!=824 || mismatches[1][0] || mismatches[1][1] || mismatches[1][2] || mismatches[1][3] || flagsbad[1] || errbad[1];
}
