/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

#include "sv_main.h"
#include "sv_tcp.h"

void send_multicast(char *multicast_ip, int port, char *message) {
    struct sockaddr_in addr;
    int sockfd;

    // Create the socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    // Set the multicast group
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(multicast_ip);
    addr.sin_port = htons(port);

    // Send a message to the multicast group
    if (sendto(sockfd, message, strlen(message), 0,
               (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("sendto");
        exit(1);
    }

    printf("Mensagem enviada para o grupo %s:%d\n", multicast_ip, port);

    // Close the socket
    close(sockfd);
}

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

int generate_multicast_port() {
    srand(time(NULL));
    int port = 1024 + rand() % 64511;
    return port;
}

// Function to handle messages from a single client
void handle_tcp(multicast_shm_t *multicast_groups, int new_socket, int server_fd, struct sockaddr_in address) {
    char buffer[BUF_SIZE] = {0};

    sprintf(buffer, "Bem-vindo ao servidor de notícias");
    send(new_socket, buffer, strlen(buffer), 0);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(new_socket, buffer, BUF_SIZE);
        if (valread < 0) {
            // Error
            perror("Erro ao ler mensagem do cliente");
            break;
        } else if (valread == 0) {
            // Client disconnected
            printf("Cliente desconectado\n");
            exit(EXIT_SUCCESS);
        }

        buffer[strcspn(buffer, "\n")] = 0;

        // Check if message is from an authenticated user
        int argc = 0;
        char **args = command_validation(buffer, &argc);
        if (args == NULL) continue;

        if (strcmp(args[0], "X") == 0) continue;
        int try_auth = authenticate_user(address, args, argc, 1);

        switch (try_auth) {
            case 1:
                sprintf(buffer, "Utilizador não autenticado\n");
                break;
            case 2:
                sprintf(buffer, "Utilizador %s não admitido\n", args[1]); // Administrator
                break;
            case 3:
                write(new_socket, "journalist", strlen("journalist")+1); // +1 for '\0'
                sprintf(buffer, "Utilizador %s autenticado com sucesso [Jornalista]\n", args[1]); // Journalist
                printf("Login de %s\n", args[1]);
                break;
            case 4:
                write(new_socket, "reader", strlen("reader")+1); // +1 for '\0'
                sprintf(buffer, "Utilizador %s autenticado com sucesso [Leitor]\n", args[1]); // Reader
                printf("Login de %s\n", args[1]);
                break;
            case 5:
                sprintf(buffer, "Password incorreta\n");
                break;
            case 6:
                sprintf(buffer, "Utilizador '%s' não existe\n", args[1]);
                break;
            case 0:
                if (strcmp(args[0], "logout") == 0) {
                    sprintf(buffer, "O utilizador deslogou da conta\n");
                    break;
                } else {
                    // Main function
                    if (strcmp(args[0], "list_topics") == 0) {
                        printf("Listar tópicos\n");
                        write(new_socket, "ID\t\tTopic", strlen("list_topics")+1); // +1 for '\0'
                        for (int i = 0; i < MAX_GROUPS; i++) {
                            // TODO: FIX BUFFER
                            char bufferz[BUF_SIZE*4];
                            if (multicast_groups->multicastgroup[i].id[0] != '\0') {
                                sprintf(bufferz, "\r*%s\t\t%s\n", multicast_groups->multicastgroup[i].id, multicast_groups->multicastgroup[i].topic);
                                write(new_socket, bufferz, strlen(bufferz));
                                memset(bufferz, 0, sizeof(bufferz));
                            }
                        }
                        sprintf(buffer, "Listagem de tópicos enviada\n");
                        print_groups(multicast_groups);
                    } else if (strcmp(args[0], "subscribe_topic") == 0) {
                        int subscribed = 0;
                        for (int i = 0; i < MAX_GROUPS; i++) {
                            if (strcmp(multicast_groups->multicastgroup[i].id, args[1]) == 0) {
                                sprintf(buffer, "%s:%d", multicast_groups->multicastgroup[i].ip, multicast_groups->multicastgroup[i].port);
                                write(new_socket, buffer, strlen(buffer)+1); // +1 for '\0'
                                sprintf(buffer, "Subscrição do tópico %s efetuada com sucesso\n", args[1]);
                                subscribed = 1;
                                break;
                            }
                        }
                        if (subscribed == 0)
                            sprintf(buffer, "Tópico com o ID %s não existe\n", args[1]);
                    } else if (strcmp(args[0], "create_topic") == 0) {
                        // Concat topic name
                        char topic_name[BUF_SIZE];
                        memset(topic_name, 0, sizeof(topic_name));
                        for (int i = 2; i < argc; i++) {
                            strcat(topic_name, args[i]);
                            strcat(topic_name, " ");
                        }

                        char *multicast_ip = generate_multicast_ip();
                        int multicast_port = generate_multicast_port();
                        
                        if (find_group(multicast_groups, args[1], multicast_ip, multicast_port) == 1) {
                            sprintf(buffer, "Tópico com o ID %s já existe\n", args[1]);
                            break;
                        }

                        while (find_group(multicast_groups, args[1], multicast_ip, multicast_port) == 2) {
                            multicast_ip = generate_multicast_ip();
                            multicast_port = generate_multicast_port();
                        }

                        add_group(multicast_groups, args[1], topic_name, multicast_ip, multicast_port);
                        sprintf(buffer, "Tópico %s criado com sucesso\n", args[1]);
                        free(multicast_ip);
                    } else if (strcmp(args[0], "send_news") == 0) {
                        printf("Enviar notícia\n");
                        int i = find_group // Should return index
                        send_multicast(multicast_groups->multicastgroup[i].ip, multicast_groups->multicastgroup[i].port, "Novo utilizador conectado\n");

                    }
                }
                break;
        }
        printf("LAST MESSAGE: %s", buffer);
        write(new_socket, buffer, strlen(buffer));
        free_args(args, argc);
    }

    close(new_socket);
}

// Creates TCP listening thread
void *tcp_thread(void *arg) {
    int port = *(int*)arg;
    int server_fd;

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUF_SIZE] = {0};

    // Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket TCP falhou");
        exit(EXIT_FAILURE);
    }

    // Address assignment
    bzero((void*) &address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("Binding do TCP falhou");
        exit(EXIT_FAILURE);
    }

    // Multicast groups
    multicast_shm_t *multicast_shm = create_multicast_shm();

    // Listen mode / [Only for TCP, as UDP is connectionless]
    // Max 3 connections
    if (listen(server_fd, 3) < 0) {
        perror("Escuta TCP falhou");
        exit(EXIT_FAILURE);
    }

    printf("Servidor TCP à escuta na porta %d\n", port);

    // Keep-alive
    while (1) {
        int new_socket;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*) &addrlen))<0) {
            perror("Accept do TCP falhou");
            exit(EXIT_FAILURE);
        }

        printf("Cliente conectado: %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        if (fork() == 0) {
            close(server_fd);
            multicast_shm_t *multicast_shm_ptr = attach_multicast_shm(multicast_shm->shmid);
            handle_tcp(multicast_shm_ptr, new_socket, server_fd, address);
            exit(EXIT_SUCCESS);
        }
        close(new_socket);
    }
}