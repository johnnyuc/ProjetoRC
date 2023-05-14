/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

# ifndef _TERMINAL_H_
# define _TERMINAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

// Defines
#define BUF_SIZE 1024
#define MAX_LEN_LINE 128
#define MAX_GROUPS 32
#define MAX_ARGS 3
#define h_addr h_addr_list[0]

// Structs
typedef struct multicast_group_t {
    char topic[BUF_SIZE];
    char ip[16];
    int port;
} multicast_group_t;

# endif // _TERMINAL_H_