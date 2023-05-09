/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

#include "sv_main.h"
#include "sv_udp.h"

// Global variables [inside sv_main.c]
extern user_t *users;
extern int users_size;
extern savedata_t save_data;

// Creates UDP listening thread
void *udp_thread(void *arg) {
    int port = *(int*)arg;
    int sock_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUF_SIZE] = {0};

    // Socket
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("Socket UDP falhou");
        exit(EXIT_FAILURE);
    }

    // Address assignment
    bzero((void*) &address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind
    if (bind(sock_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("Binding do UDP falhou");
        exit(EXIT_FAILURE);
    }

    printf("Servidor UDP à escuta na porta %d\n", port);

    int valread;

    while (1) {
        valread = recvfrom(sock_fd, buffer, BUF_SIZE, MSG_WAITALL, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (valread > 0){
            break;
        }
    }
    printf("\nA receber mensagens UDP!\n");

    char msg[200];
    snprintf(msg, 200, "\nlogin {Username} {Password}: ");
    if (sendto(sock_fd, msg, strlen(msg), 0, (struct sockaddr *)&address, addrlen) == -1) {
        printf("Erro no sendto");
    }


    // Keep-alive
    while (1) {
        valread = recvfrom(sock_fd, buffer, BUF_SIZE, MSG_WAITALL,
                           (struct sockaddr *)&address, (socklen_t*)&addrlen);

        buffer[strcspn(buffer, "\n")] = 0;

        // Check if message is from an authenticated user
        int argc = 0;
        char **argv = command_validation(buffer, &argc);
        if (argv == NULL) continue;

        if (strcmp(argv[0], "X") == 0) continue; // Ignore verbose UDP startup from netcat
        int try_auth = authenticate_user(address, argv, argc);

        int aux=0;
        switch (try_auth) {
            case 1:
                sprintf(buffer, "Utilizador não autenticado\n");
                break;
            case 2:
                sprintf(buffer, "Utilizador %s autenticado com sucesso\n\n", argv[1]);
                printf("Login de %s\n", argv[1]);
                aux=1;
                break;
            case 3:
                sprintf(buffer, "Utilizador não autenticado, %s nao possui permissoes de administrador!\n", argv[1]);
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
                    printf("Comando UDP: %s\n", buffer);
                    aux=2;
                    if(strcmp(argv[0], "ADD_USER") == 0 && ((strcmp(argv[3], "administrator")==0) || (strcmp(argv[3], "reader")==0) || (strcmp(argv[3], "journalist")==0))){
                        //ADD_USER {username} {password} {administrador/cliente/jornalista}
                        add_user(argv[1], argv[2], argv[3]);
                    }else if(strcmp(argv[0], "DEL") == 0){
                        //DEL {username}
                        remove_user(argv[1]);
                    }else if(strcmp(argv[0], "LIST") == 0){
                        for (int i = 0; i < users_size; i++) {
                            sprintf( buffer,"  Username: %s\n", users[i].username);
                            if (sendto(sock_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&address, sizeof(address)) == -1) printf("Erro no sendto");
                        }
                    }else if(strcmp(argv[0], "QUIT") == 0){
                        sprintf(buffer, "A encerrar a consola de admnistracao...\n ");
                        if (sendto(sock_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&address, sizeof(address)) == -1) printf("Erro no sendto");
                        //Enviar um sinal que permita fezhar a consola
                    }else if(strcmp(argv[0], "QUIT_SERVER") == 0){
                        save_users();
                        sprintf(buffer, "A encerrar o servidor...\n ");
                        if (sendto(sock_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&address, sizeof(address)) == -1) printf("Erro no sendto");
                        printf("Encerrando...\n");
                        exit(0);
                    }else{
                        sprintf(buffer, "O comando que introduziu nao se encontra na lista de Comandos.\n"
                                        "----------------MENU DE COMANDOS----------------\n"
                                        "- ADD_USER {username} {password} {administrador/cliente/jornalista}\n"
                                        "- DEL {username}\n"
                                        "- LIST\n"
                                        "- QUIT\n"
                                        "- QUIT_SERVER\n\n");
                        aux=1;
                    }
                }
                break;
        }
        if(aux==1){
            if (sendto(sock_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&address, sizeof(address)) == -1) printf("Erro no sendto");
        }else if(aux==2){
            //
        }else{
            if (sendto(sock_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&address, sizeof(address)) == -1) printf("Erro no sendto");
            snprintf(msg, 200, "\nlogin {Username} {Password}: ");
            if (sendto(sock_fd, msg, strlen(msg), 0, (struct sockaddr *)&address, addrlen) == -1) printf("Erro no sendto");
        }
        free_argv(argv, argc);
    }
}

void save_users() {
    FILE *file;
    int i = 0; // Tokeziner counter

    file = fopen(save_data.str, "w");
    if (file == NULL) {
        printf("Erro ao abrir o ficheiro.\n");
        exit(1);
    }
    for (i = 0; i < users_size; i++) {
        fprintf(file, "%s;%s;%s\n", users[i].username, users[i].password, users[i].type);
    }
    fclose(file);
}

// Function to add an user
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

// Function to remove an user
void remove_user(const char *username) {
    // Searches for user
    int index = -1;
    for (int i = 0; i < users_size; i++) {
        if (strcmp(users[i].username, username) == 0) {
            index = i;
            break;
        }
    }

    // If user is found
    if (index != -1) {
        // Adjusts the array
        for (int i = index; i < users_size - 1; i++) {
            users[i] = users[i+1];
        }
        // Resizes the array
        (users_size)--;
        users = (user_t*)realloc(users, (users_size) * sizeof(user_t));
    } else {
        printf("Utilizador não encontrado\n");
    }
}