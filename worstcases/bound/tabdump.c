// Dump the dualscan log2 table (built by the identical dd_log2 call) as raw doubles.
#include <stdio.h>
#include <stdlib.h>
#include "ddlib.h"
int main(int argc, char **argv) {
  size_t n = (size_t) 1 << 24;
  double *Th = malloc(n * 8), *Tl = malloc(n * 8);
  for (size_t i = 0; i < n; i++) dd_log2(1.0 + (double) i * 0x1p-24, &Th[i], &Tl[i]);
  FILE *f = fopen(argv[1], "wb");
  fwrite(Th, 8, n, f);
  fwrite(Tl, 8, n, f);
  fclose(f);
  return 0;
}
