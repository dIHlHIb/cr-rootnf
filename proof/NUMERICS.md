# Why the slow comparison works

Let X be the positive magnitude of a finite binary32 input and let n be its signed integer degree. The general path handles 2 ≤ |n| < 2³². Its result is normal in binary32. Exact special values, reciprocals, square roots, and degrees outside this range have separate branches.

## Exact results and flags

Write X as M·2ᵃ with M an odd positive integer below 2²⁴. If a positive dyadic y = R·2ᵇ is an exact root for n > 0, then Rⁿ = M and nb = a. Either R = 1 or R ≥ 3. In the latter case n ≤ 15, since 3¹⁶ exceeds 2²⁴. For n < 0, R = M = 1 is necessary. The program tests these conditions with integers before any approximating operation can raise the inexact flag. A generator computes the entire odd-power map for degrees 3 through 15. Degree 2 uses square root. The verifier generates perfect powers independently, reconstructing binary32 inputs directly from their odd significands and exponents.

Revy gives related conditions for exact representability in Properties 3.4–3.5. Property 3.6 rules out an exact midpoint when |n| ≥ 2 by raising an odd midpoint significand to an integer power. The finite count and test data for binary32 are given in the search report. [Revy, Chapter 3](https://perso.univ-perp.fr/guillaume.revy/publications/files/pdf/Revy09.pdf) · [Search report](../worstcases/SCAN.md)

## From an approximation to an exact comparison

The fast approximation has a conservative relative-error bound of 2⁻⁴² in all four IEEE rounding directions. The guard excludes 8,192 binary64 steps on either side of each binary32 value or midpoint in the binade. The separation between consecutive such boundaries is 2²⁸ steps. Within the guard the approximate value identifies a unique candidate boundary g. The large-degree and near-one shortcuts use the exact side of 1 and a strict bound on the distance from 1. [Fast-path derivation, steps 1–6](fast-path.md)

For positive n, compare X with gⁿ. For negative n, compare 1 with X·g^|n|. In both cases the sign of the first operand minus the second gives the side of g on which the positive root lies. The lower and upper products are computed by squaring and multiplying with outward truncation. If the exact reference exceeds the upper endpoint, the root lies above g. If it is below the lower endpoint, the root lies below g. Equal coincident endpoints establish equality. Otherwise redo the computation at twice the precision. The preceding or following binary64 value around g is in the desired open binary32 rounding cell. Applying the sign and converting in the caller's dynamic mode selects the returned float.

The C multiplication forms a full product of p-bit significands using base-2³² limbs. Each inner sum is bounded by (2³²−1)² + 2(2³²−1) = 2⁶⁴−1. The leading product bit selects one of two normalization shifts. A nonzero discarded suffix increments the upper significand, renormalizing once on carry. The discarded suffix has no effect on the lower significand. The default first attempt has four limbs per endpoint and needs no allocation.

## Termination and the 128-bit bound

The odd part of g has at most 25 bits. Every power produced by squaring has exponent at most |n| and the running product uses at most |n| copies of g. The negative-degree path includes one extra 24-bit input significand. Therefore precision p ≥ 25|n| + 24 suffices to retain every product exactly. This loose termination bound applies even when the starting precision is reduced for testing.

For a p-bit normalized multiplication each endpoint has relative truncation error below u = 2^(1−p). Expanding a shared square counts its error twice. For an initially exact base and accumulator the total number of multiplicative error factors is at most |n|. At p = 128 the possible displacement in log base 2 is less than 2⁻⁹³ for |n| < 2³², using −log₂(1−u) ≤ 2u/ln(2). The 138-shard midpoint-corrected replay reproduced all 2,855 inexact candidates and 278,559 exact cases. Directed MPFR evaluation and the filter bound give a strict lower bound of 2⁻⁶⁵ for every nonzero log gap under the search assumptions. Thus the default comparison resolves in its first 128-bit attempt. The implementation retains precision doubling as a fallback. Under the search assumptions no input reaches it. If it were reached and memory allocation failed, the function would return NaN and, with `CORE_MATH_SUPPORT_ERRNO`, set `errno` to `ENOMEM`. The replay assumes round-to-nearest binary64 and the specified operations, including FMA. Its normal initial index must be representable in int64_t and its subnormal initial index must be defined. The numerical search and its assumptions are documented in [the scan report](../worstcases/SCAN.md). The separately tested strict scanner guards those conversions.

The interval tests also start the comparison at 32 bits, where some inputs require a retry. Under the same fast-path and arithmetic assumptions, a missing search entry would affect the guarantee about the first 128-bit attempt. A comparison that remained undecided would retry at higher precision; any strict decision still comes from an enclosing interval.

## The inclusive switch at 2³²

Put K = 2³². Both X and its reciprocal lie in [2⁻¹⁴⁹, 2¹⁴⁹] for every positive finite binary32 X. Lean checks the integer inequalities equivalent to (1 + 2⁻²⁵)ᴷ > 2¹⁴⁹ and (1 − 2⁻²⁵)ᴷ < 2⁻¹⁴⁹. Taking positive roots, and using monotonicity for degrees larger than K, places the root strictly between 1 − 2⁻²⁵ and 1 + 2⁻²⁵. The lower endpoint is the midpoint between 1 and its predecessor. The upper endpoint is below the midpoint between 1 and its successor. Nearest rounding therefore returns magnitude 1. The sign of X−1 and the sign of n determine the side for directed rounding. The implementation handles zero, unit magnitudes, infinities, NaNs and negative radicands with even degrees separately. [Integer inequalities](LargeDegree.lean)

## Formal coverage

Lean models nonnegative fractions by integer numerator and positive denominator. It allows separate truncation units at the two endpoints at each multiplication. It proves enclosure for arbitrary multiplier bits, strict comparison soundness, the natural-number significand bound, the degree bound for exact odd powers, and the error-factor count of the squaring schedule. It does not model C memory, the limb implementation, IEEE-754 expressions, the compiler or the entire library API. Tests separately compare the C checker with MPFR at several precisions. [Lean source](PowerInterval.lean)
