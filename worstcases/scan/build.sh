#!/bin/bash
# Build the dual scanner. FMA contraction must stay off: the error analysis in
# SCAN.md assumes only the explicit fma() calls are fused.
set -e
cd "$(dirname "$0")"
${CC:-cc} -O3 ${ARCHFLAGS:-} -ffp-contract=off -o dualscan dualscan.c -lm
echo "build ok"
