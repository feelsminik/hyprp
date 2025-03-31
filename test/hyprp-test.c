#include "../include/hyprp.h"
#include <stdio.h>

void test_init(void) {
	fprintf(stderr, "Initializing tests...\n");
}

int main(int argc, char *argv[]) {
	foo();
	test_init();
}
