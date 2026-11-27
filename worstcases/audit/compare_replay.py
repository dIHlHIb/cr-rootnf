# SPDX-License-Identifier: MIT
# Copyright (c) 2026 dIHlHIb
# Developed with AI assistance.
"""Usage: python3 compare_replay.py worstcases replay-cand.tgz (or completed worker directory)."""
import argparse
from collections import Counter
from contextlib import ExitStack
import hashlib
from pathlib import Path
import tarfile

def records(f,limits):
    lo,hi=limits
    items={}
    for line in f:
        n,x,e,i,r=line.split()
        key=int(n),int(x,16),int(e),int(i)
        if not (lo<=key[0]<=hi and 0<key[1]<0x7f800000 and 0<=key[3]<2**24):
            raise ValueError(key)
        if key in items: raise ValueError(f"duplicate {key}")
        items[key]=float.fromhex(r.decode()).hex()
    return items

def handles(path,expected,stack):
    if path.is_dir():
        files={p.stem:p for p in (path/"cand").glob("*.txt")}
        if {p.name for p in (path/"done").iterdir()} != expected:
            raise ValueError("incomplete replay: check done markers")
        if (path/"logs/FAILED").exists() and (path/"logs/FAILED").stat().st_size:
            raise ValueError("worker failures: inspect logs before certifying the replay")
        if set(files)!=expected: raise ValueError("wrong candidate file inventory")
        return lambda name: files[name].open("rb")
    archive=stack.enter_context(tarfile.open(path,"r:gz"))
    files={}
    for m in archive.getmembers():
        if m.isdir(): continue
        loc=Path(m.name)
        if not m.isfile() or loc.suffix!=".txt" or loc.stem in files:
            raise ValueError(m.name)
        files[loc.stem]=m
    if set(files)!=expected: raise ValueError("wrong archived shard inventory")
    return lambda name: archive.extractfile(files[name])

p=argparse.ArgumentParser(description=__doc__)
p.add_argument("original",type=Path)
p.add_argument("replay",type=Path)
args=p.parse_args()
intervals={}
for line in (args.original/"scan/shards.txt").read_text().splitlines():
    name,kind,a,b=line.split()
    if name in intervals or kind!="D": raise ValueError(line)
    intervals[name]=int(a),int(b)
if len(intervals)!=138: raise ValueError("expected 138 shards")
counts=Counter()
with ExitStack() as stack:
    read_old=handles(args.original/"candidates.tgz",set(intervals),stack)
    read_new=handles(args.replay,set(intervals),stack)
    for name,limits in intervals.items():
        with read_old(name) as a,read_new(name) as b:
            old=records(a,limits)
            new=records(b,limits)
        removed=old.keys()-new.keys()
        added=new.keys()-old.keys()
        flips={k for k in new.keys()&old.keys() if new[k]!=old[k]}
        for label,items in (("old-only",removed),("new-only",added),("residual-changed",flips)):
            for k in sorted(items): print(name,label,*k,old.get(k),new.get(k))
        counts.update(shards=1,old=len(old),new=len(new),removed=len(removed),added=len(added),
                      changed_residual=len(flips))
print(" ".join(f"{k}={v}" for k,v in counts.items()))
for path in (args.original/"candidates.tgz",args.replay):
    if path.is_file(): print(hashlib.sha256(path.read_bytes()).hexdigest(),path.name)
raise SystemExit(int(any(counts[k] for k in ("removed","added","changed_residual"))))
