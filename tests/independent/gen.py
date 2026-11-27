"""Generate cr_rootnf test inputs as "xbits_hex n" lines.

Sources: the full non-exact hard-case list, the full exact list, their
1-ulp neighbours, special values, and seeded random (x, n) pairs whose
degrees span small, medium, large, near-2^32 and 64-bit extremes.
Usage: python3 gen.py hard.wc exact.wc n_random seed > inputs.txt
"""
import random
import struct
import sys

LLMIN, LLMAX = -(1 << 63), (1 << 63) - 1


def bits(v):
    return struct.unpack(">I", struct.pack(">f", v))[0]


def read_wc(path):
    out = []
    for line in open(path):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        xs, ns = line.split(",")
        out.append((bits(float.fromhex(xs)), int(ns)))
    return out


def emit(xb, n):
    sys.stdout.write("%08x %d\n" % (xb, n))


def emit_signed(xb, n):
    emit(xb, n)
    if n & 1:
        emit(xb | 0x80000000, n)


def random_degree(rng):
    c = rng.random()
    if c < 0.30:
        n = rng.randint(1, 20)
    elif c < 0.50:
        n = rng.randint(21, 1000)
    elif c < 0.70:
        n = int(2 ** rng.uniform(10, 32))
    elif c < 0.80:
        n = (1 << 32) + rng.randint(-8, 8)
    elif c < 0.95:
        n = int(2 ** rng.uniform(32, 62.9))
    else:
        return rng.choice([LLMIN, LLMAX, LLMIN + 1, 0])
    return n if rng.random() < 0.5 else -n


def main():
    hard_path, exact_path, n_random, seed = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
    hard = read_wc(hard_path)
    for xb, n in hard:
        emit_signed(xb, n)
        for d in (-1, 1):
            emit_signed(xb + d, n)
    for xb, n in read_wc(exact_path):
        emit_signed(xb, n)
    specials = [0x00000000, 0x80000000, 0x7f800000, 0xff800000, 0x7fc00000,
                0x7fa00000, 0x3f800000, 0xbf800000, 0x00000001, 0x80000001,
                0x007fffff, 0x00800000, 0x7f7fffff, 0xff7fffff, 0x3f7fffff,
                0x3f800001, 0x00200000, 0x00200001, 0x7e800000, 0x7e800001,
                0x7f000000, 0x7f000001]
    degrees = [0, 1, -1, 2, -2, 3, -3, 4, -4, 15, -15, 16, -16, 149, -149, 150,
               (1 << 32) - 1, 1 << 32, (1 << 32) + 1, -(1 << 32) + 1, -(1 << 32),
               -(1 << 32) - 1, LLMAX, LLMIN, LLMIN + 1]
    for xb in specials:
        for n in degrees:
            emit(xb, n)
    rng = random.Random(seed)
    for _ in range(n_random):
        xb = rng.randrange(1, 0x7f800000) | (0x80000000 if rng.random() < 0.5 else 0)
        emit(xb, random_degree(rng))


if __name__ == "__main__":
    main()
