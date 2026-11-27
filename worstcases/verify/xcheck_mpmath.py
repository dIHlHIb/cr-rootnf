"""Independent recomputation of the hardest cases with mpmath (no gmpy2)."""
import mpmath as mp
mp.mp.prec = 600
cases = [("0x1.153bfcp+46", 8649606), ("0x1.ae6b22p-52", 595995026),
         ("0x1.2946c2p-64", -5488), ("0x1.7374480000000p+62", -1454496471)]
for xs, n in cases:
    x = mp.mpf(float.fromhex(xs))
    y = mp.power(x, mp.mpf(1) / n)
    e = int(mp.floor(mp.log(y, 2)))
    half_ulp = mp.mpf(2) ** (e - 24)
    g = mp.nint(y / half_ulp) * half_ulp
    r = mp.log(x, 2) - n * mp.log(g, 2)
    rel = abs(y - g) / g
    print("%s n=%d  log2|r|=%.3f  log2|y-g|/g=%.3f  g=%s" % (xs, n, float(mp.log(abs(r), 2)),
          float(mp.log(rel, 2)), mp.nstr(g, 12)))
