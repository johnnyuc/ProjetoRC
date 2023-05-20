/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

# ifndef SV_MAIN_H
# define SV_MAIN_H

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>

// Defines
#define NEWS_BUF 1024
#define MAX_LEN_LINE 128
#define MAX_LEN_CREDS 32
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

typedef struct savedata_t {
    char str[100];
} savedata_t;

// Function prototypes
user_t* load_users(const char* filename, int* size);
int authenticate_user(struct sockaddr_in address, char **args, int size, int protocol);
char **command_validation(char *command, int *num_tokens);
void free_args(char **args, int argc);

# endif // SV_MAIN_H