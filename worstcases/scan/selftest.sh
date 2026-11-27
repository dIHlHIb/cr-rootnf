#!/bin/bash
# Self-test: the candidate sets for n = 15 and n = -40000 must equal the
# reference sets of the published run. Exit status 1 on mismatch.
set -u
cd "$(dirname "$0")"
mkdir -p selftest
rm -f selftest/s15.txt selftest/sm40k.txt
./dualscan 15 15 -40 selftest/s15.txt
./dualscan -40000 -40000 -28 selftest/sm40k.txt
md5() { if command -v md5sum >/dev/null; then md5sum; else command md5; fi; }
fail=0
for p in "s15.txt 6c1b7f57af90ddcb99ab35ffdee56604" "sm40k.txt 32f7ae129bedde58d33f4889a53ee2e8"; do
  f=${p% *}
  want=${p#* }
  got=$(cut -d' ' -f1,2,3,4 "selftest/$f" | sort | md5 | cut -c1-32)
  if [ "$got" = "$want" ]; then echo "$f ok ($(wc -l < selftest/$f) lines)"; else echo "$f MISMATCH $got"; fail=1; fi
done
exit $fail
