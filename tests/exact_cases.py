# SPDX-License-Identifier: MIT
# Copyright (c) 2026 dIHlHIb
# Developed with AI assistance.
"""Generate every positive dyadic binary32 perfect power of degree ≥2.

Negative radicands are added for odd degrees, negative degrees for dyadic
reciprocals. The infinite-in-degree unit family uses representative degrees.
"""
import sys
from pathlib import Path

def word(odd, e):
    top = odd.bit_length()-1
    if e+top < -126:
        return odd << (e+149)
    return (e+top+127) << 23 | ((odd << (23-top)) & 0x7fffff)


pairs = set()
for n in range(2, 150):
    base = 1
    while (odd := base**n) < (1 << 24):
        for e in range(-149, 128-(odd.bit_length()-1)):
            if e % n:
                continue
            x = word(odd, e)
            pairs.add((x, n))
            if n % 2:
                pairs.add((x | (1 << 31), n))
            if base == 1:
                pairs.add((x, -n))
                if n % 2:
                    pairs.add((x | (1 << 31), -n))
        base += 2
for line in Path(sys.argv[1]).read_text().splitlines():
    x,n = line.split()
    pairs.add((int(x,16),int(n)))
for x,n in sorted(pairs):
    print(f"{x:08x} {n}")
