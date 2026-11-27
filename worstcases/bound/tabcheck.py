"""Check every entry of the dualscan table against MPFR.

err_i = |Th[i] + Tl[i] - log2(1 + i 2^-24)|. The scanner multiplies log2 g by n
with |n| <= min(2^32, 151 / |log2 g|), so the table contributes at most
err_i * min(2^32, 151/|log2 g|) to t = n log2 g. Report that maximum for
g = 1 + i 2^-24 (e = 0), g = (1 + i 2^-24)/2 (e = -1), and |e| >= 1 otherwise
(|log2 g| >= 1 there, factor 151)."""
import sys
from multiprocessing import Pool
import numpy as np
import gmpy2
from gmpy2 import mpfr

N = 1 << 24
CH = 16


def work(c):
    gmpy2.get_context().precision = 200
    lo, hi = c * N // CH, (c + 1) * N // CH
    th = np.fromfile("table.bin", dtype="<f8", count=hi - lo, offset=8 * lo)
    tl = np.fromfile("table.bin", dtype="<f8", count=hi - lo, offset=8 * (N + lo))
    emax = m0 = m1 = 0.0
    arg = (0, 0, 0)
    mono = bool(np.all(np.diff(th) >= 0))
    for k in range(hi - lo):
        i = lo + k
        ex = gmpy2.log2(mpfr(1.0 + i * 2.0 ** -24))
        err = float(abs(mpfr(float(th[k])) + mpfr(float(tl[k])) - ex))
        l0 = float(ex)
        if err > emax:
            emax = err
            arg = (i, 0, 0)
        if i:
            v = err * min(2.0 ** 32, 151.0 / l0)
            if v > m0:
                m0, arg = v, (arg[0], i, arg[2])
        v = err * min(2.0 ** 32, 151.0 / (1.0 - l0))
        if v > m1:
            m1, arg = v, (arg[0], arg[1], i)
    return emax, m0, m1, mono, float(th[0]), float(th[-1]), arg


if __name__ == "__main__":
    with Pool(4) as p:
        res = p.map(work, range(CH))
    import math
    emax = max(r[0] for r in res)
    m0 = max(r[1] for r in res)
    m1 = max(r[2] for r in res)
    mono = all(r[3] for r in res) and all(res[c][5] <= res[c + 1][4] for c in range(CH - 1))
    print("max table abs error      = 2^%.2f" % math.log2(emax))
    print("max err*|n| (e=0, g>1)   = 2^%.2f" % math.log2(m0))
    print("max err*|n| (e=-1, g<1)  = 2^%.2f" % math.log2(m1))
    print("max err*151 (|e|>=1)     = 2^%.2f" % math.log2(emax * 151))
    print("Th non-decreasing:", mono)
