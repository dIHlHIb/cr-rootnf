# Independent check

This harness was written separately from the tests in `tests/` and shares no code with them. It needs a C compiler and Python 3 with gmpy2. Run `./run.sh` after building `src/binary32/rootn`.

**End-to-end comparison.**
- `gen.py` writes 1,304,096 inputs:
  - every worst case from `worstcases/` and its two binary32 neighbours;
  - every exact case;
  - special values;
  - one million seeded random pairs, with degrees of both signs from 1 up to the 64-bit extremes, plus n = 0.

  Inputs with odd degree are tested with both signs of x.
- `drv.c` calls `cr_rootnf` in all four rounding modes. For each call it records the result bits, the five IEEE flags and errno.
- `check.py` compares the results with MPFR at 450 bits, rounded to binary32 in each mode. It skips errno for n = −1 and ignores underflow for |n| = 1 on non-NaN results. Platforms may detect reciprocal tininess differently. Exit status 1 on any mismatch.

**Fast-path error.**
- `make_fpe.py` cuts the fast path verbatim out of `rootnf.c`.
- `fpe_inputs.py` writes random and targeted inputs.
- `fpe_check.py` reports the largest relative error of the binary64 approximation in each rounding mode. Compare it with the 2^-42 bound used by the guard.

**Results on an Apple M4 (macOS, clang).**
- **End-to-end:** 5,216,384 calls, 0 mismatches.
- **Fast-path maximum relative error:**

  | mode | error |
  |---|---|
  | to nearest | 2^-47.12 |
  | upward | 2^-46.52 |
  | downward | 2^-46.55 |
  | toward zero | 2^-46.10 |

  The worst cases occur at |n| = 2 and 3 with subnormal x or x near FLT_MAX.

**Positive control.** Changing one result by one ulp, dropping an inexact flag, setting a spurious errno, or raising inexact on an exact case was reported in each case.
