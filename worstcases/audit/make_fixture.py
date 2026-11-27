# SPDX-License-Identifier: MIT
# Copyright (c) 2026 dIHlHIb
# Developed with AI assistance.
"""Select the release fixture from the two complete WC inputs. Writes to stdout."""
from collections import defaultdict
from pathlib import Path
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("data", type=Path)
data = parser.parse_args().data

def inputs(name):
    for row in (data/name).read_text().splitlines():
        if row and not row.startswith("#"):
            literal, degree = row.split(",")
            yield literal, int(degree)

print("# rootnf positive magnitude, signed degree. Finite non-exact hard cases.")
for literal,n in inputs("rootnf.wc"):
    print(f"{literal},{n}")
print("# Exact powers of two and up to eight other exact examples per signed degree.")
powers=[]
other=defaultdict(list)
for literal,n in inputs("rootnf_exact.wc"):
    v=float.fromhex(literal)
    numerator,denominator=v.as_integer_ratio()
    if numerator & (numerator-1) == 0:
        powers.append((n,v,literal))
    else:
        other[n].append((v,literal))
for n,v,literal in sorted(powers):
    print(f"{literal},{n}")
for n,items in sorted(other.items()):
    items.sort()
    for j in sorted({k*(len(items)-1)//7 for k in range(8)}):
        print(f"{items[j][1]},{n}")
