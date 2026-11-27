"""Measure the fast-path relative error of cr_rootnf against 300-bit MPFR.

Input: fpe output lines "xbits n y_RN y_RU y_RD y_RZ" (binary64 bits).
Reports the maximum of |y - x^(1/n)| / x^(1/n) per rounding mode, the
worst inputs, and the margin to the 2^-42 bound used by the fast-path
guard (8192 binary64 steps, >= 2^-40 relative).
Usage: python3 fpe_check.py fpe_out.txt [workers]
"""
import struct
import sys
from multiprocessing import Pool

import gmpy2
from gmpy2 import mpfr

gmpy2.get_context().precision = 300


def f32(b):
    return struct.unpack(">f", struct.pack(">I", b))[0]


def f64(h):
    return struct.unpack(">d", bytes.fromhex(h))[0]


def one(line):
    p = line.split()
    xb, n = int(p[0], 16), int(p[1])
    a = mpfr(f32(xb & 0x7FFFFFFF))
    y = gmpy2.root(a, n) if n > 0 else 1 / gmpy2.root(a, -n)
    errs = []
    for h in p[2:6]:
        v = mpfr(f64(h))
        e = abs(v - y) / y
        errs.append(float(gmpy2.log2(e)) if e > 0 else -999.0)
    return errs, p[0], n


def main():
    path = sys.argv[1]
    workers = int(sys.argv[2]) if len(sys.argv) > 2 else 4
    lines = open(path).read().splitlines()
    worst = [(-999.0, "", 0)] * 4
    hist = {}
    with Pool(workers) as pool:
        for errs, xh, n in pool.imap(one, lines, chunksize=2000):
            for m, e in enumerate(errs):
                if e > worst[m][0]:
                    worst[m] = (e, xh, n)
                b = int(-e) if e > -999 else 999
                hist[b] = hist.get(b, 0) + 1
    names = ["RN", "RU", "RD", "RZ"]
    print("inputs %d" % len(lines))
    for m in range(4):
        e, xh, n = worst[m]
        print("%s max rel err 2^%.3f at x=%s n=%d (margin to 2^-42: %.2f bits)"
              % (names[m], e, xh, n, -42 - e))
    print("histogram floor(-log2 err): " +
          ", ".join("%d:%d" % (k, hist[k]) for k in sorted(hist)[:12]))


if __name__ == "__main__":
    main()
