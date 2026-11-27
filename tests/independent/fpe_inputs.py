"""Inputs for the fast-path error measurement, as "xbits n" lines.

Random pairs plus targeted ones: extreme magnitudes with small degrees
(largest |t|), significands at log-table cell edges, and t close to an
odd multiple of 1/128 (largest reduced argument of the exponential).
Usage: python3 fpe_inputs.py > fpe_in.txt
"""
import random
import struct

rng = random.Random(42)
out = []


def emit(xb, n):
    if n in (0, 1, -1, 2) or abs(n) >= 1 << 32:
        return
    out.append("%08x %d" % (xb, n))


for _ in range(500000):
    xb = rng.randrange(1, 0x7f800000)
    c = rng.random()
    if c < 0.4:
        n = rng.randint(2, 40)
    elif c < 0.7:
        n = rng.randint(41, 100000)
    else:
        n = int(2 ** rng.uniform(17, 32))
    emit(xb, n if rng.random() < 0.5 else -n)
ext = [1, 2, 3, 0x007fffff, 0x00800000, 0x00800001, 0x7f7fffff, 0x7f7ffffe, 0x7f000000, 0x7effffff]
for xb in ext:
    for n in (-2, 3, -3, 4, -4, 5, -5, 7, -7):
        emit(xb, n)
for k in range(128):
    for d in (0, -1, 1, 0xffff):
        mant = (k << 16) + d
        if not (0 <= mant < 1 << 23):
            continue
        for _ in range(40):
            ex = rng.randint(1, 254)
            n = rng.choice([-2, 3, -3, 5, 17, -31, 1000, -65537, 2**31 - 1, -(2**31)])
            emit((ex << 23) | mant, n)
for _ in range(100000):
    n = rng.choice([3, -3, 5, -7, 11, -2, 13, 100, -1000])
    k = rng.randint(-74 * 64, 74 * 64)
    t = (k + rng.choice([0.5, -0.5]) + rng.uniform(-1e-4, 1e-4)) / 64
    L = t * n
    if not (-149 < L < 128):
        continue
    try:
        xb = struct.unpack(">I", struct.pack(">f", 2.0 ** L))[0]
    except OverflowError:
        continue
    if 0 < xb < 0x7f800000:
        emit(xb, n)
print("\n".join(out))
