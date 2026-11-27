# Correctly rounded nth roots in binary32

`cr_rootnf(x, n)` takes a single-precision number and a signed 64-bit degree. It returns the nth root rounded in the caller's current IEEE 754 rounding mode. For example, the cube root of `0x1p-126f` is exactly `0x1p-42f`. This implementation returns that value in all four modes and leaves the inexact flag clear.

Most inputs need a binary64 approximation and a cast to float. When the approximation is close to a rounding boundary, the function raises the boundary to an integer power and compares it with the input. A search over about 201 billion grid-point and degree pairs found the closest nonzero logarithmic gap at `x = 0x1.153bfcp+46f`, `n = 8649606`. Even there, the first 128-bit interval comparison suffices. The [search report](worstcases/SCAN.md) gives the assumptions, error bounds and worst cases. The [implementation notes](proof/NUMERICS.md) explain the separate treatment of exact results, large degrees and exceptional inputs.

## What this adds

Gladman and colleagues report a largest known error of 1.84 ulp for glibc 2.44's `rootnf` in round-to-nearest mode, at `x = 0x1.05986cp-123f`, `n = 15`. Glibc is the only library with a binary32 `rootn` result in their comparison. [August 2026 accuracy report, Tables 1 and 2](https://members.loria.fr/PZimmermann/papers/accuracy.pdf#page=4).

- `cr_rootnf` rounds in all four modes and handles the IEEE exception flags and `errno`.
- For positive finite `x` other than 1 and 2 ≤ |n| ≤ 2³², the data include all 278,559 exact pairs and the 2,855 inexact pairs selected by the scanner. It uses a numerical cutoff of 2⁻⁵⁰. After allowing for the scanner's error, the list covers every pair whose true logarithmic gap is at most 2⁻⁵¹. [Worst cases and coverage](worstcases/SCAN.md)
- Under the assumptions in the search report, the first 128-bit interval decides every input that reaches the integer comparison.

## Prior work

GNU MPFR already computes correctly rounded roots of signed integer degree at a caller-selected precision. Revy studied correctly rounded roots for a fixed degree, including binary32 examples. Lefèvre published hard-to-round binary64 roots for degrees −180 through −2 and 3 through 5188. This implementation takes the degree at run time. Its binary32 search covers 2 ≤ |n| ≤ 2³², and a separate bound handles larger degrees. [MPFR](https://www.mpfr.org/mpfr-4.2.0/) · [Revy, Chapter 3](https://perso.univ-perp.fr/guillaume.revy/publications/files/pdf/Revy09.pdf) · [Lefèvre, slides 19–20](https://www.vinc17.net/research/slides/tamadi2013-10.pdf#page=19)

RTOSKit has the same native C signature. Its published x86-64 tests report 36 incorrectly rounded `rootnf` results in 4,194,304 cases. Luna-Flow's software floating-point library supports correctly rounded integer roots and tests a binary32 context, using an integer-root path for selected degrees and an adaptive calculation elsewhere. [RTOSKit accuracy results](https://github.com/rtoskit-org/rtoskit-libc/blob/2af43f18f4cceb287720598b1fb4e5f13a6d4cb1/docs/research/libm-float-pow.md#accuracy) · [Luna-Flow implementation](https://github.com/Luna-Flow/floating/pull/388)

## Build and use

On an IEEE binary32/binary64 platform, build with a recent GCC or Clang that supports dynamic rounding, fused multiply-add and `__builtin_roundeven`.

```sh
make -C src/binary32/rootn
```

Include `rootnf.h`, link `src/binary32/rootn/librootnf.a` and the system math library (`-lm`). The supplied Makefile enables `errno`. MPFR is a test dependency. The library itself uses C and the platform math library.

## Check the results

The tests need Python 3.9 or later, GMP, and MPFR 4.2.0 or later with their development headers. The MPFR test driver requires a 64-bit C `long` to pass the full degree range to `mpfr_rootn_si`. Run it from the repository root.

```sh
./verify
```

The tests check result bits, floating-point exception flags, preservation of the caller's rounding mode and `errno`. They also deliberately break the interval multiplication and check that the tests detect it. To run the two Lean 4.19.0 proofs and print their axioms, use `./verify --lean /path/to/lean`. To check the 824 binary32 rows from glibc 2.44, add `--glibc-reference /path/to/glibc-2.44/math/auto-libm-test-out-rootn`. The test driver accepts `MPFR_CFLAGS` and `MPFR_LIBS` if pkg-config cannot locate the libraries.

The [independent C and MPFR tests](tests/independent/README.md) recorded 5,216,384 calls in four modes with zero mismatches on an Apple M4. The [independent search tools](worstcases/audit/README.md) regenerate selected degree ranges, all 278,559 non-unit positive exact cases and the bounds on the two smallest distances. The Lean files prove properties of the integer interval algorithm and the large-degree cutoff. The fast-path error estimate and the full search use numerical arguments and executable checks. The search measures the distance to both floats and the midpoints between them, so its separation bound covers directed rounding as well as rounding to nearest.

AI systems wrote and reviewed the implementation, tests and proof material. The code is available under the [MIT license](LICENSE).
