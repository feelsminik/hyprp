#ifndef HYPRP_H
#define HYPRP_H

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// number of workspaces includes special workspaces
#define NUM_WORKSPACES 16
#define NUM_REGULAR_WORKSPACES 9
#define NUM_SPECIAL_WORKSPACES (NUM_WORKSPACES - NUM_REGULAR_WORKSPACES)

#define NUM_MONITORS 4

// clang-format off
#define ACTIVEWINDOW_EVENT    0b0000000000000001
#define WINDOWTITLE_EVENT     0b0000000000000010
#define FOCUSEDMON_EVENT      0b0000000000000100
#define OPENWINDOW_EVENT      0b0000000000001000
#define WORKSPACE_EVENT       0b0000000000010000
#define ALL_EVENTS            0b1111111111111111

#define ACTIVEWINDOW_PREFIX   "activewindow>>"
#define WINDOWTITLE_PREFIX    "windowtitle>>"
#define FOCUSEDMON_PREFIX     "focusedmon>>"
#define OPENWINDOW_PREFIX     "openwindow>>"
#define WORKSPACE_PREFIX      "workspace>>"

#define CLIENT_ITEM_LEN 64
#define INITIAL_CLIENTS_SIZE 128
// clang-format on

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
  const char *client;
  char *icon;
} Symbol;

typedef struct {
  int workspace_id;
  char initial_title[CLIENT_ITEM_LEN];
  char initial_class[CLIENT_ITEM_LEN];
  char *icon;
} Client;

typedef struct {
  Client items[INITIAL_CLIENTS_SIZE];
  size_t count;
  size_t capacity;
} Clients;

typedef struct {
  int hypr_r_socket_fd;
  int write_socket_fd;
  long events_mask;
} ThreadData;

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

/*
 * Undefined behaviour if functions (besides init_hypr_bindings()) are called
 * with bindings, which haven't been initialized using init_hypr_bindings()
 */

// Initializes bindings struct by dynamically allocating members,
// The caller is responsible cleanup by calling destroy_hypr_bindings()
int init_hypr_bindings(Bindings *bindings);

// Takes already initialized bindings struct and frees up all dynamically
// allocated memory
int destroy_hypr_bindings(Bindings *bindings);

// Connects to IPC Unix socket exposed by Hyprland and fills up bindings by
// writing and reading socket
int get_hypr_bindings(Bindings *bindings);

// Connects to IPC Unix socket and exposes subscribed events by event_mask as a
// file stream
// The caller is responible for closing the stream
// Subscribe to selected events only by specifing creating a bitmask
// If all events should be emitted supply 0 instead.
int get_hypr_event_stream(int *stream_fd, Bindings *binds, long event_mask);

#endif
