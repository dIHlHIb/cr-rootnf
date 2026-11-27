#!/bin/bash
# One worker: claims shards with mkdir locks and skips finished ones, so the
# run can be restarted at any time. Touch STOP to make workers exit after
# their current shard.
set -u
cd "$(dirname "$0")"
source ./config.sh  # TR
mkdir -p lock done cand logs
while read -r name kind a b; do
  [ -e STOP ] && exit 0
  [ -e "done/$name" ] && continue
  mkdir "lock/$name" 2>/dev/null || continue
  nice -n 15 ./dualscan "$a" "$b" "$TR" "cand/$name.txt" >> "logs/$name.log" 2>&1
  rc=$?
  if [ $rc -eq 0 ]; then
    touch "done/$name"
  else
    echo "$(date -u +%FT%TZ) $name rc=$rc" >> logs/FAILED
  fi
  rmdir "lock/$name"
done < shards.txt
