/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

# ifndef SV_SHM_H
# define SV_SHM_H

#include <sys/shm.h> // Shared memory [do not remove]

// Includes
#include "sv_main.h"

// Defines
#define MAX_GROUPS 1024

// Structs
typedef struct multicast_group_t {
    char id[MAX_LEN_CREDS];
    char topic[MAX_LEN_CREDS];
    char ip[16];
    int port;
} multicast_group_t;

// Shared memory worker main struct
typedef struct multicast_shm_t {
    pthread_mutex_t mutex;
    multicast_group_t multicastgroup[MAX_GROUPS];
    int shmid;
} multicast_shm_t;

// Function prototypes
multicast_shm_t *create_multicast_shm();
multicast_shm_t* attach_multicast_shm(int shmid);
void detach_multicast_shm(multicast_shm_t* shm_ptr);
void destroy_multicast_shm(multicast_shm_t* shm_ptr);

// Shared memory functions
void print_groups(multicast_shm_t *multicast_shm);
int group_exists(multicast_shm_t *multicast_shm, const char *id, const char *ip, int port);
int group_index(multicast_shm_t *multicast_shm, const char *id);
void add_group(multicast_shm_t *multicast_shm, const char *id, const char *topic, const char *ip, int port);
void remove_group(multicast_shm_t *multicast_shm, const char *id);

# endif // SV_SHM_H