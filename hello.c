#include <stdio.h>
#include <unistd.h>

int main() {
	int cnt = 0;
	while (1) {
		printf("%d\n", cnt++);
		sleep(1);
	}

	return 0;
}
