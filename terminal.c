/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

#include "terminal.h"

// Global variables
int fd;
pthread_t listener_thread, writer_thread;

multicast_group_t groups[MAX_GROUPS];
int socks[MAX_GROUPS];
int num_groups = 0;

char type[MAX_LEN_LINE];
char req_topic[MAX_LEN_LINE];

// SIGINT handler
void sigint_handler(int sig) {
    printf("EXIT STEP 0\n");
    // Close all socks
    for (int i = 0; i < num_groups; i++) {
        close(socks[i]);
    }

    printf("EXIT STEP 1\n");
    // Close all threads
    pthread_cancel(listener_thread);
    if (sig == SIGINT || sig == EXIT_FAILURE) 
        pthread_cancel(writer_thread);
    
    printf("EXIT STEP 2\n");
    // Wait for child processes to finish
    for (int i = 0; i < num_groups; i++) {
        wait(NULL);
    }

    printf("\rA sair...\n");
    exit(0);
}

// Function to join multicast group
void join_multicast(char *ip, int port, char *topic) {
    struct sockaddr_in addr;
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    socks[num_groups-1] = sock;
    if (sock < 0) {
        perror("Error creating socket");
        exit(1);
    }

    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Error setting socket options");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error binding socket");
        exit(1);
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("Error joining multicast group");
        exit(1);
    }
    
    // Create child process to listen to multicast group
    if (fork() == 0) {
        printf("A ouvir grupo %s\n", topic);
        char msg[BUF_SIZE];
        while (1) {
            ssize_t bytes_read = read(sock, msg, BUF_SIZE);
            if (bytes_read > 0) {
                printf("\rNova notícia [%s]: %.*s> ", req_topic, (int)bytes_read, msg);
                fflush(stdout);
            }
        }
    }
}

// Function to keep reading data from the server (THREAD)
void *listener_function(void *arg) {
    char msg[BUF_SIZE];

    while (1) {
        memset(msg, 0, BUF_SIZE);
        ssize_t bytes_read = read(fd, msg, BUF_SIZE);

        if (strcmp(msg, "reader") == 0) {
            strcpy(type, "reader");
            continue;
        } else if (strcmp(msg, "journalist") == 0) {
            strcpy(type, "journalist");
            continue;
        }

        // If message is a multicast address, join group
        char *token = strtok(msg, ":");
        if (token == NULL)
            continue;
        struct in_addr addr;
        if (inet_aton(token, &addr) != 0) {
            if (IN_MULTICAST(ntohl(addr.s_addr))) {
                if (num_groups == MAX_GROUPS) {
                    printf("\rNúmero máximo de tópicos subscritos atingido.\n");
                } else {
                    // Verify if it was already added
                    for (int i = 0; i < num_groups; i++) {
                        if (strcmp(groups[i].ip, token) == 0) {
                            printf("\rTópico já subscrito\n");
                            break;
                        }
                    }

                    char *port = strtok(NULL, ":");
                    strncpy(groups[num_groups].ip, msg, 16);
                    groups[num_groups].port = atoi(port);
                    num_groups++;
                    join_multicast(msg, atoi(port), req_topic);
                }
                continue;
            }
        }

        // Bytes to print
        if (bytes_read > 0) {
            printf("\r*%.*s\n> ", (int) bytes_read, msg);
            fflush(stdout);
        }
    }
}

// Function to send data to the server (THREAD)
void *writer_function(void *arg) {
    char command[MAX_LEN_LINE];

    while (1) {
        int valid = 0;
        do {
            printf("> ");
            fflush(stdout);

            // Read command from stdin
            memset(command, 0, BUF_SIZE);
            fgets(command, MAX_LEN_LINE, stdin);

            // Remove newline character
            char *newline_pos = strchr(command, '\n');
            if (newline_pos != NULL) *newline_pos = '\0';
            if (strlen(command) == 0) continue;
            
            if (strcmp(command, "quit") == 0) {
                sigint_handler(0);
                pthread_cancel(pthread_self());
                pthread_exit(NULL);
            } else {
                // Make a copy of the original command
                char command_copy[MAX_LEN_LINE];
                strcpy(command_copy, command);

                char *token;
                char *args[MAX_ARGS];
                int argc = 0;

                token = strtok(command_copy, " ");
                while (token != NULL && argc < MAX_ARGS) {
                    args[argc++] = token;
                    token = strtok(NULL, " ");
                }
                args[argc] = NULL;  // Terminate the list with NULL

                if (strcmp(args[0], "login") == 0 && argc == 3) {
                    valid = 1;
                } else if (strcmp(args[0], "list_topics") == 0 && (strcmp(type, "reader") == 0 || strcmp(type, "journalist") == 0) && argc == 1) {
                    valid = 1;
                } else if (strcmp(args[0], "subscribe_topic") == 0 && (strcmp(type, "reader") == 0 || strcmp(type, "journalist") == 0) && argc == 2) {
                    strcpy(req_topic, args[1]);
                    valid = 1;
                } else if (strcmp(args[0], "create_topic") == 0 && strcmp(type, "journalist") == 0 && argc == 3) {
                    valid = 1;
                } else if (strcmp(args[0], "send_news") == 0 && strcmp(type, "journalist") == 0 && argc == 3) {
                    valid = 1;
                } else {
                    if (strcmp(type, "reader") != 0 && strcmp(type, "journalist") != 0)
                        printf("You need to login first.\n");
                    else printf("Command is not valid.\n");
                }
                /* // DEBUG
                printf("Command: %s\n", args[0]);
                printf("Arguments: ");
                for (int i = 1; i < argc; i++) {
                    printf("%s ", args[i]);
                }
                printf("\nARGC: %d\n", argc);
                */
            }
        } while (!valid);

        write(fd, command, strlen(command));
    }
}

// Main function
int main(int argc, char *argv[]) {
    signal(SIGINT, sigint_handler);

    char endServer[100];
    struct sockaddr_in addr;
    struct hostent *hostPtr;

    // Verify arguments
    if (argc != 3) {
        printf("./tcp_client <host> <port>\n");
        exit(-1);
    }

    strcpy(endServer, argv[1]);
    if ((hostPtr = gethostbyname(endServer)) == 0)
        perror("Could not obtain address");

    // Address assignment
    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *) (hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short) atoi(argv[2]));

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        perror("Socket");
    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        perror("Connect");

    pthread_create(&listener_thread, NULL, listener_function, NULL);
    pthread_create(&writer_thread, NULL, writer_function, NULL);

    pthread_join(listener_thread, NULL);
    pthread_join(writer_thread, NULL);

    close(fd);
    return 0;
}
