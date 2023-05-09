/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define MAX_LEN_LINE 128
#define MAX_GROUPS 32
#define h_addr h_addr_list[0]

struct MulticastGroup {
    char ip[16];
    int port;
};

int fd;
struct MulticastGroup groups[MAX_GROUPS];
int num_groups = 0;
char type[MAX_LEN_LINE];

void join_multicast(char *ip, int port) {
    struct sockaddr_in addr;
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
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

    printf("Joined multicast group %s:%d\n", ip, port);
    
    if (fork() == 0) {
        char msg[BUF_SIZE];
        while (1) {
            ssize_t bytes_read = read(sock, msg, BUF_SIZE);
            if (bytes_read > 0) {
                printf("%.*s\n", (int)bytes_read, msg);
                fflush(stdout);
            }
        }
    }
}

void *listener_function(void *arg) {
    char msg[BUF_SIZE];

    while (1) {
        ssize_t bytes_read = read(fd, msg, BUF_SIZE);

        // Bytes to print
        if (bytes_read > 0) {
            printf("%.*s\n", (int) bytes_read, msg);
            fflush(stdout);
        }
        
        if (strcmp(msg, "reader") == 0) {
            strcpy(type, "reader");
            printf("Tipo de utilizador alterado para leitor.\n");
            continue;
        } else if (strcmp(msg, "journalist") == 0) {
            strcpy(type, "journalist");
            printf("Tipo de utilizador alterado para jornalista.\n");
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
                    printf("Número máximo de tópicos subscritos atingido.\n");
                } else {
                    // Verify if it was already added
                    for (int i = 0; i < num_groups; i++) {
                        if (strcmp(groups[i].ip, token) == 0) {
                            printf("Tópico já subscrito\n");
                            break;
                        }
                    }

                    char *port = strtok(NULL, ":");
                    strncpy(groups[num_groups].ip, msg, 16);
                    groups[num_groups].port = atoi(port);
                    num_groups++;
                    join_multicast(msg, atoi(port));
                }
                continue;
            }
        }
    }
}

void *writer_function(void *arg) {
    char command[MAX_LEN_LINE];

    while (1) {

        // While domain is not searchable, requests new domain
        do {
            printf("> ");
            fflush(stdout);

            memset(command, 0, BUF_SIZE);
            fgets(command, MAX_LEN_LINE, stdin);

            if (strlen(command) == 0)
                printf("Insira um comando.\n");

        } while (strlen(command) < 1);

        // Sends domain request
        write(fd, command, strlen(command));
    }
}

int main(int argc, char *argv[]) {
    char endServer[100];
    struct sockaddr_in addr;
    struct hostent *hostPtr;

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

    pthread_t listener_thread, writer_thread;
    pthread_create(&listener_thread, NULL, listener_function, NULL);
    pthread_create(&writer_thread, NULL, writer_function, NULL);

    pthread_join(listener_thread, NULL);
    pthread_join(writer_thread, NULL);

    close(fd);
    return 0;
}
