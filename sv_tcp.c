/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

#include "sv_main.h"
#include "sv_tcp.h"

// Global variables [inside sv_main.c]
extern volatile sig_atomic_t flag;
extern multicast_shm_t *multicast_shm;
extern int num_clients;
extern int server_fd;
extern int connected_clients[3];

// Function to generate ip for a multicast group
char* generate_multicast_ip() {
    char* multicast_ip = malloc(16 * sizeof(char));
    srand(time(NULL));
    int ip_octet1 = 224 + rand() % 16;
    int ip_octet2 = rand() % 256;
    int ip_octet3 = rand() % 256;
    int ip_octet4 = rand() % 256;
    sprintf(multicast_ip, "%d.%d.%d.%d", ip_octet1, ip_octet2, ip_octet3, ip_octet4);
    return multicast_ip;
}

// Function to generate port for a multicast group
int generate_multicast_port() {
    srand(time(NULL));
    int port = 1024 + rand() % 64511;
    return port;
}

// Function to handle messages from a single client
void handle_tcp(multicast_shm_t *multicast_groups, int new_socket, struct sockaddr_in address) {
    char incoming[MAX_LEN_LINE];
    char outgoing[MAX_LEN_LINE];

    // Send welcome message
    sprintf(outgoing, "Welcome to our news server!\n");
    send(new_socket, outgoing, strlen(outgoing), 0);

    while (!flag) {
        memset(incoming, 0, sizeof(incoming));
        int valread = read(new_socket, incoming, MAX_LEN_LINE);
        if (valread < 0) {
            // Error
            perror("Error reading from sock");
            break;
        } else if (valread == 0) {
            // Client disconnected
            printf("TCP client disconnected: %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            exit(EXIT_SUCCESS);
        }

        incoming[strcspn(incoming, "\n")] = 0;

        // Check if message is from an authenticated user
        int argc = 0;
        char **args = command_validation(incoming, &argc);
        if (args == NULL) continue;

        if (strcmp(args[0], "X") == 0) continue; // Ignore startup if any
        int try_auth = authenticate_user(address, args, argc, 1);

        switch (try_auth) {
            case 1:
                sprintf(outgoing, "User not authenticated\n");
                break;
            case 2:
                sprintf(outgoing, "User %s not admited\n", args[1]); // Administrator
                break;
            case 3:
                write(new_socket, "journalist", strlen("journalist")+1); // +1 for '\0'
                sprintf(outgoing, "User %s authenticated with success [Journalist]\n", args[1]); // Journalist
                break;
            case 4:
                write(new_socket, "reader", strlen("reader")+1); // +1 for '\0'
                sprintf(outgoing, "User %s authenticated with success [Reader]\n", args[1]); // Reader
                break;
            case 5:
            case 6:
                sprintf(outgoing, "User or password is incorrect\n");
                break;
            case 0:
                if (strcmp(args[0], "logout") == 0) {
                    sprintf(outgoing, "The user has logged out of the account\n");
                    break;
                } else if (strcmp(args[0], "login") == 0) {
                    sprintf(outgoing, "The user is already logged in\n");
                    break;
                } else {
                    // Main function
                    if (strcmp(args[0], "list_topics") == 0) {
                        write(new_socket, "ID\t\tTopic", strlen("list_topics")+1); // +1 for '\0'
                        for (int i = 0; i < MAX_GROUPS; i++) {
                            // TODO: FIX BUFFER
                            char outgoing[MAX_LEN_LINE];
                            if (multicast_groups->multicastgroup[i].id[0] != '\0') {
                                sprintf(outgoing, "\r*%s\t\t%s\n", multicast_groups->multicastgroup[i].id, multicast_groups->multicastgroup[i].topic);
                                write(new_socket, outgoing, strlen(outgoing));
                                memset(outgoing, 0, sizeof(outgoing));
                            }
                        }
                        sprintf(outgoing, "List of topics sent\n");
                    } else if (strcmp(args[0], "subscribe_topic") == 0) {
                        int subscribed = 0;
                        for (int i = 0; i < MAX_GROUPS; i++) {
                            if (strcmp(multicast_groups->multicastgroup[i].id, args[1]) == 0) {
                                sprintf(outgoing, "%s:%d", multicast_groups->multicastgroup[i].ip, multicast_groups->multicastgroup[i].port);
                                write(new_socket, outgoing, strlen(outgoing)+1); // +1 for '\0'
                                sprintf(outgoing, "Subscription of topic %s made with success\n", args[1]);
                                subscribed = 1;
                                break;
                            }
                        }
                        if (subscribed == 0)
                            sprintf(outgoing, "Topic with ID %s does not exist\n", args[1]);
                    } else if (strcmp(args[0], "create_topic") == 0) {
                        // Concat topic name
                        char topic_name[MAX_LEN_LINE];
                        memset(topic_name, 0, sizeof(topic_name));
                        for (int i = 2; i < argc; i++) {
                            strcat(topic_name, args[i]);
                            strcat(topic_name, " ");
                        }

                        char *multicast_ip = generate_multicast_ip();
                        int multicast_port = generate_multicast_port();
                        
                        if (group_exists(multicast_groups, args[1], multicast_ip, multicast_port) == 1) {
                            sprintf(outgoing, "Topic with ID %s already exists\n", args[1]);
                            break;
                        }

                        while (group_exists(multicast_groups, args[1], multicast_ip, multicast_port) == 2) {
                            multicast_ip = generate_multicast_ip();
                            multicast_port = generate_multicast_port();
                        }

                        add_group(multicast_groups, args[1], topic_name, multicast_ip, multicast_port);
                        sprintf(outgoing, "Topic %s created with success\n", args[1]);
                        free(multicast_ip);
                    } else if (strcmp(args[0], "send_news") == 0) {
                        int result = group_index(multicast_groups, args[1]);
                        if (result == -1) {
                            sprintf(outgoing, "Topic with ID %s does not exist\n", args[1]);
                        } else {
                            sprintf(outgoing, "%s:%d", multicast_groups->multicastgroup[result].ip, 
                                multicast_groups->multicastgroup[result].port);
                        }
                    }
                }
                break;
        }
        write(new_socket, outgoing, strlen(outgoing));
        free_args(args, argc);
    }

    close(new_socket);
}

// Creates TCP listening thread
void *tcp_thread(void *arg) {
    int port = *(int*)arg;

    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("TCP socket failed");
        exit(EXIT_FAILURE);
    }

    // Address assignment
    bzero((void*) &address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Error setting SO_REUSEADDR option");
        exit(EXIT_FAILURE);
    }

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("TCP binding failed");
        exit(EXIT_FAILURE);
    }

    // Multicast groups
    multicast_shm = create_multicast_shm();

    // Listen mode / [Only for TCP, as UDP is connectionless]
    // Max 3 connections
    if (listen(server_fd, 3) < 0) {
        perror("TCP listen failed");
        exit(EXIT_FAILURE);
    }

    printf("TCP server listening on port %d\n", port);

    // Keep-alive
    while (!flag) {
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*) &addrlen)) < 0) {
            perror("TCP accept failed");
            exit(EXIT_FAILURE);
        }
        connected_clients[num_clients] = new_socket;
        pid_t tcpPID;

        printf("TCP client connected: %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        if (fork() == 0) {
            num_clients++;
            signal(SIGTSTP, SIG_IGN);
            close(server_fd);
            multicast_shm_t *multicast_shm_ptr = attach_multicast_shm(multicast_shm->shmid);
            handle_tcp(multicast_shm_ptr, new_socket, address);
        }
        close(new_socket);
    }
    close(server_fd);
}