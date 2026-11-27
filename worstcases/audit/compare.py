# SPDX-License-Identifier: MIT
# Copyright (c) 2026 dIHlHIb
# Developed with AI assistance.
"""Compare independently produced four-component keys, then reconcile both WC files."""
from collections import Counter
from pathlib import Path
import argparse
import hashlib
import struct
import tarfile

p = argparse.ArgumentParser()
p.add_argument("data", type=Path)
data = p.parse_args().data
results = Path.cwd()
scopes = {"cubes": (3, 3), **{str(n): (n, n) for n in (-5488, 8649606, 595995026, -1454496471)},
          "shard": (2818572288, 2952790015)}

shards = {}
for line in (data / "scan/shards.txt").read_text().splitlines():
    name, ty, a, b = line.split()
    assert name not in shards and ty == "D"
    shards[name] = (int(a), int(b))
for sign in (1,-1):
    intervals = sorted((min(abs(a),abs(b)), max(abs(a),abs(b))) for a,b in shards.values()
                       if a*sign>0 and b*sign>0)
    assert intervals[0][0] == 2 and intervals[-1][1] == 2**32
    assert all(l[1]+1 == r[0] for l,r in zip(intervals,intervals[1:]))

def key(fields):
    n, x, e, i = fields
    return int(n),int(x,16),int(e),int(i)

present=set()
full=set()
grids={}
expected={label:set() for label in scopes}
with tarfile.open(data/"candidates.tgz","r:gz") as archive:
    for entry in archive:
        if entry.isdir(): continue
        path=Path(entry.name)
        assert entry.isfile() and path.parent.as_posix()=="cand" and path.suffix==".txt"
        name=path.stem
        assert name in shards and name not in present
        present.add(name)
        a,b=shards[name]
        for row in archive.extractfile(entry):
            fields=row.split()
            assert len(fields)==5
            n,x,e,i = k = key(fields[:4])
            assert a<=n<=b and (n,x) not in full and 0<=i<2**24
            full.add((n,x))
            grids[n,x]=(e,i)
            for label,(left,right) in scopes.items():
                if left<=n<=right: expected[label].add(k)
assert present==set(shards)
wc=set()
nonexact=[]
for name in ("rootnf.wc","rootnf_exact.wc"):
    for row in (data/name).read_text().splitlines():
        if not row or row.startswith("#"): continue
        h,n=row.split(",")
        x=int.from_bytes(struct.pack(">f",float.fromhex(h)),"big")
        pair=(int(n),x)
        assert pair not in wc
        wc.add(pair)
        if name=="rootnf.wc":
            e,i=grids[pair]
            nonexact.append(f"{pair[0]} {x:08x} {e} {i}\n")
assert wc==full
(results/"nonexact-grids.txt").write_text("".join(nonexact))
print(f"raw/WC pair sets identical ({len(full)} pairs), {len(present)} contiguous degree shards validated")
ok=True
for label in scopes:
    rows=Counter(key(line.split()) for line in (results/f"ours-{label}.txt").read_text().splitlines())
    ref=expected[label]
    extra=rows.keys()-ref
    missing=ref-rows.keys()
    duplicates=sum(rows.values())-len(rows)
    print(f"{label}: independent={len(rows)} archive={len(ref)} only-independent={len(extra)} only-archive={len(missing)} duplicate={duplicates}")
    for tag,items in (("independent",extra),("archive",missing)):
        for item in sorted(items): print(tag,*item)
    ok &= not (extra or missing or duplicates)
for name in ("scan/shards.txt","candidates.tgz","rootnf.wc","rootnf_exact.wc"):
    print(hashlib.sha256((data/name).read_bytes()).hexdigest(),name)
raise SystemExit(int(not ok))
