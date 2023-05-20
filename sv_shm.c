
/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

#include "sv_main.h"
#include "sv_shm.h"

// Function to create multicast shared memory
multicast_shm_t *create_multicast_shm() {
    int shmid;
    multicast_shm_t *shm;
    
    // Create the shared memory segment
    if ((shmid = shmget(IPC_PRIVATE, sizeof(multicast_shm_t), IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach to the shared memory segment
    if ((shm = shmat(shmid, NULL, 0)) == (multicast_shm_t *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Initialize the shared memory
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shm->mutex, &mutex_attr);
    shm->shmid = shmid;
    for (int i = 0; i < MAX_GROUPS; i++) {
        memset(&shm->multicastgroup[i], 0, sizeof(multicast_group_t));
    }
    
    return shm;
}

// Function to attach to multicast shared memory
multicast_shm_t* attach_multicast_shm(int shmid) {
    multicast_shm_t* shm_ptr = shmat(shmid, NULL, 0);
    if (shm_ptr == (void*) -1) {
        perror("shmat failed");
        return NULL;
    }
    return shm_ptr;
}

// Function to detach from multicast shared memory
void detach_multicast_shm(multicast_shm_t* shm_ptr) {
    if (shmdt(shm_ptr) == -1) {
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }
}

// Function to destroy multicast shared memory
void destroy_multicast_shm(multicast_shm_t* shm_ptr) {
    int storedShmid = shm_ptr->shmid;

    detach_multicast_shm(shm_ptr);

    if (shmctl(storedShmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
        exit(EXIT_FAILURE);
    }
}

// Function to print all groups in the shared memory
void print_groups(multicast_shm_t *multicast_shm) {
    pthread_mutex_lock(&multicast_shm->mutex);
    printf("\n");
    for (int i = 0; i < MAX_GROUPS; i++) {
        if (strcmp(multicast_shm->multicastgroup[i].id, "") != 0) {
            printf("ID: %s Topic: %s Multicast IP: %s Multicast port: %d\n", 
            multicast_shm->multicastgroup[i].id, multicast_shm->multicastgroup[i].topic, 
            multicast_shm->multicastgroup[i].ip, multicast_shm->multicastgroup[i].port);
        }
    }
    pthread_mutex_unlock(&multicast_shm->mutex);
}

// Function to check if a group already exists in the shared memory
int group_exists(multicast_shm_t *multicast_shm, const char *id, const char *ip, int port) {
    pthread_mutex_lock(&multicast_shm->mutex);
    // Finds if either id or (ip and port) are already in the shared memory
    for (int i = 0; i < MAX_GROUPS; i++) {
        if (strcmp(multicast_shm->multicastgroup[i].id, id) == 0) {
            pthread_mutex_unlock(&multicast_shm->mutex);
            return 1; // Found match with id
        } else if (strcmp(multicast_shm->multicastgroup[i].ip, ip) == 0 && multicast_shm->multicastgroup[i].port == port) {
            pthread_mutex_unlock(&multicast_shm->mutex);
            return 2; // Found match with ip and port
        }
    }
    pthread_mutex_unlock(&multicast_shm->mutex);
    return 0; // No match found
}

// Function to check if a group already exists in the shared memory and returns its index
int group_index(multicast_shm_t *multicast_shm, const char *id) {
    pthread_mutex_lock(&multicast_shm->mutex);
    // Finds if either id or (ip and port) are already in the shared memory
    for (int i = 0; i < MAX_GROUPS; i++) {
        if (strcmp(multicast_shm->multicastgroup[i].id, id) == 0) {
            pthread_mutex_unlock(&multicast_shm->mutex);
            return i; // Found match, returns index
        }
    }
    pthread_mutex_unlock(&multicast_shm->mutex);
    return -1; // No match found
}

// Function to add a group to the shared memory
void add_group(multicast_shm_t *multicast_shm, const char *id, const char *topic, const char *ip, int port) {
    pthread_mutex_lock(&multicast_shm->mutex);
    for (int i = 0; i < MAX_GROUPS; i++)
        if (strcmp(multicast_shm->multicastgroup[i].id, id) == 0) 
            break;

    for (int i = 0; i < MAX_GROUPS; i++) {
        if (strcmp(multicast_shm->multicastgroup[i].id, "") == 0) {
            strcpy(multicast_shm->multicastgroup[i].id, id);
            strcpy(multicast_shm->multicastgroup[i].topic, topic);
            strcpy(multicast_shm->multicastgroup[i].ip, ip);
            multicast_shm->multicastgroup[i].port = port;
            break;
        }
    }
    pthread_mutex_unlock(&multicast_shm->mutex);
}

// Function to remove a group from the shared memory
void remove_group(multicast_shm_t *multicast_shm, const char *id) {
    pthread_mutex_lock(&multicast_shm->mutex);
    for (int i = 0; i < MAX_GROUPS; i++) {
        if (strcmp(multicast_shm->multicastgroup[i].id, id) == 0) {
            memset(&multicast_shm->multicastgroup[i], 0, sizeof(multicast_group_t));
            break;
        }
    }
    pthread_mutex_unlock(&multicast_shm->mutex);
}