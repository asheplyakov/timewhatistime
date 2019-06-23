#define _GNU_SOURCE
#include <time.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    struct timespec ts;
    for (int i = 0; i < 1000000; i++) {
        asm volatile("": : :"memory");
	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
            exit(1);
        }
    }
    return 0;
}
