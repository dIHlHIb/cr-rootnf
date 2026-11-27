# SPDX-License-Identifier: MIT
# Copyright (c) 2026 dIHlHIb
# Developed with AI assistance.
"""Generate every non-unit positive exact grid hit, without scanner candidates."""
from pathlib import Path
import math
import struct
import sys

def word(x):
    return int.from_bytes(struct.pack(">f",x),"big")

def exact_pairs():
    for e in range(-149,128):
        if not e: continue
        x=word(math.ldexp(1.0,e))
        for n in range(2,abs(e)+1):
            if e%n==0:
                yield n,x
                yield -n,x
    for n in range(2,16):
        odd=3
        while (m:=odd**n)<2**24:
            # m is odd; need n*b >= -149 and n*b + floor(log2 m) <= 127.
            for b in range(-(149//n),(128-m.bit_length())//n+1):
                yield n,word(math.ldexp(m,b*n))
            odd+=2

independent=set(exact_pairs())
listed=set()
for line in Path(sys.argv[1]).read_text().splitlines():
    if line and not line.startswith("#"):
        x,n=line.split(",")
        listed.add((int(n),word(float.fromhex(x))))
print(f"all positive non-unit exact grid hits: constructed={len(independent)} file={len(listed)} "
      f"only-constructed={len(independent-listed)} only-file={len(listed-independent)}")
raise SystemExit(int(independent!=listed))
