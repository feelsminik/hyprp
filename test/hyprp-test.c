#include "../include/hyprp.h"

// static void test_get_bindings(void) {
// 	Bindings bindings = {};
// 	init_hypr_bindings(&bindings);

// 	if (get_current_hypr_bindings(&bindings) == -1) {
// 		fprintf(stderr, "We failed.\n");
// 	};
// 	fprintf(stderr, "%s\n", bindings.clients.items[0].initial_class);
// 	destroy_hypr_bindings(&bindings);
// }

static void test_get_events(void) {
	Bindings bindings = {};
	init_hypr_bindings(&bindings);

	AsyncIO io = {};
	if (get_hypr_event_stream(&io, &bindings, 0) == -1) {
		fprintf(stderr, "We failed.\n");
	};

	char buffer[1024] = {0};
	FILE *read_socket = fdopen(io.read_socket_fd, "r");
	while (fgets(buffer, sizeof(buffer), read_socket) != NULL) {
		fprintf(stderr, "%s", buffer);
	}

	// TODO close sockets

	destroy_hypr_bindings(&bindings);
}

int main(int argc, char *argv[]) {
	test_get_events();
}
