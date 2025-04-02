#include "../include/hyprp.h"

static void test_get_bindings(void) {
	Bindings bindings = {};
	init_hypr_bindings(&bindings);

	if (get_hypr_bindings(&bindings) == -1) {
		fprintf(stderr, "We failed.\n");
	};
	for (size_t i = 0; i < bindings.workspaces.count; i++) {
		fprintf(stderr, "%d\n", bindings.workspaces.items[i].id);
	}
	for (size_t i = 0; i < bindings.clients.count; i++) {
		fprintf(stderr, "Workspace Id: %d\n",
		        bindings.clients.items[i].workspace_id);
		fprintf(stderr, "Class: %s\n", bindings.clients.items[i].initial_class);
	}
	destroy_hypr_bindings(&bindings);
}

static void test_get_events(void) {
	Bindings bindings = {};
	init_hypr_bindings(&bindings);

	int stream_fd;
	if (get_hypr_event_stream(&stream_fd, &bindings,
	                          WORKSPACE_EVENT | FOCUSEDMON_EVENT |
	                              OPENWINDOW_EVENT) == -1) {
		fprintf(stderr, "We failed.\n");
	};

	char buffer[1024] = {0};
	FILE *read_socket = fdopen(stream_fd, "r");
	while (fgets(buffer, sizeof(buffer), read_socket) != NULL) {
		fprintf(stderr, "%s", buffer);
	}

	// TODO close sockets

	destroy_hypr_bindings(&bindings);
}

int main(int argc, char *argv[]) {
	// test_get_events();
	test_get_bindings();
}
