#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_LEN_CREDS 32
#define MAX_LEN_LINE 128

struct User {
  char username[MAX_LEN_CREDS];
  char password[MAX_LEN_CREDS];
  char type[MAX_LEN_CREDS];
};

// Function to open credentials.txt file and read all users data
struct User* load_users(const char* filename, int* size) {
    FILE *file;

    char login[MAX_LEN_LINE];
    char *token; // Tokenizer

    struct User *users; // Pointer to struct array
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
    users = (struct User*) malloc((*size) * sizeof(struct User));

    // Goes back to beggining
    rewind(file);

    while (fgets(login, MAX_LEN_LINE, file)) {
        token = strtok(login, ";");
        strcpy(users[i].username, token);
        token = strtok(NULL, ";");
        strcpy(users[i].password, token);
        token = strtok(NULL, ";");
        strcpy(users[i].type, token);
        i++;
    }

    fclose(file);
    return users;
}

// Function to add an user
void add_user(struct User *users, int *size, const char *username, const char *password, const char *type) {
    // Realloc to add another user
    *size += 1;
    users = (struct User*) realloc(users, (*size) * sizeof(struct User));

    // Inserting user
    strcpy(users[(*size)-1].username, username);
    strcpy(users[(*size)-1].password, password);
    strcpy(users[(*size)-1].type, type);
}

// Function to remove an user
void remove_user_by_username(struct User *users, int *size, const char *username) {
    // Searchs for user index
    int index = -1;
    for (int i = 0; i < *size; i++) {
        if (strcmp(users[i].username, username) == 0) {
            index = i;
        break;
        }
    }

    // If found
    if (index != -1) {
        // Adjusts array
        for (int i = index; i < *size - 1; i++) {
            users[i] = users[i+1];
        }
        // Reallocs memory to free space
        (*size)--;
        users = (struct User*)realloc(users, (*size) * sizeof(struct User));
    } else {
        printf("Utilizador não encontrado\n");
    }
}


// Function to print credentials
void print_credentials(struct User *users, int size) {
    for (int j = 0; j < size; j++) {
        printf("Utilizador %d: %s - %s - %s", j+1, users[j].username, users[j].password, users[j].type);
    }
    printf("\n");
}

// Creates TCP listener thread
void *tcp_thread(void *arg) {
    int port = *(int*)arg;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

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

    // Listening mode / TCP needs while UDP doesn't keep persistent connections
    if (listen(server_fd, 3) < 0) {
        perror("Escuta TCP falhou");
        exit(EXIT_FAILURE);
    }

    printf("Servidor TCP à escuta na porta %d\n", port);

    // Keep-alive
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*) &addrlen))<0) {
            perror("Accept do TCP falhou");
            exit(EXIT_FAILURE);
        }

        int valread = read(new_socket, buffer, 1024);
        printf("Mensagem TCP: %s\n", buffer);
        close(new_socket);
    }
}

// Creates UDP listener thread
void *udp_thread(void *arg) {
    int port = *(int*)arg;
    int sock_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

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

    // Keep-alive
    while (1) {
        int valread = recvfrom(sock_fd, buffer, 1024,
                               MSG_WAITALL, (struct sockaddr *)&address,
                               (socklen_t*)&addrlen);
        printf("Mensagem UDP: %s\n", buffer);
    }
}

int main(int argc, char const *argv[]) {
   // Verify arguments and set ports
    if (argc < 4) {
        printf("Sintaxe: ./news_server {PORTO_NOTICIAS} {PORTO_CONFIG} {FICHEIRO_CONFIG}\n");
        exit(EXIT_FAILURE);
    } 
    int news_port = atoi(argv[1]);
    int config_port = atoi(argv[2]);
    if (news_port < 0 || news_port > 65535 || config_port < 0 || config_port > 65535) {
        printf("Erro: o valor das portas deverá estar entre 1 e 65535\n");
        exit(EXIT_FAILURE);
    }
    // printf("PORTO_NOTICIAS: %d, PORTO_CONFIG: %d\n", news_port, config_port); // Debug purposes

    // User loading
    struct User *users;
    int size = 0;
    users = load_users(argv[3], &size);
    // print_credentials(users, size); // Debug purposes

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