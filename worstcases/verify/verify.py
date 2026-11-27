"""Verify rootnf hard-case candidates with MPFR and summarise the search.

Input: candidate files from dualscan ("n xhex e i rhex"); 3-field "n xhex r"
lines are also accepted.
For every distinct (n, x) with x > 0 the script computes, at 400 bits,
  L = log2 x, y = 2^(L/n), g = nearest grid point (binary32 value or midpoint),
  r = L - n log2 g,
and decides exactness (y == g) with exact rational arithmetic. It reports the
smallest |r| over non-exact inputs and writes the hardest cases as
"x_hexfloat,n" lines (positive x; odd n also implies -x by symmetry).

Usage: python3 verify.py out.wc K file1 [file2 ...]
Exit status 1 if any candidate line is malformed.
"""
import heapq
import struct
import sys

import gmpy2
from gmpy2 import mpfr, mpq

gmpy2.get_context().precision = 400
TWO = mpfr(2)


def f32(xh):
    return struct.unpack(">f", bytes.fromhex(xh))[0]


def c_hex(v):
    """C printf %a style for a positive normal or subnormal binary32 value"""
    m, e = float(v).hex().split("p")
    m = m.rstrip("0").rstrip(".")
    return "%sp%s" % (m, e)


def grid_point(y):
    """nearest binary32 value or midpoint to y > 0 (exact mpq)"""
    e = int(gmpy2.floor(gmpy2.log2(y)))
    if TWO ** e > y:
        e -= 1
    if TWO ** (e + 1) <= y:
        e += 1
    step = mpq(2) ** (e - 24)  # half ulp in [2^e, 2^(e+1))
    k = gmpy2.rint(y / mpfr(step))
    return mpq(int(k)) * step


def is_pow2(q):
    a, b = q.numerator, q.denominator
    return a & (a - 1) == 0 and b & (b - 1) == 0


def is_exact(x, n, g):
    """g^n == x exactly (x binary32, g with <= 25 significant bits)"""
    xq = mpq(x)
    if is_pow2(g):
        lg = (g.numerator.bit_length() - 1) - (g.denominator.bit_length() - 1)
        lx = (xq.numerator.bit_length() - 1) - (xq.denominator.bit_length() - 1)
        return is_pow2(xq) and lg * n == lx
    if abs(n) > 64:  # odd part >= 3 and 3^65 > 2^24: g^n has too many bits
        return False
    if n > 0:
        return g ** n == xq
    return False  # odd part of g^|n| * x would have to be 1


def main():
    out, K, files = sys.argv[1], int(sys.argv[2]), sys.argv[3:]
    cands = set()
    bad_lines = 0
    for fn in files:
        for line in open(fn):
            f = line.split()
            if len(f) not in (3, 5) or len(f[1]) != 8:
                bad_lines += 1
                continue
            try:
                cands.add((int(float(f[0])), f[1]))
            except ValueError:
                bad_lines += 1
    hardest = []  # max-heap of the K smallest |r| (stored as negative)
    n_exact = 0
    min_r = None
    for n, xh in cands:
        x = f32(xh)
        if x == 1.0:
            n_exact += 1
            continue
        L = gmpy2.log2(mpfr(x))
        y = gmpy2.exp2(L / n)
        g = grid_point(y)
        if is_exact(x, n, g):
            n_exact += 1
            continue
        r = abs(L - n * gmpy2.log2(mpfr(g)))
        if min_r is None or r < min_r:
            min_r = r
        item = (-r, n, xh)
        if len(hardest) < K:
            heapq.heappush(hardest, item)
        elif item > hardest[0]:
            heapq.heapreplace(hardest, item)
    hardest.sort(reverse=True)
    with open(out, "w") as fo:
        fo.write("# rootnf hard cases: x,n (|r| = |log2 x - n log2 g| ascending)\n")
        for negr, n, xh in hardest:
            fo.write("%s,%d\n" % (c_hex(f32(xh)), n))
    print("candidates %d (malformed lines skipped %d), exact %d, non-exact %d"
          % (len(cands), bad_lines, n_exact, len(cands) - n_exact))
    if min_r is not None:
        print("min |r| over non-exact = 2^%.3f" % float(gmpy2.log2(min_r)))
        for negr, n, xh in hardest[:5]:
            print("  x=%s n=%d |r|=2^%.3f" % (float(f32(xh)).hex(), n, float(gmpy2.log2(-negr))))
    if bad_lines:
        print("CERTIFICATION FAILED: %d malformed candidate lines" % bad_lines)
        sys.exit(1)


if __name__ == "__main__":
    main()
