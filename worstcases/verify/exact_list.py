"""Write the exact cases (y is a binary32 value or midpoint) found by the dual scan as x,n lines."""
import sys
import gmpy2
from gmpy2 import mpfr
sys.path.insert(0, ".")
from verify import f32, c_hex, grid_point, is_exact
gmpy2.get_context().precision = 400
out = []
seen = set()
for fn in sys.argv[2:]:
    for line in open(fn):
        f = line.split()
        key = (int(f[0]), f[1])
        if key in seen:
            continue
        seen.add(key)
        x = f32(f[1])
        L = gmpy2.log2(mpfr(x))
        if is_exact(x, key[0], grid_point(gmpy2.exp2(L / key[0]))):
            out.append((abs(key[0]), key[0], x))
out.sort()
with open(sys.argv[1], "w") as fo:
    fo.write("# rootnf exact cases x,n (y = x^(1/n) is a binary32 value or midpoint)\n")
    for _, n, x in out:
        fo.write("%s,%d\n" % (c_hex(x), n))
print("exact written", len(out))
