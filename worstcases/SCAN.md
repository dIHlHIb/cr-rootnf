# Worst cases of binary32 rootn for all 64-bit degrees

This directory holds the hardest-to-round inputs of `rootnf(x, n)` (binary32 x, signed 64-bit n), the program that found them, and the error analysis that guarantees coverage below the stated threshold.

## Result

Let x be a positive finite binary32 number, let n satisfy 2 ≤ |n| ≤ 2^32, and let y = x^(1/n). A *grid point* is a binary32 value or the midpoint of two consecutive binary32 values; these are the rounding boundaries for all four IEEE rounding directions. Let g be the grid point nearest to y and define

    r = log2 x − n·log2 g.

The input is *exact* if y = g. For every input that is not exact:

| measure | approximate minimum | attained at |
|---|---|---|
| \|r\| | 2^-64.820 (the strict bound \|r\| > 2^-65 holds) | x = 0x1.153bfcp+46, n = 8649606 |
| \|y − g\| / g | 2^-88.585 | x = 0x1.ae6b22p-52, n = 595995026 |

- **Exact inputs.** Apart from x = 1, which is exact for every n, there are 278,559 exact inputs (x > 0). Among them, n = 2 contributes 262,142, n = 3 contributes 10,954 and n = 4 contributes 2,078. Every exact root is a binary32 value: in the normal output range a midpoint has a 25-bit odd significand. Its positive power of degree ≥ 2 has an odd part at least 2^48, while its reciprocal has a nontrivial odd denominator.
- **Signs.** Negative x is defined only for odd n, and the result is symmetric, so negative x needs no separate search.
- **Other degrees.**
  - For positive finite x and |n| ≥ 2^32 the root lies strictly between 1 − 2^-25 and 1 + 2^-25, so its rounding follows from the side of 1 alone (`proof/LargeDegree.lean`).
  - The cases n = 0 and n = ±1 need no search.
- **Consequence for the implementation.** The integer interval comparison in `src/binary32/rootn` displaces log2 by less than 2^-93 at 128 bits (`proof/NUMERICS.md`). Since every inexact input has |r| > 2^-65, every input that reaches that comparison is decided in the first 128-bit round.

The distribution of the 2,855 inexact candidates, grouped by floor(−log2 |r|):

| group | 50 | 51 | 52 | 53 | 54 | 55 | 56 | 57 | 58 | 59 | 64 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| count | 1707 | 622 | 255 | 145 | 53 | 37 | 21 | 8 | 4 | 2 | 1 |

An unlisted pair has |r| ≥ 2^-50 − E > 2^-51, where E is the scanner error bound below. Thus u = |ln(y/g)| = |r|·ln(2)/|n| > 2^-84. For either sign of ln(y/g), |y/g − 1| is at least 1 − exp(−u), which exceeds 2^-85 since 1 − exp(−a) ≥ a/2 for 0 ≤ a ≤ 1. This separates every unlisted pair from the reported minimum near 2^-88.585.

## Method: dual enumeration

**Why enumerate grid points.** If |r| < 2^-50, then X = g^n lies within a factor 2^|r| of x. The binary32 neighbours of x are at least 2^-24 away in relative terms, so x is the binary32 number nearest to X.

**The scan.** For each n the scanner `scan/dualscan.c` therefore enumerates grid points instead of inputs:
1. Take every g = 2^e·(1 + i·2^-24), 0 ≤ i < 2^24, with n·log2 g in [−151, 128).
2. Let x be the binary32 number nearest to g^n.
3. Compute r in double-double arithmetic and report the triple when |r| < 2^-50.

**Cost.** This visits about 2^24·278/|n| points per degree instead of 2^31 inputs. All 2 ≤ |n| ≤ 2^32 together are 2.006·10^11 pairs. They are split into 138 shards: one shard per degree up to 15, doubling ranges up to 2^27, then steps of 2^27. The midpoint-corrected replay used 5.24 CPU-hours on an Apple M4.

**The log table.** Every log2(1 + i·2^-24), i < 2^24, is precomputed in double-double (256 MB per process). The odd entries of the table are the logarithms of the rounding midpoints. The scanner uses them to choose x:
- It starts from an exp2 guess.
- It then moves to the mantissa whose rounding cell [log2 of lower midpoint, log2 of upper midpoint) contains the fractional part of n·log2 g.

The guess only affects how many steps this takes. The subnormal range uses log2(2m ± 1) − 1 in the same way.

## The scanner's own error

Write E for the absolute error of the computed r (the variable named r in `pair()`). We show E < 2^-53.99. We also show that for every triple with true |r| < 2^-50 the scanner selects the same x and g. Then every non-unit grid triple with true |r| < 2^-50 − E is reported. The unit grid point is handled separately in step 7.

Every reported candidate is recomputed with MPFR (see *Verification*). So every inexact input satisfies |r| ≥ min(r_min, 2^-50 − E) = r_min, where r_min ≈ 2^-64.820 is the smallest listed value. (For a strict bound, quote |r| > 2^-65; a rounded decimal exponent is not a valid lower bound.)

### Assumptions

- binary64 arithmetic in round-to-nearest, with IEEE-correct +, −, ×, fma, floor and rint.
- No FMA contraction (`-ffp-contract=off`); only the explicit `fma` calls are fused.
- In the normal branch, the integer rint((exp2(f) − 1)·2^23) is representable as `int64_t`. In the subnormal branch, the initial guess rint(exp2(s)) is not a NaN. No accuracy of exp2 is assumed. The variant `scan/dualscan_strict.c` guards both conversions and needs neither assumption.

### Steps

Write g = 2^e(1 + i·2^-24), ℓ = log2 g and t = n·ℓ. Every triple that matters (true |r| < 2^-50) has |t| ≤ 149 + 2^-49, hence |n| ≤ min(2^32, 151/|ℓ|).

1. **Table.** The [directed MPFR check](audit/table_certificate.c) bounds the error in the exact sum of the two words by 0x6.d0abe3fc4b9fp-108 < 2^-105 over all 2^24 mantissas. The high words are strictly increasing and the low words have magnitude at most 2^-52. The weighted errors can also be inspected with `bound/tabcheck.py`.
2. **Forming log2 g.** For qualifying triples, |e| ≤ 76. FastTwoSum(e, Th) is exact because e is zero or |e| ≥ |Th|. Its residual is at most 2^-47. Adding Tl to that residual costs less than 2^-99. Multiplication by |n| ≤ 2^32 makes the table and addition errors together smaller than 2^-73 + 2^-67.
3. **Forming t.** The binary64 product n·lh has magnitude below 151 on the pairs under consideration. The FMA recovers its exact rounding residual, of magnitude at most 2^-46. Since |ll| < 2^-46, the magnitude of n·ll is below 2^-14. Rounding this product and adding it to the recovered residual cost at most 2^-67 and 2^-66 respectively. For non-unit g, |log2 g| > 2^-25. Since |ll| < 2^-46 and the sum lh + ll approximates log2 g within 2^-98, |lh| > 2^-26. Hence |n·ll| / |n·lh| < 2^-20 for the same n. The relative error of the rounded high product is at most 2^-53. Even after rounding the low product and the sum with the FMA residual, their combined magnitude is less than half the high product. FastTwoSum therefore renormalizes exactly. In total the pair of words represents t with absolute error below 2^-64.
4. **f = t − et.**
   - For 0 ≤ th < 1, the subtraction uses zero. For th ≥ 1 or th ≤ −1, the difference from the integer floor is exactly representable (it is a multiple of the input spacing whenever that spacing is at most one).
   - For th ∈ [−1, −1/2] it is exact by Sterbenz.
   - Only th ∈ (−1/2, 0) rounds, by at most **2^-54**. This is the dominant term.
   - When th is an integer and tl < 0, et = th − 1 and th − et = 1 exactly.
5. **Choice of x.**
   - **Normal range.** Work in the binade selected by et. For a qualifying triple, t − et is within 2^-50 of log2(x) − et. At a binade boundary the same x can be represented by index 0 in the upper binade or by the carry index 2^23 in the lower one. The distance in log2 from a binary32 value to its nearest cell edge is at least log2(1 + 2^-25) > 2^-25. Thus the true coordinate stays more than 2^-26 from *every* edge tested by the walk. The stored coordinate has error below 2^-54 + 2^-64. Each normal edge is an odd log-table entry whose error is below 2^-105. In `below`, the two subtractions and the final addition have combined absolute error below 2^-48 on these bounded operands. All tests therefore point toward the correct cell, including at the two ends of the index range.
   - **Subnormal range.** In units of 2^-149, x has an integer significand m ≤ 2^23. Its upper and lower cell edges are m ± 1/2. For m ≥ 1 the log distance to either existing edge is at least log2(1 + 2^-24), again greater than 2^-25. The code compares the coordinate t + 149 against log2(2m ± 1) − 1. Adding 149 to th is exact by Sterbenz. The table lookup, subtraction of 1 and comparison on operands of magnitude at most 25 together perturb the test by less than 2^-43. The walk reaches the same m from any valid starting index. The smallest normal x receives the same bit pattern in both branches.
6. **Assembling r.** For a qualifying pair, the high-word difference between the selected coordinate and the logarithm of x's significand has magnitude below 2^-45. This follows from the true difference below 2^-50, the coordinate error, and the low words (of magnitude below 2^-47, including in the subnormal branch). Rounding the high difference and subtracting the low components therefore each cost less than 2^-97. In the subnormal branch the integer logarithm uses the same two-word table with an integer exponent ≤ 23, adding less than 2^-99 in error. TwoSum combines the differences exactly before the last binary64 rounding. The assembly terms in total cost less than 2^-94.
7. **Enumeration bounds.**
   - **Range of log2 g.** The bounds on log2 g are widened by 2^-30, far more than the rounding of e + Th (at most 2^-45) or of −151/n. Every triple that matters has ℓ at least 2^-31 inside the bounds.
   - **Range of t.** Restricting th to [−151, 128) loses nothing, because every triple that matters has t ∈ [−149 − 2^-49, 128 − 2^-25]. Here log2(FLT_MAX) = 128 + log2(1 − 2^-24) ≈ 128 − 2^-23.47.
   - **The skipped point g = 1.** Skipping g = 1 loses no hard case. There r = log2 x, and for x ≠ 1 the binary32 number nearest to 1 is 1 − 2^-24, so |r| > 2^-24. The input x = 1 is exact.

**Total: E ≤ 2^-54 + 2^-64 + 2^-94 < 2^-53.99.**

**Measurement.** `bound/scanerr.py` compares the scanner's r with 300-bit MPFR for all 281,414 candidates. The largest deviation is 2^-54.00, at n = 4, x = 0x3f534551 ≈ 0.825. That input has t ∈ (−1/2, 0), the dominant case of step 4, and stays within the bound.

**Replay.** An earlier version of the scanner chose x from exp2 alone. Its completeness proof therefore assumed exp2 had a relative error below 2^-28. The midpoint-corrected scanner in this directory was run over all 138 shards again. It reproduced every candidate line, including every r, of the earlier run bit for bit. All 281,414 lines matched. So the published list does not depend on the accuracy of the system exp2.

The distributed archive has normalized owner, group and timestamps. All 138 candidate files match the original payloads byte for byte.

The strict variant was additionally tested with exp2 replaced by a function returning NaN. On n = −1454496471, 595995026 and 8649606 it produced the same output.

## Verification of the candidates

- **Classification.** `verify/verify.py` recomputes every candidate at 400 bits with MPFR (through gmpy2). It finds the nearest grid point, computes r, and decides exactness with exact rational arithmetic. It writes `rootnf.wc`. `verify/exact_list.py` writes `rootnf_exact.wc`.
- **Hardest four.** `verify/xcheck_mpmath.py` recomputes the four hardest inputs with mpmath at 600 bits, without gmpy2. The results agree to 0.001 in log2.
- **Independent checks.** The [independent audit programs](audit/README.md)
  - re-scanned n = 3, −5488, 8649606, 595995026 and −1454496471, plus the full shard 2818572288 ≤ n ≤ 2952790015;
  - regenerated the exact set from odd parts and exponents.

  The independently generated sets have empty differences in both directions.
- **Through the library.** The C function in `src/binary32/rootn` was compared with MPFR on all 281,414 candidates in all four rounding modes, including flags and errno.

## Files

| path | content |
|---|---|
| `rootnf.wc` | all 2,855 inexact candidates as `x,n` (C `%a`, decimal n), ascending \|r\| |
| `rootnf_exact.wc` | all 278,559 exact inputs as `x,n`, ordered by \|n\| |
| `candidates.tgz` | raw scanner output, one file per shard; lines `n xbits e i rscan` with g = 2^e(1 + i·2^-24) and rscan = n·log2 g − log2 x (note the sign) |
| `scan/` | `dualscan.c` (the scanner used for the published run), `dualscan_strict.c`, double-double helpers and tables, shard and worker scripts |
| `verify/` | MPFR verification, exact-case listing, statistics, mpmath cross-check |
| `audit/` | independent enumerators, directed table and distance checks, full replay comparison |
| `bound/` | table dump and MPFR table check, measured scanner error, single-pair replay |

## Reproducing

Requirements: a C compiler, bash, and Python 3 with gmpy2 (and numpy for `bound/tabcheck.py`). Each worker needs about 300 MB of memory.

```
cd worstcases/scan
bash build.sh && bash selftest.sh   # selftest compares n = 15 and n = -40000 with the reference
bash make_shards.sh                 # writes shards.txt (138 shards)
bash run_all.sh 4                   # 4 workers; restartable; progress: bash status.sh
cd ../verify
python3 verify.py rootnf.wc 5000 ../scan/cand/*.txt
python3 exact_list.py rootnf_exact.wc ../scan/cand/*.txt
```

The resulting `rootnf.wc` and `rootnf_exact.wc` should be byte-identical to the published files. To check the scanner bound, run the following from the `bound/` directory:

```
cc -O2 -ffp-contract=off -I../scan -o tabdump tabdump.c -lm && ./tabdump table.bin
python3 tabcheck.py                 # reads table.bin
python3 scanerr.py ../scan/cand     # candidate directory
```

## Prior work

Revy gave a binary floating-point method specialized to a chosen integer degree, including binary32 examples. Properties 3.4–3.6 discuss exact representability and exclude exact midpoints for |n| ≥ 2. [Revy, 2009, Chapter 3](https://perso.univ-perp.fr/guillaume.revy/publications/files/pdf/Revy09.pdf)

Kornerup, Lauter, Lefèvre, Louvet and Muller studied correctly rounded integer powers in binary64. Lefèvre subsequently reported hard-to-round binary64 integer roots for degrees −180 through −2 and 3 through 5188, with separate examples for nearest and directed rounding. The results here concern binary32. The search covers both signs of the degree through 2³², with the large-degree branch handling the rest of the signed 64-bit range. [Integer powers, ACM TOMS](https://www.vinc17.net/research/papers/rr_crip.pdf) · [Lefèvre, TaMaDi 2013, slides 19–20](https://www.vinc17.net/research/slides/tamadi2013-10.pdf#page=19)

GNU MPFR already computes correctly rounded roots with signed integer degree at a caller-selected precision. The implementation here takes a native binary32 input, returns a binary32 result in the current rounding mode, and uses the supplied separation analysis to bound its fixed-precision comparison. [GNU MPFR, Basic Arithmetic Functions](https://www.mpfr.org/mpfr-current/mpfr.html#Basic-Arithmetic-Functions)
