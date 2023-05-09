/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

#include "sv_main.h"
#include "sv_tcp.h"

// Function to handle messages from a single client
void handle_tcp(int new_socket, int server_fd, struct sockaddr_in address) {
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
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0;

        // Check if message is from an authenticated user
        int argc = 0;
        char **argv = command_validation(buffer, &argc);
        if (argv == NULL) continue;

        if (strcmp(argv[0], "X") == 0) continue;
        int try_auth = authenticate_user(address, argv, argc, 1);

        switch (try_auth) {
            case 1:
                sprintf(buffer, "Utilizador não autenticado\n");
                break;
            case 2:
                sprintf(buffer, "Utilizador %s não admitido\n", argv[1]); // Administrator
                break;
            case 3:
                write(new_socket, "journalist", strlen("journalist")+1); // +1 for '\0'
                sprintf(buffer, "Utilizador %s autenticado com sucesso\n", argv[1]); // Journalist
                printf("Login de %s\n", argv[1]);
                break;
            case 4:
                write(new_socket, "reader", strlen("reader")+1); // +1 for '\0'
                sprintf(buffer, "Utilizador %s autenticado com sucesso\n", argv[1]); // Reader
                printf("Login de %s\n", argv[1]);
                break;
            case 5:
                sprintf(buffer, "Password incorreta\n");
                break;
            case 6:
                sprintf(buffer, "Utilizador '%s' não existe\n", argv[1]);
                break;
            case 0:
                if (strcmp(argv[0], "logout") == 0) {
                    sprintf(buffer, "O utilizador deslogou da conta\n");
                    break;
                } else {
                    // Main function
                    printf("Mensagem TCP: %s\n", buffer);
                }
                break;
        }
        write(new_socket, buffer, strlen(buffer));
        free_argv(argv, argc);
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
            handle_tcp(new_socket, server_fd, address);
            exit(EXIT_SUCCESS);
        }
        close(new_socket);
    }
}