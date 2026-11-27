# Independent checks of the candidate archive

From the repository root, run:

```sh
./worstcases/audit/recheck --table
```

The checker needs an LP64 C compiler with 128-bit unsigned integers, Python 3.9 or later, GMP and MPFR 4.2.0 or later. The `MPFR_CFLAGS` and `MPFR_LIBS` overrides work as in the root verifier. Build products and results are temporary by default. Add `--work DIR` to keep the raw lists and logs.

The independent scanner searches without reading the candidate archive. For degree 3 it cubes every 25-bit significand exactly. For four other degrees it brackets powers with directed MPFR operations. Across degrees 2,818,572,288 through 2,952,790,015, only the first midpoint below 1 can qualify. The scanner bounds each successive power of that midpoint by an outward-rounded fixed-point recurrence, reseeded every 8,192 steps. Each surviving input receives a directed MPFR check of the strict log threshold.

| Degree or inclusive range | Matching input and grid-point pairs |
| --- | ---: |
| 3 | 10,954 |
| −5,488 | 1 |
| 8,649,606 | 1 |
| 595,995,026 | 1 |
| −1,454,496,471 | 1 |
| 2,818,572,288 through 2,952,790,015 | 2 |

The comparison matches the signed degree, input bits, grid exponent and grid index in both directions. Further checks regenerate the exact inputs, enclose the minimum distances for all 2,855 inexact candidates, test the complete candidate list against the C implementation in all four rounding modes, and regenerate the library's smaller regression fixture. The optional table check bounds the error in all 16,777,216 high/low logarithm entries using directed MPFR arithmetic.

For a full replay, follow [the scanner instructions](../SCAN.md). After all workers finish, compare every output and residual against the shipped archive from the repository root:

```sh
python3 worstcases/audit/compare_replay.py worstcases worstcases/scan
```
