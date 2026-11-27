# SPDX-License-Identifier: MIT
# Copyright (c) 2026 dIHlHIb
# Developed with AI assistance.
CC ?= cc
CFLAGS ?= -O3
FPFLAGS = -std=gnu11 -frounding-math -ffp-contract=off -fno-math-errno

all: librootnf.a

rootnf.o: rootnf.c rootnf.h rootnf_tables.h exact_table.h power_interval.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FPFLAGS) -DCORE_MATH_SUPPORT_ERRNO -fPIC -c $< -o $@

librootnf.a: rootnf.o
	$(AR) rcs $@ $^

check_special: check_special.c librootnf.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FPFLAGS) $< librootnf.a $(LDFLAGS) -lmpfr -lgmp -lm -o $@

.PHONY: all
