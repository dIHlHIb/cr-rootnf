"""Check cr_rootnf driver output against MPFR (via gmpy2), independently of
the implementation's own test harness.

Each input line: xbits n, then for RN, RU, RD, RZ: resbits flagmask errno.
Flag mask: 1 inexact, 2 underflow, 4 overflow, 8 divbyzero, 16 invalid.
Expected values follow C23 rootn / IEEE 754-2019 rootn. For finite nonzero
x and n != 0 the reference root is computed with 450-bit MPFR and rounded to
binary32 in each mode (safe: non-exact roots are >= 2^-89 relative away
from every rounding boundary, far above the 2^-440 reference error).
Usage: python3 check.py drv_out.txt [workers]
"""
import errno as E
import struct
import sys
from multiprocessing import Pool

import gmpy2
from gmpy2 import mpfr

MODES = [gmpy2.RoundToNearest, gmpy2.RoundUp, gmpy2.RoundDown, gmpy2.RoundToZero]
F_INEX, F_UNF, F_OVF, F_DBZ, F_INV = 1, 2, 4, 8, 16
QNAN = 0x7FC00000


def f32(b):
    return struct.unpack(">f", struct.pack(">I", b))[0]


def b32(v):
    return struct.unpack(">I", struct.pack(">f", v))[0]


def is_nan(b):
    return (b & 0x7FFFFFFF) > 0x7F800000


def expect(xb, n):
    """list of 4 (bits or 'nan', allowed-flag-predicate, errno or None)"""
    sign = xb >> 31
    mag = xb & 0x7FFFFFFF
    if n == 0:
        return [("nan", F_INV, E.EDOM)] * 4
    if mag > 0x7F800000:
        snan = not (mag & 0x00400000)
        return [("nan", F_INV if snan else 0, 0)] * 4
    if sign and mag and not (n & 1):
        return [("nan", F_INV, E.EDOM)] * 4
    odd = n & 1
    if mag == 0:
        s = (sign << 31) if odd else 0
        if n > 0:
            return [(s, 0, 0)] * 4
        return [(s | 0x7F800000, F_DBZ, E.ERANGE)] * 4
    if mag == 0x7F800000:
        if n > 0:
            return [(xb, 0, 0)] * 4
        return [((sign << 31) if odd else 0, 0, 0)] * 4
    out = []
    with gmpy2.local_context(gmpy2.context(), precision=450) as c:
        a = mpfr(f32(mag))
        y = gmpy2.root(a, n) if n > 0 else 1 / gmpy2.root(a, -n)
        if sign:
            y = -y
    for m in MODES:
        ctx = gmpy2.ieee(32)
        ctx.round = m
        with gmpy2.local_context(ctx) as c:
            r = gmpy2.mul(y, 1)
            r = gmpy2.check_range(r)
            fl = (F_INEX if c.inexact else 0) | (F_OVF if c.overflow else 0) | \
                 (F_UNF if c.underflow else 0)
        rb = b32(float(r))
        if abs(n) != 1:
            fl &= F_INEX  # roots with |n| >= 2 are always normal
        err = None if n == -1 else 0
        out.append((rb, fl, err))
    return out


def check_line(line):
    f = line.split()
    xb, n = int(f[0], 16), int(f[1])
    exp = expect(xb, n)
    bad = []
    for k in range(4):
        rb, mask, er = int(f[2 + 3 * k], 16), int(f[3 + 3 * k]), int(f[4 + 3 * k])
        eb, efl, eer = exp[k]
        ok_val = is_nan(rb) if eb == "nan" else rb == eb
        if abs(n) == 1 and eb != "nan":
            ok_fl = (mask & ~F_UNF) == (efl & ~F_UNF)  # tininess detection may differ
        else:
            ok_fl = mask == efl
        ok_er = eer is None or er == eer
        if not (ok_val and ok_fl and ok_er):
            bad.append("%s mode=%d got=%08x/%d/%d want=%s/%d/%s" % (
                f[0] + " " + f[1], k, rb, mask, er,
                eb if eb == "nan" else "%08x" % eb, efl, eer))
    return bad


def main():
    path = sys.argv[1]
    workers = int(sys.argv[2]) if len(sys.argv) > 2 else 4
    lines = open(path).read().splitlines()
    nbad = 0
    with Pool(workers) as p:
        for bad in p.imap(check_line, lines, chunksize=2000):
            for b in bad:
                nbad += 1
                if nbad <= 40:
                    print("MISMATCH", b)
    print("lines %d calls %d mismatches %d" % (len(lines), 4 * len(lines), nbad))
    sys.exit(1 if nbad else 0)


if __name__ == "__main__":
    main()
