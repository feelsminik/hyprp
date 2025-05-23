#include "../include/hyprp.h"
#include <string.h>

// defined in sys/un.h
#define MAX_SUN_PATH 108
#define DELIMITERS " \n\t"
#define BUF_SIZE 1024
// exclude space as parsed items count as a single token
#define ITEM_DELIMITERS "\n\t"

// clang-format off
static Symbol symbols[] = {
	{.client = "firefox",      .icon = ""},
	{.client = "obsidian",     .icon = "󰎚"},
	{.client = "wezterm",      .icon = ""},
	{.client = "youtube",      .icon = "󰝚"},
	{.client = "discord",      .icon = ""},
	{.client = "zathura",      .icon = ""},
	{.client = "pavucontrol",  .icon = "󰓃"},
};
// clang-format on

static size_t symbols_len() {
	return sizeof(symbols) / sizeof(symbols[0]);
}

typedef int (*ParserFunc)(char *, Bindings *);
typedef int (*UpdateBindingFunc)(Bindings *);
typedef int (*ReadSocketFunc)(FILE *, Bindings *);

static int str2int(int *out, char *s, int base) {
	char *end;
	if (s[0] == '\0' || isspace(s[0])) return -1;
	errno = 0;
	long l = strtol(s, &end, base);
	/* Both checks are needed because INT_MAX == LONG_MAX is possible. */
	if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX)) return -1;
	if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN)) return -1;
	if (*end != '\0') return -1;
	*out = l;
	return 0;
}

static bool str_prefix(const char *pre, const char *word) {
	return (strncmp(pre, word, strlen(pre)) == 0);
}

static int sort_workspaces(const void *w1_p, const void *w2_p) {
	Workspace w1 = *(const Workspace *)w1_p;
	Workspace w2 = *(const Workspace *)w2_p;

	int order = 0;
	if (w1.id < 0) {
		return 1;
	} else if (w2.id < 0) {
		return -1;
	}

	if (w1.id != w2.id) {
		order = w1.id - w2.id;
	} else {
		order = w1.id - w2.id;
	}

	return order;
}

static int get_socket_dir(Bindings *binds) {
	const char *hypr_sig = getenv("HYPRLAND_INSTANCE_SIGNATURE");
	if (hypr_sig == NULL) return -1;
	const char *xdg_path = getenv("XDG_RUNTIME_DIR");
	if (xdg_path == NULL) return -1;
	const char *hypr_folder = "/hypr/";

	size_t sock_dir_len =
		strlen(hypr_sig) + strlen(xdg_path) + strlen(hypr_folder) + 1;
	binds->sock_dir = malloc(sock_dir_len);
	if (binds->sock_dir == NULL) return -1;

	if (snprintf(binds->sock_dir, sock_dir_len, "%s%s%s", xdg_path, hypr_folder,
	             hypr_sig) < 0) {
		return -1;
	}
	return 0;
}

static int con_hypr_sock(const char *sock_dir, SocketInfo *streams,
                         const char *sock_name) {
	streams->r_sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (streams->r_sock_fd == -1) return -1;


	struct sockaddr_un srv_addr = {0};
	srv_addr.sun_family = AF_UNIX;
	int error =
		snprintf(srv_addr.sun_path, MAX_SUN_PATH, "%s/%s", sock_dir, sock_name);
	if (error == -1) return -1;


	int status = connect(streams->r_sock_fd, (struct sockaddr *)&srv_addr,
	                     sizeof(srv_addr));
	if (status == -1) return -1;

	return 0;
}

int init_hypr_bindings(Bindings *bindings) {
	if (bindings == NULL) return -1;

	bindings->workspaces.count = 0;
	bindings->seen_workspaces.count = 0;
	Workspace empty_workspace = {.id = 0, .monitor_id = 0};
	for (size_t i = 1; i <= NUM_WORKSPACES; i++) {
		if (i <= NUM_REGULAR_WORKSPACES) {
			Workspace workspace = {.id = i, .monitor_id = 0};
			bindings->workspaces.items[i] = workspace;
		} else {
			bindings->workspaces.items[i] = empty_workspace;
		}
	}

	for (size_t i = 0; i < NUM_MONITORS; i++) {
		bindings->seen_workspaces.items[i] = empty_workspace;
	}

	Clients clients = {.count = 0, .capacity = INITIAL_CLIENTS_SIZE};
	bindings->clients = clients;

	bindings->active_workspace_id = 0;

	if (get_socket_dir(bindings) == -1) return -1;

	return 0;
}

int destroy_hypr_bindings(Bindings *bindings) {
	free(bindings->sock_dir);
	return 0;
}

static int parse_bindings(FILE *read_sock, Bindings *bindings,
                          ParserFunc parser_func,
                          UpdateBindingFunc update_binding) {
	char buffer[BUF_SIZE] = {0};
	while (fgets(buffer, sizeof(buffer), read_sock) != NULL) {
		for (char *token = strtok(buffer, DELIMITERS); token != NULL;
		     token = strtok(NULL, DELIMITERS)) {
			parser_func(token, bindings);
		}
		if (strlen(buffer) == 1 && buffer[0] == '\n') {
			if (update_binding != NULL) update_binding(bindings);
		}
	}
	if (ferror(read_sock) != 0) return -1;
	return 0;
}

static int parse_client_item(char *client_item) {
	char *next_token = strtok(NULL, ITEM_DELIMITERS);
	if (next_token == NULL) {
		next_token = "";
	}
	strncpy(client_item, next_token, CLIENT_ITEM_LEN);
	client_item[CLIENT_ITEM_LEN - 1] = '\0';
	return 0;
}

static int toLower(char *str) {
	for (int i = 0; str[i]; i++) {
		str[i] = tolower(str[i]);
	}
	return 0;
}

static char *parse_client_item_icon(char *client_item) {
	toLower(client_item);
	for (size_t i = 0; i < symbols_len(); i++) {
		if (strstr(client_item, symbols[i].client) != NULL) {
			return symbols[i].icon;
		}
	}
	return "";
}

static int parse_clients(char *token, Bindings *bindings) {
	size_t count = bindings->clients.count;
	if (count >= (INITIAL_CLIENTS_SIZE - 1)) return -1;
	if (strncmp(token, "initialClass:", BUF_SIZE) == 0) {
		char *initial_class = bindings->clients.items[count].initial_class;
		if (parse_client_item(initial_class) == -1) return -1;
		bindings->clients.items[count].icon =
			parse_client_item_icon(initial_class);
	} else if (strncmp(token, "initialTitle:", BUF_SIZE) == 0) {
		char *initial_title = bindings->clients.items[count].initial_title;
		if (parse_client_item(initial_title) == -1) return -1;
		bindings->clients.items[count].icon =
			parse_client_item_icon(initial_title);
	} else if (strncmp(token, "workspace:", BUF_SIZE) == 0) {
		int monitor_id = 0;
		if (str2int(&monitor_id, strtok(NULL, DELIMITERS), 10) == -1) return -1;
		bindings->clients.items[count].workspace_id = monitor_id;
	}
	return 0;
}

static int parse_workspaces(char *token, Bindings *bindings) {
	size_t count = bindings->workspaces.count;
	if (strncmp(token, "ID", BUF_SIZE) == 0) {
		int id = 0;
		if (str2int(&id, strtok(NULL, DELIMITERS), 10) == -1) return -1;
		bindings->workspaces.items[count].id = id;
	} else if (strncmp(token, "monitorID:", BUF_SIZE) == 0) {
		int monitor_id = 0;
		if (str2int(&monitor_id, strtok(NULL, DELIMITERS), 10) == -1) return -1;
		bindings->workspaces.items[count].monitor_id = monitor_id;
	}
	return 0;
}

static int parse_seen_workspaces(char *token, Bindings *bindings) {
	size_t count = bindings->seen_workspaces.count;
	// 'workspace:' will match for 'active workspace' and 'special workpace:'
	if (strncmp(token, "workspace:", BUF_SIZE) == 0) {
		int id = 0;
		if (str2int(&id, strtok(NULL, DELIMITERS), 10) == -1) return -1;
		if (id > 0) { // normal workspace
			bindings->seen_workspaces.items[count].id = id;
		}
	} else if (strncmp(token, "ID", BUF_SIZE) == 0) {
		int monitor_id = 0;
		if (str2int(&monitor_id, strtok(NULL, DELIMITERS), 10) == -1) return -1;
		bindings->seen_workspaces.items[count].monitor_id = monitor_id;
	}
	return 0;
}

static int parse_active_workspace(char *token, Bindings *bindings) {
	if (strncmp(token, "ID", BUF_SIZE) == 0) {
		int id = 0;
		if (str2int(&id, strtok(NULL, DELIMITERS), 10) == -1) return -1;
		bindings->active_workspace_id = id;
	}
	return 0;
}

static int increase_workspace_count(Bindings *bindings) {
	bindings->workspaces.count++;
	return 0;
}

static int read_workspaces(FILE *read_sock, Bindings *binds) {
	return parse_bindings(read_sock, binds, parse_workspaces,
	                      increase_workspace_count);
}

static int read_active_workspace(FILE *read_sock, Bindings *bindings) {
	return parse_bindings(read_sock, bindings, parse_active_workspace, NULL);
}

static int increase_client_count(Bindings *bindings) {
	bindings->clients.count++;
	return 0;
}

static int read_clients(FILE *read_sock, Bindings *bindings) {
	return parse_bindings(read_sock, bindings, parse_clients,
	                      increase_client_count);
}

static int increase_seen_workspace_count(Bindings *bindings) {
	bindings->seen_workspaces.count++;
	return 0;
}

static int read_seen_workspaces(FILE *read_sock, Bindings *bindings) {
	return parse_bindings(read_sock, bindings, parse_seen_workspaces,
	                      increase_seen_workspace_count);
}

static int send_sock_msg(FILE *w_sock, Bindings *bindings,
                         const char *message) {
	if (fprintf(w_sock, "%s", message) < 0) return -1;
	if (fflush(w_sock) == EOF) return -1;
	return 0;
}

static int open_socket(SocketInfo *sock_info, Bindings *binds) {
	sock_info->r_sock_fd = 0;
	if (con_hypr_sock(binds->sock_dir, sock_info, ".socket.sock") == -1)
		return -1;

	sock_info->w_sock_fd = dup(sock_info->r_sock_fd);
	if (sock_info->w_sock_fd == -1) return -1;

	sock_info->r_sock = fdopen(sock_info->r_sock_fd, "r");
	if (sock_info->r_sock == NULL) return -1;

	sock_info->w_sock = fdopen(sock_info->w_sock_fd, "w");
	if (sock_info->w_sock == NULL) return -1;

	return 0;
}

static int open_socket2(SocketInfo *sock_info, Bindings *binds) {
	sock_info->r_sock_fd = 0;
	if (con_hypr_sock(binds->sock_dir, sock_info, ".socket2.sock") == -1)
		return -1;

	sock_info->r_sock = fdopen(sock_info->r_sock_fd, "r");
	if (sock_info->r_sock == NULL) return -1;

	return 0;
}

static int close_socket(SocketInfo *sock_info) {
	if (fclose(sock_info->r_sock) == EOF) return -1;
	if (fclose(sock_info->w_sock) == EOF) return -1;
	return 0;
}

static int get_hypr_binding(Bindings *binds, ReadSocketFunc read_socket,
                            const char *message) {
	SocketInfo sock_info = {};
	if (open_socket(&sock_info, binds) == -1) return -1;
	if (send_sock_msg(sock_info.w_sock, binds, message) == -1) return -1;
	if (read_socket(sock_info.r_sock, binds) == -1) return -1;
	if (close_socket(&sock_info)) return -1;

	return 0;
}

int get_hypr_bindings(Bindings *binds) {
	int exit;
	exit = get_hypr_binding(binds, read_clients, "/clients");
	if (exit == -1) return -1;
	exit = get_hypr_binding(binds, read_workspaces, "/workspaces");
	if (exit == -1) return -1;
	qsort(binds->workspaces.items, binds->workspaces.count, sizeof(Workspace),
	      sort_workspaces);
	exit = get_hypr_binding(binds, read_active_workspace, "/activeworkspace");
	if (exit == -1) return -1;
	exit = get_hypr_binding(binds, read_seen_workspaces, "/monitors");
	if (exit == -1) return -1;

	return 0;
}


static void match_events(const char *buffer, FILE *write_socket,
                         long events_mask) {
	if ((events_mask & ACTIVEWINDOW_EVENT) &&
	    str_prefix(ACTIVEWINDOW_PREFIX, buffer)) {
		fprintf(write_socket, "%s", buffer);
		return;
	}
	if ((events_mask & WINDOWTITLE_EVENT) &&
	    str_prefix(WINDOWTITLE_PREFIX, buffer)) {
		fprintf(write_socket, "%s", buffer);
		return;
	}
	if ((events_mask & FOCUSEDMON_EVENT) &&
	    str_prefix(FOCUSEDMON_PREFIX, buffer)) {
		fprintf(write_socket, "%s", buffer);
		return;
	}
	if ((events_mask & OPENWINDOW_EVENT) &&
	    str_prefix(OPENWINDOW_PREFIX, buffer)) {
		fprintf(write_socket, "%s", buffer);
		return;
	}
	if ((events_mask & WORKSPACE_EVENT) &&
	    str_prefix(WORKSPACE_PREFIX, buffer)) {
		fprintf(write_socket, "%s", buffer);
		return;
	}
}

static void *filter_events(void *arg) {
	ThreadData *data = (ThreadData *)arg;
	long events_mask = data->events_mask;
	char buffer[1024] = {0};

	FILE *hypr_socket = fdopen(data->hypr_r_socket_fd, "r");
	FILE *write_socket = fdopen(data->write_socket_fd, "w");
	while (fgets(buffer, sizeof(buffer), hypr_socket) != NULL) {
		match_events(buffer, write_socket, events_mask);
		fflush(write_socket);
	}
	if (ferror(hypr_socket) != 0) return (void *)-1;
	free(data);
	return NULL;
}

int get_hypr_event_stream(int *stream_fd, Bindings *binds, long events_mask) {
	int pipefd[2];
	if (pipe(pipefd) == -1) return -1;
	*stream_fd = pipefd[0];

	SocketInfo sock_info = {};
	if (get_socket_dir(binds) == -1) return -1;
	if (open_socket2(&sock_info, binds) == -1) return -1;

	ThreadData *io = malloc(sizeof(ThreadData));
	if (io == NULL) return -1;
	io->hypr_r_socket_fd = sock_info.r_sock_fd;
	io->write_socket_fd = pipefd[1];
	io->events_mask = events_mask;

	pthread_t thread;
	if (pthread_create(&thread, NULL, filter_events, io) != 0) return -1;

	return 0;
}
