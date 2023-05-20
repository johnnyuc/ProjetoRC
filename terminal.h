/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

# ifndef _TERMINAL_H_
# define _TERMINAL_H_

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>

// Defines
#define h_addr h_addr_list[0]
#define STANDARD_LEN 128
#define INPUT_LEN 32
#define NEWS_LEN 1024
#define MAX_GROUPS 32

// Structs
typedef struct multicast_group_t {
    char topic[STANDARD_LEN];
    char ip[16];
    int port;
} multicast_group_t;

# endif // _TERMINAL_H_