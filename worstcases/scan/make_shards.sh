#!/bin/bash
# Write shards.txt for the dual scan over 2 <= |n| <= 2^32, both signs.
# Cost per n is about 4.7e9/|n| pairs plus ~0.5 us; shards are sized to
# roughly 30-60 s each on one core.
cd "$(dirname "$0")"
: > shards.txt
add() {  # name lo hi
  echo "D_p$1 D $2 $3" >> shards.txt
  echo "D_m$1 D -$3 -$2" >> shards.txt
}
for ((n = 2; n < 16; n++)); do add "$n" $n $n; done
a=16
while [ $a -lt $((1 << 27)) ]; do add "$a" $a $((2 * a - 1)); a=$((2 * a)); done
step=$((1 << 27))
a=$((1 << 27))
while [ $a -le $((1 << 32)) ]; do
  b=$((a + step - 1)); [ $b -gt $((1 << 32)) ] && b=$((1 << 32))
  add "$a" $a $b
  a=$((a + step))
done
echo "shards: $(wc -l < shards.txt)"
