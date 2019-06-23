#define _GNU_SOURCE
#include <time.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

int64_t timespec_diff_usec(struct timespec const* start, struct timespec const* end) {
     int64_t ret = 0;
     ret = end->tv_sec - start->tv_sec;
     ret *= 1000000;
     ret += (end->tv_nsec - start->tv_nsec)/1000;
     return ret;
}

void realloc_benchmark(unsigned L) {
     unsigned int *v = NULL;
     struct timespec start, end;
     int64_t elapsed;
     if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
         perror("clock_gettime");
         exit(1);
     }
     for (unsigned i = 0; i < L; i++) {
          v = realloc(v, sizeof(i)*(i+1));
          if (!v) {
             perror("realloc");
             exit(1);
          }
          v[i] = i;
     }
     if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
         perror("clock_gettime");
         exit(1);
     }
     elapsed = timespec_diff_usec(&start, &end);
     printf("%u reallocs in %" PRId64 " usec\n", L, elapsed);
}

int main(int argc, char** argv) {
    unsigned L = 0;
    struct timespec res;
    if (argc >= 2) {
       L = atoi(argv[1]);
    }
    if (0 == L) {
        L = 1U << 20;
    }
    if (clock_getres(CLOCK_MONOTONIC, &res) < 0) {
        perror("clock_getres");
	exit(1);
    }
    printf("Using CLOCK_MONOTONIC, resolution: %ld nsec\n", res.tv_nsec);
    realloc_benchmark(L);
    return 0;
}
