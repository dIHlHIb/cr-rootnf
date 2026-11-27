# SPDX-License-Identifier: MIT
# Copyright (c) 2026 dIHlHIb
# Developed with AI assistance.
"""Run the candidate binary against the two complete supplied .wc files."""
from pathlib import Path
import argparse, hashlib, os, re, shlex, shutil, struct, subprocess, tempfile

parser = argparse.ArgumentParser()
parser.add_argument("data", type=Path)
args = parser.parse_args()
work = Path(__file__).resolve().parent
out = Path.cwd()
src = work.parents[1] / "src/binary32/rootn"
cc = shlex.split(os.getenv("CC","cc")) + ["-O2","-std=gnu11","-ffp-contract=off",
                                        "-frounding-math","-fno-math-errno","-DCORE_MATH_SUPPORT_ERRNO"]
inc = shlex.split(os.environ.get("MPFR_CFLAGS",""))
libs = shlex.split(os.environ.get("MPFR_LIBS","-lmpfr -lgmp")) + ["-lm"]
with tempfile.TemporaryDirectory() as tmp:
    p = Path(tmp)
    local = p/"src"
    shutil.copytree(src,local)
    h = local/"power_interval.h"
    t = h.read_text()
    anchor = "if (root_at_precision(x, n, grid, k, local, side)) return 1;"
    assert t.count(anchor)==1
    h.write_text(t.replace(anchor, "audit_grid_calls++;\n    "+anchor+"\n    audit_refinements++;"))
    f = local/"rootnf.c"
    f.write_text("#include <stdio.h>\nstatic unsigned long long audit_grid_calls, audit_refinements;\n"
                 +f.read_text()+"\n__attribute__((destructor)) static void stats(void) {"
                 'printf("grid calls=%llu first-128-bit refinements=%llu\\n",audit_grid_calls,audit_refinements);}\n')
    subprocess.run(cc+inc+[f,local/"check_special.c",*libs,"-o",p/"check"],check=True)
    subprocess.run(cc+inc+["-DROOTNF_INITIAL_LIMBS=1","-I",src,
                           work.parents[1]/"tests/check_interval.c",*libs,"-o",p/"grid"],check=True)
    seen=set()
    for name in ("rootnf.wc","rootnf_exact.wc"):
        wc=args.data/name
        with wc.open("rb") as inp:
            v=subprocess.run([p/"check","--worst"],stdin=inp,capture_output=True,check=True)
        (out/(name+".log")).write_bytes(v.stdout)
        print(name,hashlib.sha256(wc.read_bytes()).hexdigest(),v.stdout.decode().strip(),flush=True)
        stats = re.search(rb"^grid calls=(\d+) first-128-bit refinements=(\d+)$", v.stdout, re.M)
        if not stats or int(stats[2]) or (name == "rootnf.wc") != (int(stats[1]) > 0):
            raise RuntimeError(f"unexpected default-precision statistics for {name}")
        for row in wc.read_text().splitlines():
            if row.startswith("#") or not row.strip(): continue
            x,n=row.split(",")
            pair=(int.from_bytes(struct.pack(">f",float.fromhex(x)),"big"),int(n))
            if pair in seen: raise ValueError(f"duplicate {pair}")
            seen.add(pair)
    cases="".join(f"{u:08x} {n}\n" for u,n in sorted(seen))
    v=subprocess.run([p/"grid"],input=cases.encode(),capture_output=True,check=True)
    (out/"all-grids.log").write_bytes(v.stdout)
    print(v.stdout.decode().strip(),flush=True)
