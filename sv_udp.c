/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

#include "sv_main.h"
#include "sv_udp.h"
#include <fcntl.h>

// Global variables [inside sv_main.c]
extern volatile sig_atomic_t flag;
extern user_t *users;
extern int users_size;
extern savedata_t save_data;
int server_socket;
int clients_udp = 0;

// UDP Thread
void *udp_thread(void *arg) {
    int port = *(int *) arg; // Port passed as argument

    int client_sockets[MAX_CLIENTS], max_clients = MAX_CLIENTS;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    client_len = sizeof(client_addr);

    int valread;
    char incoming[MAX_LEN_LINE];
    char outgoing[MAX_LEN_LINE];

    // Socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {
        perror("UDP socket failed");
        exit(EXIT_FAILURE);
    }

    // Address assignment
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port); // Server Port

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("UDP binding failed");
        exit(EXIT_FAILURE);
    }

    printf("UDP server listening on port %d\n", port);

    while (!flag) {
        memset(incoming, 0, sizeof(incoming)); // Clean buffer
        memset(outgoing, 0, sizeof(outgoing)); // Clean buffer
        if((recvfrom(server_socket, incoming, MAX_LEN_LINE, 0, (struct sockaddr *)&client_addr, &client_len)) == -1){
            perror("Error in recvfrom\n");
        }
        if (strcmp(inet_ntoa(client_addr.sin_addr), "0.0.0.0") != 0 && ntohs(client_addr.sin_port) != 0) {
            if (!isClientInList(inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port))) {
                printf("UDP client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                addClient(inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                memset(outgoing, 0, sizeof(outgoing));
                snprintf(outgoing, MAX_LEN_LINE, "\nlogin {username} {password}: ");
                if (sendto(server_socket, outgoing, MAX_LEN_LINE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) printf("Error in sendto");
            }

            // Check if message is from an authenticated user
            int argc = 0;
            char **args = command_validation(incoming, &argc);
            if (args == NULL) continue;

            if (strcmp(args[0], "X") == 0  || strcmp(args[0], "Xser") == 0) continue; // Ignore verbose UDP startup from netcat
            int try_auth = authenticate_user(client_addr, args, argc, 0);
            int aux = 0; // Auxiliar variable

            memset(outgoing, 0, sizeof(outgoing)); // Clean buffer
            switch (try_auth) {
                case 1:
                    sprintf(outgoing, "User not authenticated\n");
                    break;
                case 2:
                    sprintf(outgoing, "User %s authenticated with success\n", args[1]);
                    printf("Admin login from %s\n", args[1]);
                    aux = 1;
                    break;
                case 3:
                    sprintf(outgoing, "User not authenticated, %s does not have administrator permissions!\n", args[1]);
                    break;
                case 4:
                case 5:
                    sprintf(outgoing, "User or password is incorrect\n");
                    break;
                case -1:
                    sprintf(outgoing, "User '%s' is already authenticated in somewhere else\n", args[1]);
                    break;
                case 0:
                    if (strcmp(args[0], "logout") == 0) {
                        sprintf(outgoing, "The user has logged out of the account\n");
                        break;
                    } else {
                        aux = 2;
                        if (strcmp(args[0], "add_user") == 0) {
                            if (argc == 4 && ((strcmp(args[3], "administrator") == 0) || (strcmp(args[3], "reader") == 0) || (strcmp(args[3], "journalist") == 0))){
                                // ADD_USER {username} {password} {administrador/cliente/jornalista}
                                add_user(args[1], args[2], args[3]);
                                save_users();
                                sendto(server_socket, "OK\n", 4, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                            }
                        } else if (strcmp(args[0], "del") == 0){
                            // DEL {username}
                            int res = remove_user(args[1]);
                            save_users();
                            if (res == 0) sendto(server_socket, "User not found\n", 16, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                            else sendto(server_socket, "User removed\n", 14, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                        } else if (strcmp(args[0], "list") == 0){
                            memset(outgoing, 0, sizeof(outgoing));
                            for (int i = 0; i < users_size; i++) {
                                char user_info[MAX_LEN_LINE];
                                snprintf(user_info, MAX_LEN_LINE, " Username: %s\n", users[i].username);
                                strcat(outgoing, user_info);
                            }
                            if (sendto(server_socket, outgoing, MAX_LEN_LINE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) printf("Error in sendto");
                        } else if (strcmp(args[0], "quit") == 0){
                            removeClient(inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                            char *logout = "logout";
                            authenticate_user(client_addr, &logout, 1, 0);
                            sprintf(outgoing, "Shutting down the administration console...\n ");
                            if (sendto(server_socket, outgoing, MAX_LEN_LINE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) printf("Error in sendto");
                        } else if (strcmp(args[0], "quit_server") == 0){
                            save_users();
                            sprintf(outgoing, "Shutting down the server...\n ");
                            if (sendto(server_socket, outgoing, MAX_LEN_LINE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) printf("Error in sendto");
                            printf("Shutting down...\n");
                            exit(0);
                        } else {
                            sprintf(outgoing, "The command you entered is not in the Commands list\n\n");
                            aux = 1;
                        }
                    }
                    break;
            }
            if (aux == 1) {
                if (sendto(server_socket, outgoing, MAX_LEN_LINE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) printf("Error in sendto");
            } else if(aux == 2) {
                //
            } else {
                if (sendto(server_socket, outgoing, MAX_LEN_LINE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) printf("Error in sendto");
                memset(outgoing, 0, sizeof(outgoing));
                snprintf(outgoing, MAX_LEN_LINE, "\nlogin {username} {password}: ");
                if (sendto(server_socket, outgoing, MAX_LEN_LINE, 0, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) printf("Error in sendto");
            }
            free_args(args, argc);
        }
    }
    close(server_socket);
    return 0;
}

// Function to add client
void addClient(const char* ip, int port) {
    if (clients_udp < MAX_CLIENTS) {
        strcpy(connected_clients_udp[clients_udp].ip, ip);
        connected_clients_udp[clients_udp].port = port;
        clients_udp++;
    }
}

// Function to remove client
void removeClient(const char* ip, int port) {
    for (int i = 0; i < clients_udp; i++) {
        if (strcmp(connected_clients_udp[i].ip, ip) == 0 && connected_clients_udp[i].port == port) {
            for (int j = i; j < clients_udp - 1; j++) {
                connected_clients_udp[j] = connected_clients_udp[j+1];
            }
            clients_udp--;
            break;
        }
    }
}

// Function to check if client is in list
int isClientInList(const char* ip, int port) {
    for (int i = 0; i < clients_udp; i++) {
        if (strcmp(connected_clients_udp[i].ip, ip) == 0 && connected_clients_udp[i].port == port) {
            return 1; // Client found in list
        }
    }
    return 0; // Client not found in list
}

// Function to save user data in file
void save_users() {
    FILE *file;
    int i = 0; // Tokeziner counter

    file = fopen(save_data.str, "w");
    if (file == NULL) {
        printf("Error opening the file\n");
        exit(1);
    }
    for (i = 0; i < users_size; i++) {
        fprintf(file, "%s;%s;%s\n", users[i].username, users[i].password, users[i].type);
    }
    fclose(file);
}

// Function to add an user to textfile
void add_user(const char *username, const char *password, const char *type) {
    // Realloc to add another user
    users_size += 1;
    users = (user_t*) realloc(users, users_size * sizeof(user_t));

    // Inserting user
    strcpy(users[users_size-1].username, username);
    strcpy(users[users_size-1].password, password);
    strcpy(users[users_size-1].type, type);
    users[users_size-1].authenticated = 0;
}

// Function to remove an user from textfile
int remove_user(const char *username) {
    // Searches for user
    int index = -1;
    int found = 0;
    for (int i = 0; i < users_size; i++) {
        if (strcmp(users[i].username, username) == 0) {
            index = i;
            found = 1;
            printf("USER: %s\n", users[i].username);
            break;
        }
    }

    // If user is found
    if (found == 1) {
        // Adjusts the array
        for (int i = index; i < users_size - 1; i++) {
            users[i] = users[i+1];
        }
        // Resizes the array
        (users_size)--;
        users = (user_t*)realloc(users, (users_size) * sizeof(user_t));
        return 1; // User removed
    } else {
        return 0; // User not found
    }
}

