/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 dIHlHIb
   Developed with AI assistance. */
/* Compare a binary32 magnitude's integer-degree root with a 25-bit dyadic
   boundary. Each interval multiplication rounds outward using integer limbs.
   On success, *side is -1, 0 or +1 for the positive root relative to g. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t *word;  /* least significant limb first, leading bit is one */
    int64_t exponent;
} root_endpoint;

static void root_set(root_endpoint *a, size_t k, uint64_t bits) {
    uint64_t significand = (bits << 11) | (UINT64_C(1) << 63);
    memset(a->word, 0, k * sizeof(uint32_t));
    a->word[k-1] = (uint32_t)(significand >> 32);
    if (k > 1) a->word[k-2] = (uint32_t)significand;
    a->exponent = (int64_t)(bits >> 52) - 1023;
}

static int root_compare(const root_endpoint *a, const root_endpoint *b, size_t k) {
    if (a->exponent != b->exponent) return a->exponent < b->exponent ? -1 : 1;
    for (size_t i = k; i-- > 0;)
        if (a->word[i] != b->word[i]) return a->word[i] < b->word[i] ? -1 : 1;
    return 0;
}

static void root_multiply(root_endpoint *a, const root_endpoint *b, size_t k,
                          int upward, uint32_t *product) {
    memset(product, 0, 2*k*sizeof(*product));
    for (size_t i = 0; i < k; i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < k; j++) {
            uint64_t t = (uint64_t)a->word[i]*b->word[j] + product[i+j] + carry;
            product[i+j] = (uint32_t)t;
            carry = t >> 32;
        }
        product[i+k] = (uint32_t)carry;
    }
    unsigned shift = product[2*k-1] >> 31;
    a->exponent += b->exponent + shift;
    uint32_t discarded = shift ? product[k-1] : product[k-1] & UINT32_C(0x7fffffff);
    for (size_t i = 0; i+1 < k; i++) discarded |= product[i];
    for (size_t i = 0; i < k; i++)
        a->word[i] = shift ? product[k+i] : (product[k+i] << 1) | (product[k+i-1] >> 31);
    if (upward && discarded) {
        for (size_t i = 0; i < k; i++)
            if (++a->word[i] != 0) return;
        a->word[k-1] = UINT32_C(1) << 31;
        a->exponent++;
    }
}

static int root_at_precision(uint64_t x, int64_t n, uint64_t grid, size_t k,
                             uint32_t *work, int *side) {
    root_endpoint lo = {work, 0}, hi = {work+k, 0};
    root_endpoint blo = {work+2*k, 0}, bhi = {work+3*k, 0}, ref = {work+4*k, 0};
    uint32_t *product = work+5*k;
    uint64_t one = UINT64_C(0x3ff0000000000000);
    root_set(&lo, k, n < 0 ? x : one);
    root_set(&hi, k, n < 0 ? x : one);
    root_set(&blo, k, grid);
    root_set(&bhi, k, grid);
    root_set(&ref, k, n < 0 ? one : x);
    uint64_t power = (uint64_t)(n < 0 ? -n : n); /* caller has |n| < 2^32 */
    while (power) {
        if (power & 1) {
            root_multiply(&lo, &blo, k, 0, product);
            root_multiply(&hi, &bhi, k, 1, product);
        }
        power >>= 1;
        if (power) {
            root_multiply(&blo, &blo, k, 0, product);
            root_multiply(&bhi, &bhi, k, 1, product);
        }
    }
    int lower = root_compare(&ref, &hi, k), upper = root_compare(&ref, &lo, k);
    if (lower > 0) *side = 1;
    else if (upper < 0) *side = -1;
    else if (!lower && !upper) *side = 0;
    else return 0;
    return 1;
}

#ifndef ROOTNF_INITIAL_LIMBS
#define ROOTNF_INITIAL_LIMBS 4
#endif

static int root_grid_side(uint64_t x, int64_t n, uint64_t grid, int *side) {
    size_t k = ROOTNF_INITIAL_LIMBS;
    uint32_t local[7 * ROOTNF_INITIAL_LIMBS];
    if (root_at_precision(x, n, grid, k, local, side)) return 1;
    for (;;) {
        if (k > SIZE_MAX / (14*sizeof(uint32_t))) return 0;
        k *= 2;
        uint32_t *w = malloc(7*k*sizeof(*w));
        if (!w) return 0;
        int done = root_at_precision(x, n, grid, k, w, side);
        free(w);
        if (done) return 1;
    }
}
