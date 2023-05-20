/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

#include "terminal.h"

// Global variables
int fd; // File descriptor for FIFO
sem_t sem; // Semaphore
pthread_t listener_thread, writer_thread; // Threads

// User
char type[STANDARD_LEN]; // Type of user

// Multicast
char temp[2][STANDARD_LEN]; // Temporary array for multicast group info
multicast_group_t groups[MAX_GROUPS]; // Multicast groups joined´
int pids[MAX_GROUPS]; // PIDs of child processes
int socks[MAX_GROUPS]; // Sockets for multicast groups
int num_groups = 0; // # of groups joined by user

// SIGINT
volatile sig_atomic_t flag = 0;

// SIGINT handler
void sigint_handler(int sig) {
    flag = 1; // Set flag to 1

    // Close FIFO
    close(fd);

    // Close sockets
    for (int i = 0; i < num_groups; i++) {
        kill(pids[i], SIGINT);
        close(socks[i]);
    }

    // Wait for child processes to finish
    for (int i = 0; i < num_groups; i++) {
        wait(NULL);
    }
    
    // Destroy semaphore
    sem_destroy(&sem);

    // Exit
    if (pthread_self() != listener_thread && pthread_self() != writer_thread) 
        printf("\rExiting...\n");

    exit(0);
}

// Function to send a message to a multicast group
void send_multicast(char *multicast_ip, int port, char *message) {
    int sockfd;
    struct sockaddr_in addr;

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        raise(SIGINT);
    }

    // Set the TTL to limit the scope of the multicast
    int ttl = 3;
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("setsockopt");
        raise(SIGINT);
    }

    // Set up the address structure for the multicast group
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(multicast_ip);
    addr.sin_port = htons(port);

    // Send the message to the multicast group
    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("sendto");
        raise(SIGINT);
    }

    // Print message sent
    printf("Sent message to multicast group %s:%d\n", multicast_ip, port);
    close(sockfd);
}

// Function to join multicast group
void join_multicast(char *ip, int port, char *topic) {
    printf("Joining multicast group %s:%d with topic: %s\n", ip, port, topic);
    struct sockaddr_in addr;
    int sock;

    // Create a UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    socks[num_groups-1] = sock;
    if (sock < 0) {
        perror("Error creating socket");
        raise(SIGINT);
    }

    // Set socket options
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Error setting socket options");
        raise(SIGINT);
    }

    // Bind socket to multicast address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Error binding socket");
        raise(SIGINT);
    }

    // Join multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(ip);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("Error joining multicast group");
        raise(SIGINT);
    }
    
    // Create child process to listen to multicast group
    pids[num_groups-1] = fork();
    if (pids[num_groups-1] == 0) {
        char incoming[NEWS_LEN];
        memset(incoming, 0, NEWS_LEN);
        while (!flag) {
            ssize_t bytes_read = read(sock, incoming, NEWS_LEN);
            if (bytes_read > 0) {
                printf("\rNews [from \'%s\']: %.*s\n> ", topic, (int)bytes_read, incoming);
                fflush(stdout);
            } else if (bytes_read == -1 && errno == EINTR) {
                // Interrupted by signal (SIGINT)
                continue;
            } else {
                perror("Error reading from multicast group");
                break;
            }}}}

// Function to keep reading data from the server (THREAD)
void *listener_function(void *arg) {
    char incoming[STANDARD_LEN];

    while (!flag) {
        // Clear buffer
        memset(incoming, 0, STANDARD_LEN);
        // Read from server
        ssize_t bytes_read = read(fd, incoming, STANDARD_LEN);

        // Reading from server failed
        if (bytes_read < 0) {
            perror("Error reading from server");
            raise(SIGINT);
        } else if (bytes_read == 0) {
            flag = 1;
            printf("\rServer shutting down\n");
            raise(SIGINT);
        }

        // Get user type
        if (strcmp(incoming, "reader") == 0) {
            strcpy(type, "reader");
            continue;
        } else if (strcmp(incoming, "journalist") == 0) {
            strcpy(type, "journalist");
            continue;
        }

        // If message is a multicast address
        char *token = strtok(incoming, ":");

        struct in_addr addr;
        if (inet_aton(token, &addr) != 0) {
            if (IN_MULTICAST(ntohl(addr.s_addr))) {
                strcpy(temp[0], token);
                strcpy(temp[1], strtok(NULL, ":"));
                sem_post(&sem);
                continue;   
            }
        }   

        // Bytes to print
        if (bytes_read > 0) {
            printf("\r*%.*s\n> ", (int) bytes_read, incoming);
            fflush(stdout);
        }
    }
}

// Function to split string into tokens
char **split_string(char *str, const char *delim, int *argc) {
    char **args = NULL;
    char *token = strtok(str, delim);
    int size = 0;

    while (token != NULL) {
        args = realloc(args, sizeof(char *) * ++size);
        args[size - 1] = token;
        token = strtok(NULL, delim);
    }

    *argc = size;
    return args;
}

// Function to free arguments
void free_args(char **args, int argc) {
    if (args != NULL) {
        for (int i = 0; i < argc; i++)
            args[i] = NULL; // reset each element to NULL to avoid double-freeing
        free(args);
    }
}

// Function to send data to the server (THREAD)
void *writer_function(void *arg) {
    char command[NEWS_LEN];

    while (!flag) {
        int command_id = 0;

        do {
            printf("> ");
            fflush(stdout);

            // Read command from stdin
            memset(command, 0, NEWS_LEN);
            fgets(command, STANDARD_LEN, stdin);

            // Remove newline character
            char *newline_pos = strchr(command, '\n');
            if (newline_pos != NULL) *newline_pos = '\0';
            if (strlen(command) == 0) continue;
            
            // Make a copy of the original command
            char command_copy[STANDARD_LEN];
            strcpy(command_copy, command);

            // Split command into tokens
            int argc = 0;
            char **args = split_string(command_copy, " ", &argc);

            // Check command
            if (strcmp(command, "quit") == 0) {
                sigint_handler(0);
                pthread_cancel(pthread_self());
                pthread_exit(NULL);
            } else if (strcmp(args[0], "login") == 0 && argc == 3) {
                if (strlen(args[1]) > INPUT_LEN || strlen(args[2]) > INPUT_LEN) {
                    printf("\rUsername and password must be less than 32 characters\n");
                    break;
                }
                write(fd, command, strlen(command));
                free_args(args, argc); command_id = 1;
            } else if (strcmp(args[0], "list_topics") == 0 && (strcmp(type, "reader") == 0 || strcmp(type, "journalist") == 0) && argc == 1) {
                write(fd, command, strlen(command));
                free_args(args, argc); command_id = 2;
            } else if (strcmp(args[0], "subscribe_topic") == 0 && (strcmp(type, "reader") == 0 || strcmp(type, "journalist") == 0) && argc == 2) {
                if (strlen(args[1]) > INPUT_LEN) {
                    printf("\rTopic ID must be less than 32 characters\n");
                    break;
                }
                // Check current subscribed topics
                int has_subscribed = 0;
                if (num_groups == MAX_GROUPS) {
                    printf("\rMaximum number of subscribed topics reached\n");
                    break;
                } else if (num_groups > 0) {
                    for (int i = 0; i < num_groups; i++) {
                        if (strcmp(groups[i].topic, args[1]) == 0) {
                            printf("\rTopic has been already subscribed\n");
                            has_subscribed = 1; }}}

                if (has_subscribed) break;
                
                // Else, subscribe to topic
                write(fd, command, strlen(command));
                sem_wait(&sem);

                // Add topic to subscribed topics
                strcpy(groups[num_groups].topic, args[1]);
                strcpy(groups[num_groups].ip, temp[0]);
                groups[num_groups].port = atoi(temp[1]);
                num_groups++;

                // Join multicast group
                join_multicast(temp[0], atoi(temp[1]), args[1]);
                free_args(args, argc); command_id = 3;
            } else if (strcmp(args[0], "create_topic") == 0 && strcmp(type, "journalist") == 0 && argc == 3) {
                if (strlen(args[1]) > INPUT_LEN || strlen(args[2]) > INPUT_LEN) {
                    printf("\rTopic ID and topic name must be less than 32 characters\n");
                    break;
                }
                write(fd, command, strlen(command));
                free_args(args, argc); command_id = 4;
            } else if (strcmp(args[0], "send_news") == 0 && strcmp(type, "journalist") == 0 && argc >= 3) {
                char news_request[STANDARD_LEN];
                memset(news_request, 0, STANDARD_LEN);
                strcat(news_request, args[0]);
                strcat(news_request, " ");
                strcat(news_request, args[1]);

                write(fd, news_request, strlen(news_request));
                sem_wait(&sem);
                char news_message[NEWS_LEN];
                memset(news_message, 0, NEWS_LEN);
                for (int i = 2; i < argc; i++) {
                    strcat(news_message, args[i]);
                    strcat(news_message, " ");
                }
                send_multicast(temp[0], atoi(temp[1]), news_message);
                free_args(args, argc); command_id = 5;
            } else { // Invalid command
                if (strcmp(type, "reader") != 0 && strcmp(type, "journalist") != 0)
                    printf("You need to login first.\n");
                else printf("Command is not valid.\n");
                free_args(args, argc);
            }
        } while (!command_id);
    }
}

// Main function
int main(int argc, char *argv[]) {
    signal(SIGINT, sigint_handler);

    struct sockaddr_in addr;
    struct hostent *hostPtr;

    // Verify arguments
    if (argc != 3) {
        printf("./tcp_client <host> <port>\n");
        exit(EXIT_FAILURE);
    }

    // Get host address
    if ((hostPtr = gethostbyname(argv[1])) == 0)
        perror("Could not obtain address");

    // Address assignment
    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *) (hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short) atoi(argv[2]));

    sem_init(&sem, 0, 0);
    
    // Create socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        perror("Socket");
    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
        perror("Connect");

    // Create threads
    pthread_create(&listener_thread, NULL, listener_function, NULL);
    pthread_create(&writer_thread, NULL, writer_function, NULL);

    // Wait for threads to finish
    pthread_join(listener_thread, NULL);
    pthread_join(writer_thread, NULL);

    return 0;
}
