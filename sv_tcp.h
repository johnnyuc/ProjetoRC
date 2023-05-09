/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

# ifndef SV_TCP_H
# define SV_TCP_H

// Includes
#include <arpa/inet.h>

// Structs
typedef struct tcp_client_t {
    int socket;
    struct sockaddr_in address;
} tcp_client_t;

// Function prototypes
void *tcp_thread(void *arg);
void handle_tcp(int new_socket, int server_fd, struct sockaddr_in address);

# endif // SV_TCP_H