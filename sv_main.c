/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

#include "sv_main.h"
#include "sv_tcp.h"
#include "sv_udp.h"

// Global variables
user_t *users;
int users_size = 0;
savedata_t save_data;

// Function to open credentials.txt file and read all users data
user_t* load_users(const char* filename, int* size) {
    FILE *file;

    char login[MAX_LEN_LINE];
    char *token; // Tokenizer

    user_t *users; // Pointer to struct array
    int i = 0; // Tokeziner counter

    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Erro ao carregar as credenciais de utilizador.\n");
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
    if (strcmp(args[0], "login") == 0) {
        for (int j = 0; j < users_size; j++) {
            if (strcmp(users[j].username, args[1]) == 0) {
                // Verify if password is correct
                if (strcmp(users[j].password, args[2]) == 0) {
                    if (strcmp(users[j].type, "administrator") == 0) {
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

    // Print whole array
    for (int i = 0; i < users_size; i++) {
        printf("%s %s %s %d %s %d\n", users[i].username, users[i].password, users[i].type, users[i].authenticated, inet_ntoa(users[i].ipaddr), users[i].port);
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

int main(int argc, char const *args[]) {
    // Verify arguments and set ports
    if (argc < 4) {
        printf("Sintaxe: ./news_server {PORTO_NOTICIAS} {PORTO_CONFIG} {FICHEIRO_CONFIG}\n");
        exit(EXIT_FAILURE);
    }
    
    // Store ports
    int news_port = atoi(args[1]);
    int config_port = atoi(args[2]);

    // Verify if ports are valid
    if (news_port < 0 || news_port > 65535 || config_port < 0 || config_port > 65535) {
        printf("Erro: o valor das portas deverá estar entre 1 e 65535\n");
        exit(EXIT_FAILURE);
    }

    // User loading
    users = load_users(args[3], &users_size);
    strcpy(save_data.str, args[3]);

    // Opening threads
    pthread_t tcp_thread_id, udp_thread_id;

    // Pthread initialization
    if (pthread_create(&tcp_thread_id, NULL, tcp_thread, &news_port)) {
        perror("Falha a criar thread TCP");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&udp_thread_id, NULL, udp_thread, &config_port)) {
        perror("Falha a criar thread UDP");
        exit(EXIT_FAILURE);
    }

    // Wait for pthreads
    pthread_join(tcp_thread_id, NULL);
    pthread_join(udp_thread_id, NULL);

    return 0;
}