# SPDX-License-Identifier: MIT
# Copyright (c) 2026 dIHlHIb
# Developed with AI assistance.
"""Extract glibc's published binary32 results, preserving their optional flags."""
import struct
import sys
from pathlib import Path

def bits(s):
    if s == "plus_infty": return 0x7f800000
    if s == "minus_infty": return 0xff800000
    return int.from_bytes(struct.pack(">f",float.fromhex(s)),"big")

labels = {"inexact":1, "underflow":2, "overflow":4}
modes = ["tonearest","downward","upward","towardzero"]
for line in Path(sys.argv[1]).read_text().splitlines():
    if not line.startswith("= rootn ") or " binary32 " not in line: continue
    header,answer,notes=line.split(":")
    _,_,mode,_,x,n=header.split()
    required = allowed = errno_rule = 0
    for token in notes.split():
        optional = token.endswith("-ok")
        name = token.removesuffix("-ok")
        if name == "errno-erange":
            errno_rule = 1 if optional else 2
        else:
            flag = labels[name]
            allowed |= flag
            if not optional: required |= flag
    print(f"{bits(x):08x} {int(n.removesuffix('LL'),0)} {modes.index(mode)} "
          f"{bits(answer.strip()):08x} {required} {allowed} {errno_rule}")
