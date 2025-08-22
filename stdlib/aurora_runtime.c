#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int64_t print_i64(int64_t x) { printf("%lld\n", (long long)x); return x; }
int64_t read_i64(void) { long long x; if (scanf("%lld", &x)!=1) return 0; return (int64_t)x; }
/* malloc/free are imported from libc */
