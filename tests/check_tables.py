# SPDX-License-Identifier: MIT
# Copyright (c) 2026 dIHlHIb
# Developed with AI assistance.
"""Check every high table entry and the entire mantissa domain of each reduction bin."""
import math
import re
from decimal import Decimal as D, localcontext
from pathlib import Path

raw = (Path(__file__).resolve().parents[1] / "src/binary32/rootn/rootnf_tables.h").read_text()


def values(name):
    line = next(x for x in raw.splitlines() if f"double {name}[" in x)
    return [D.from_float(float.fromhex(x)) for x in re.findall(r"-?0x[0-9a-f.]+p[+-]\d+", line)]


with localcontext() as ctx:
    ctx.prec = 90
    ln2 = D(2).ln()
    inv, logs, exps = values("LOG_INV"), values("LOG_T")[::2], values("EXP_T")[::2]
    assert (len(inv), len(logs), len(exps)) == (128,128,64)
    assert all(v * 2**20 == (v * 2**20).to_integral_value() for v in inv)
    rmax = max(abs((D(1) + D(i+side)/128)*v - 1)
               for i,v in enumerate(inv) for side in (0,1))
    assert rmax < D(1)/250
    errors = [
        max(abs(t + v.ln()/ln2) for t,v in zip(logs,inv)),
        max(abs(t - (D(j)/64*ln2).exp()) for j,t in enumerate(exps)),
        max(abs(t - D((-1)**k)/((k+1)*ln2)) for k,t in enumerate(values("LOGP"))),
        max(abs(t - ln2**k / math.factorial(k)) for k,t in enumerate(values("EXPQ"),1)),
    ]
    assert max(errors) < D(2)**-52
    log_tail = rmax**7 / (7*ln2*(1-rmax))
    a = ln2/128
    exp_tail = a.exp()*a**6/math.factorial(6)
    assert log_tail < D(2)**-57 and exp_tail < D(2)**-54
    print("maximum |r|", rmax)
    print("max high-table / coefficient discrepancies", *(f"{v:.3E}" for v in errors))
    print("log2 and exp2 truncation errors", f"{log_tail:.3E}", f"{exp_tail:.3E}")
