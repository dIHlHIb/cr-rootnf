"""Exact-case breakdown and hardness in y-relative terms for dual-scan candidates."""
import sys
import struct
import gmpy2
from gmpy2 import mpfr
sys.path.insert(0, ".")
from verify import f32, grid_point, is_exact
gmpy2.get_context().precision = 400
cands = set()
for fn in sys.argv[1:]:
    for line in open(fn):
        f = line.split()
        cands.add((int(f[0]), f[1]))
x1 = 0
exact_other = []
rows = []
for n, xh in cands:
    x = f32(xh)
    if x == 1.0:
        x1 += 1
        continue
    L = gmpy2.log2(mpfr(x))
    y = gmpy2.exp2(L / n)
    g = grid_point(y)
    if is_exact(x, n, g):
        exact_other.append((n, xh))
        continue
    r = abs(L - n * gmpy2.log2(mpfr(g)))
    rows.append((float(gmpy2.log2(abs(y - g) / g)), float(gmpy2.log2(r)), n, xh))
print("x=1:", x1, " other exact:", len(exact_other))
print("other exact sample:", sorted(exact_other, key=lambda t: abs(t[0]))[-5:])
rows.sort()
print("hardest by relative distance (log2(|y-g|/g), log2 |r|, n, x):")
for t in rows[:8]:
    print("  %.3f  %.3f  n=%d x=%s" % (t[0], t[1], t[2], float(f32(t[3])).hex()))
import collections
h = collections.Counter(int(-t[1]) for t in rows)
print("non-exact |r| histogram (floor -log2):", sorted(h.items()))
