#include <time.h>
#include <stdio.h>

int main() {
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);

	printf("%d", time.tv_sec);
	printf("\n%d", time.tv_nsec);
}
