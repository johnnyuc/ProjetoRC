// Function to handle messages from a single client
void handle_tcp(int new_socket, int server_fd, struct sockaddr_in address) {
    char buffer[BUF_SIZE] = {0};
    udp_tcp = 1;

    sprintf(buffer, "Bem-vindo ao servidor de notícias");
    send(new_socket, buffer, strlen(buffer), 0);

    char msg[200];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(new_socket, buffer, BUF_SIZE);
        printf("Mensagem recebida: %s\n", buffer);
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
        int try_auth = authenticate_user(address, argv, argc);

        switch (try_auth) {
            case 1:
                sprintf(buffer, "Utilizador não autenticado\n");
                break;
            case 2:
                sprintf(buffer, "Utilizador %s não admitido\n", argv[1]); //administrator
                break;
            case 9:
                sprintf(buffer, "Utilizador %s autenticado com sucesso\n\n", argv[1]); //journalist
                printf("Login de %s\n", argv[1]);
                snprintf(msg, 200, "journalist");
                write(new_socket, msg, strlen(msg));
                break;
            case 10:
                sprintf(buffer, "Utilizador %s autenticado com sucesso\n\n", argv[1]); //reader
                printf("Login de %s\n", argv[1]);
                snprintf(msg, 200, "reader");
                write(new_socket, buffer, strlen(buffer));
                break;
            case 4:
                sprintf(buffer, "Password incorreta\n");
                break;
            case 5:
                sprintf(buffer, "Utilizador '%s' não existe\n", argv[1]);
                break;
            case 0:
                if (strcmp(argv[0], "logout") == 0) {
                    sprintf(buffer, "O utilizador deslogou da conta\n");
                    break;
                } else {
                    // Stuff happens here
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
    while (!received_signal) {
            // Aqui vai o corpo do loop
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*) &addrlen))<0) {
            perror("Accept do TCP falhou");
            exit(EXIT_FAILURE);
        }

        // Client connected
        printf("Cliente conectado: %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        if (fork() == 0) {
            close(server_fd);
            handle_tcp(new_socket, server_fd, address);
            exit(EXIT_SUCCESS);
        }
        close(new_socket);
    }
}