// LEI UC 2022-2023 // Network communications (RC)
// Johnny Fernandes 2021190668, João Macedo 2021220627

#ifndef NEWS_PROJECT_SERVER_H
#define NEWS_PROJECT_SERVER_H

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Defines
#define MAX_LEN_CREDS 32
#define MAX_LEN_LINE 128
#define MAX_CMD_ARGS 5

// Structs
typedef struct user_t {
  char username[MAX_LEN_CREDS];
  char password[MAX_LEN_CREDS];
  char type[MAX_LEN_CREDS];
  int authenticated;
  struct in_addr ipaddr;
  uint16_t port;
} user_t;

typedef struct tcp_client_t {
    int socket;
    struct sockaddr_in address;
} tcp_client_t;

#endif //NEWS_PROJECT_SERVER_H