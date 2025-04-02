#include "../include/hyprp.h"

int main(int argc, char *argv[]) {
	Bindings bindings = {};
	init_hypr_bindings(&bindings);

	if (get_current_hypr_bindings(&bindings) == -1) {
		fprintf(stderr, "We failed.\n");
	};
	destroy_hypr_bindings(&bindings);
}
