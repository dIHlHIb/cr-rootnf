#!/bin/bash
# Start W workers (run this inside tmux). Locks left by an interrupted run are
# cleared first; finished shards are never redone. A shard whose run was cut
# short has no done/ marker, so its partial candidate file is truncated first.
set -u
cd "$(dirname "$0")"
W=${1:-5}
mkdir -p logs cand done
rm -rf lock
mkdir -p lock
for f in cand/*.txt; do
  [ -e "$f" ] || continue
  name=$(basename "$f" .txt)
  [ -e "done/$name" ] || : > "$f"
done
echo "$(date -u +%FT%TZ) start W=$W" >> logs/STATUS
for ((i = 0; i < W; i++)); do ./worker.sh "$i" & done
wait
echo "$(date -u +%FT%TZ) all workers finished" >> logs/STATUS
