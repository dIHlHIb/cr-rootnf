#!/bin/bash
# Independent end-to-end check of cr_rootnf against MPFR (via gmpy2).
# Run from this directory after building src/binary32/rootn (make).
set -e
cd "$(dirname "$0")"
LIB=../../src/binary32/rootn
WC=../../worstcases
CC=${CC:-cc}
FP="-std=gnu11 -frounding-math -ffp-contract=off"
$CC -O2 $FP -I$LIB drv.c $LIB/librootnf.a -lm -o drv
python3 gen.py $WC/rootnf.wc $WC/rootnf_exact.wc ${NRANDOM:-1000000} ${SEED:-20261126} > full_in.txt
./drv < full_in.txt > full_out.txt
python3 check.py full_out.txt ${WORKERS:-4}
python3 make_fpe.py $LIB/rootnf.c > fpe.c
$CC -O2 $FP -fno-math-errno -I$LIB fpe.c -lm -o fpe
python3 fpe_inputs.py > fpe_in.txt
./fpe < fpe_in.txt > fpe_out.txt
python3 fpe_check.py fpe_out.txt ${WORKERS:-4}
