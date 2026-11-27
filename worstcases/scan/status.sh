#!/bin/bash
# progress summary
cd "$(dirname "$0")"
tot=$(wc -l < shards.txt)
dn=$(ls done 2>/dev/null | wc -l)
echo "done $dn/$tot (dual scan)"
echo "running: $(ls lock 2>/dev/null | tr '\n' ' ')"
[ -e logs/FAILED ] && { echo "FAILED:"; cat logs/FAILED; }
tail -2 logs/STATUS 2>/dev/null
uptime
