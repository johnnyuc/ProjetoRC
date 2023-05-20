/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

# ifndef SV_UDP_H
# define SV_UDP_H

// Includes
#include "sv_main.h"

// Defines
#define MAX_CLIENTS 10

// Structs
struct ConnectedClient {
    char ip[INET_ADDRSTRLEN];
    int port;
};

struct ConnectedClient connected_clients_udp[MAX_CLIENTS];

// Function prototypes
void *udp_thread(void *arg);
void addClient(const char* ip, int port);
void removeClient(const char* ip, int port);
int isClientInList(const char* ip, int port);
void save_users();
void add_user(const char *username, const char *password, const char *type);
int remove_user(const char *username);



# endif // SV_UDP_H