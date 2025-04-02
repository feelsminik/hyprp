#ifndef HYPRP_H
#define HYPRP_H

#include <ctype.h>
#include <errno.h>
#include <jq.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define NUM_WORKSPACES 9
#define NUM_MONITORS 4
#define INITIAL_CLIENT_SIZE 128

typedef struct {
  int id;
  int monitor_id;
} Workspace;

typedef struct {
  Workspace items[NUM_WORKSPACES];
  size_t count;
} Workspaces;

typedef struct {
  Workspace items[NUM_MONITORS];
  size_t count;
} SeenWorkspaces;

typedef struct {
  int workspace_id;
  char *initial_title;
  char *initial_class;
} Client;

typedef struct {
  Client *items;
  size_t count;
  size_t capacity;
} Clients;

typedef struct {
  Workspaces workspaces;
  SeenWorkspaces seen_workspaces;
  Clients clients;
  int active_workspace_id;
  char *sock_dir;
} Bindings;

typedef struct {
  FILE *r_sock;
  FILE *w_sock;
  int r_sock_fd;
  int w_sock_fd;
} SocketInfo;

/*
 * Errors are indicated by functions return value
 * ERRORS: If successful returns 0, otherwise returns -1
 */

// Initializes bindings struct by dynamically allocating members,
// caller is responsible cleanup by calling destroy_hypr_bindings()
int init_hypr_bindings(Bindings *bindings);

// Takes already initialized bindings struct and frees up all dynamically
// allocated memory
int destroy_hypr_bindings(Bindings *bindings);

// Connects to IPC Unix socket exposed by Hyprland and fills up bindings by
// writing and reading socket
int get_current_hypr_bindings(Bindings *bindings);

// Connects to IPC Unix socket and exposes subscribed events by event_mask as a
// file stream
int get_current_hypr_events(FILE **stream, int event_mask);

#endif
