# SPDX-License-Identifier: MIT
# Copyright (c) 2026 dIHlHIb
# Developed with AI assistance.
"""Probe both sides of the inclusive large-degree branch and signed-long-long limits."""
degrees = [sign*(2**32+delta) for sign in (-1,1) for delta in (-1,0,1)]
degrees += [-(2**63),2**63-1]
words = {1,2,3,0x3f7fffff,0x3f800000,0x3f800001,0x7f7fffff}
for exponent in range(255):
    base = exponent << 23
    words.update(base + m for m in (0,1,0x400000,0x7ffffe,0x7fffff))
for n in degrees:
    for u in sorted(words - {0}):
        print(f"{u:08x} {n}")
        if n % 2:
            print(f"{u | (1<<31):08x} {n}")
