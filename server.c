/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

#include "server.h"

void erro(char* msg);
void process_client(int fd, struct sockaddr_in* client_addr, int nextClientID);

void process_client(int client_fd, struct sockaddr_in *client_addr, int nextClientID) {

    char buffer[BUF_SIZE];
    char* onconnection = "Bem-vindo ao servidor de nomes do DEI. Indique o nome de domínio.\n";
    write(client_fd, onconnection, strlen(onconnection)+1);
    // Grabs identification from client to register actions
    char ip[INET_ADDRSTRLEN];
    inet_ntop(client_addr->sin_family, &(client_addr->sin_addr), ip, INET_ADDRSTRLEN);
    // Acknowledging connection 
    printf("Cliente %i (%s:%i) acabou de chegar.\n", nextClientID, ip, client_addr->sin_port);
    /****************/
    
    do {
        memset(buffer, 0, sizeof(buffer));

        read(client_fd, buffer, BUF_SIZE-1); // Receives input
        buffer[strlen(buffer)+1] = '\0'; // Guarantees end of string

        char msg[BUF_SIZE]; 

        
        char domain[DNS_LINESIZE-INET_ADDRSTRLEN];
        int iter = 0;
        while (buffer[iter] != '\0' && buffer[iter] != '\n') {domain[iter] = buffer[iter]; iter++;} domain[iter] = '\0';

        if (strlen(domain) < 1) {
            snprintf(msg, BUF_SIZE, "Insira um domínio.\n");
            write(client_fd, msg, strlen(msg)+1);
            continue;
        }

        if (strcmp(domain, "SAIR") == 0) {
            snprintf(msg, BUF_SIZE, "Até logo!");
            write(client_fd, msg, strlen(msg) + 1);
            printf("Cliente %i (%s:%i) foi embora.\n", nextClientID, ip, client_addr->sin_port);
            close(client_fd);
            return;
        }

        printf("Cliente %i (%s:%i) requisita \"%s\"\n", nextClientID, ip, client_addr->sin_port, domain);

        write(client_fd, msg, strlen(msg) + 1);
        fflush(stdout); 
    } 
    while (strcmp(buffer, "SAIR") != 0);
}

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
}

// Function for errors, receives an error message and quits
// TODO: Missing sigints and full exit of all connections and processes
void erro(char* msg) {
    printf("Error: %s\n", msg);
    exit(-1);
}

// Main functions that does most of the initial loading
int main(int argc, char *argv[]) {
    // Verify arguments and set ports
    if (argc < 4) {
        printf("Sintaxe: ./news_server {PORTO_NOTICIAS} {PORTO_CONFIG} {FICHEIRO_CONFIG}\n");
        return 1;
    } 
    int news_port = atoi(argv[1]);
    int config_port = atoi(argv[2]);
    if (news_port < 0 || news_port > 65535 || config_port < 0 || config_port > 65535) {
        printf("Erro: o valor das portas deverá estar entre 1 e 65535\n");
        return 1;
    }
    printf("PORTO_NOTICIAS: %d, PORTO_CONFIG: %d\n", *argv[1], *argv[2]); // Debug purposes

    // User loading
    struct User *users;
    int size = 0;
    users = load_users(argv[3], &size);
    print_credentials(users, size); // Debug purposes

    // Variables used
    int fd, client, nextClientID = 0;
    struct sockaddr_in addr, client_addr;
    int client_addr_size;

    // Initialize variables
    bzero((void*) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);

    // Possible errors initiating server
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) erro("Socket function");
    if (bind(fd,(struct sockaddr*) &addr, sizeof(addr)) < 0) erro("Bind function");
    if( listen(fd, 5) < 0) erro("Listen function");

    client_addr_size = sizeof(client_addr);

    // Running server
    while (1) {
        while(waitpid(-1, NULL, WNOHANG) > 0);
        client = accept(fd, (struct sockaddr*) &client_addr, (socklen_t*) & client_addr_size);
        if (client > 0) {
            nextClientID++; // Client number counter
            
            // Fork to execute code in a son process
            // Allows concurrent connections
            if (fork() == 0) {
                close(fd);
                // Will remain in process_client until connection is rejected
                process_client(client, &client_addr, nextClientID);
                exit(0);
            }
            close(client);
        }
    }
    return 0;
}