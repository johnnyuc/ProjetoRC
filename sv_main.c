/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

#include "sv_main.h"
#include "sv_tcp.h"
#include "sv_udp.h"
#include "sv_shm.h"

// Global variables
pthread_t tcp_thread_id, udp_thread_id;
multicast_shm_t *multicast_shm;
savedata_t save_data;
int users_size = 0;
user_t *users;

int connected_clients[3];
int num_clients = 0;
int server_fd;

volatile sig_atomic_t flag = 0;

// Handle SIGTSTP
void sigtstp_handler(int signum) {
    printf("\rMulticast groups:\n");
    print_groups(multicast_shm);
}

// Handle SIGINT
void sigint_handler(int signum) {

    // Wait for clients to finish
    for (int i = 0; i < num_clients; i++) {
        wait(NULL);
        printf("Client %d disconnected\n", i+1);
    }

    // Close clients socks
    for (int i = 0; i < num_clients; i++) {
        close(connected_clients[i]);
    }

    // Exiting
    if (pthread_self() != tcp_thread_id && pthread_self() != udp_thread_id)
        printf("\nFinishing server...\n");

    // Close server sock
    close(server_fd);
    exit(0);
}

// Function to open credentials.txt file and read all users data
user_t* load_users(const char* filename, int* size) {
    FILE *file;

    char login[MAX_LEN_LINE];
    char *token; // Tokenizer

    user_t *users; // Pointer to struct array
    int i = 0; // Tokeziner counter

    // Open file
    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error loading user credentials from file %s.\n", filename);
        exit(1);
    }

    // Counts how many logins for malloc size
    while (fgets(login, MAX_LEN_LINE, file)) {
        (*size)++;
    }

    // Mallocs
    users = (user_t*) malloc((*size) * sizeof(user_t));

    // Goes back to beggining
    rewind(file);

    while (fgets(login, MAX_LEN_LINE, file)) {
        // Removes newline from buffer
        char *newline_pos = strchr(login, '\n');
        if (newline_pos != NULL) *newline_pos = '\0';

        token = strtok(login, ";");
        strcpy(users[i].username, token);
        token = strtok(NULL, ";");
        strcpy(users[i].password, token);
        token = strtok(NULL, ";");
        strcpy(users[i].type, token);
        users[i].authenticated = 0;
        i++;
    }

    fclose(file);
    return users;
}

// Function to authenticate an user
int authenticate_user(struct sockaddr_in address, char **args, int size, int protocol) {
    // Verify if user is already authenticated
    for (int i = 0; i < users_size; i++) {
        if (users[i].ipaddr.s_addr == address.sin_addr.s_addr && users[i].port == address.sin_port) {
            if (users[i].authenticated == 1) {
                if (strcmp(args[0], "logout") == 0) users[i].authenticated = 0;
                return 0;
            }
        }
    }

    // Tries to authenticate user
    if (strcmp(args[0], "login") == 0 && size == 3) {
        for (int j = 0; j < users_size; j++) {
            if (strcmp(users[j].username, args[1]) == 0) {
                // Verify if password is correct
                if (strcmp(users[j].password, args[2]) == 0) {
                    if (strcmp(users[j].type, "administrator") == 0) {
                        if (users[j].authenticated == 1) return -1;
                        // Update user's IP address and port
                        users[j].ipaddr = address.sin_addr;
                        users[j].port = address.sin_port;
                        if(protocol == 0){  // UDP
                            users[j].authenticated = 1;
                        }
                        return 2;
                    } else if (strcmp(users[j].type, "journalist") == 0) {
                        // Update user's IP address and port
                        users[j].ipaddr = address.sin_addr;
                        users[j].port = address.sin_port;
                        if(protocol == 1){  // TCP
                            users[j].authenticated = 1;
                        }
                        return 3;
                    } else if (strcmp(users[j].type, "reader") == 0) {
                        // Update user's IP address and port
                        users[j].ipaddr = address.sin_addr;
                        users[j].port = address.sin_port;
                        if (protocol == 1){  // TCP
                            users[j].authenticated = 1;
                        }
                        return 4;
                    }
                }
                return 5;
            }
        }
        return 6;
    }
    return 1;
}

// Function to add command verification
char **command_validation(char *command, int *num_tokens) {
    char **args = (char **) malloc(MAX_CMD_ARGS * sizeof(char *));
    int argc = 0;
    char *token;

    // Splitting the command into arguments using space as the delimiter
    token = strtok(command, " ");

    // Splitter
    while (token != NULL && argc < MAX_CMD_ARGS) {
        // Remove trailing and leading spaces
        size_t len = strcspn(token, " \n\r\t");

        char *arg = (char *) malloc((len + 1) * sizeof(char));
        strncpy(arg, token, len);
        arg[len] = '\0';
        args[argc] = arg;
        argc++;
        token = strtok(NULL, " ");
    }

    // Set the number of tokens
    *num_tokens = argc;

    if (args[0] == NULL) {
        free_args(args, argc);
        return NULL;
    }

    return args;
}

// Function to free command arguments
// Used in command_validation
void free_args(char **args, int argc) {
    for (int i = 0; i < argc; i++) {
        free(args[i]);
    }
    free(args);
}

// Main function
int main(int argc, char const *args[]) {
    // Init sig handlers
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);

    // Verify arguments and set ports
    if (argc < 4) {
        printf("Syntax: ./news_server {PORT_NEWS} {PORT_CONFIG} {FILE_CONFIG}\n");
        exit(EXIT_FAILURE);
    }

    // Store ports
    int news_port = atoi(args[1]);
    int config_port = atoi(args[2]);

    // Verify if ports are valid
    if (news_port < 0 || news_port > 65535 || config_port < 0 || config_port > 65535) {
        printf("Error: The port value must be between 1 and 65535\n");
        exit(EXIT_FAILURE);
    }

    // User loading
    users = load_users(args[3], &users_size);
    strcpy(save_data.str, args[3]);

    // Pthread initialization
    if (pthread_create(&tcp_thread_id, NULL, tcp_thread, &news_port)) {
        perror("Failed to create TCP thread");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&udp_thread_id, NULL, udp_thread, &config_port)) {
        perror("Failed to create UDP thread");
        exit(EXIT_FAILURE);
    }

    // Wait for pthreads
    pthread_join(tcp_thread_id, NULL);
    pthread_join(udp_thread_id, NULL);

    return 0;
}
