// LEI UC 2022-2023 // Network communications (RC)
// Johnny Fernandes 2021190668, João Macedo 2021220627

#ifndef NEWS_PROJECT_SERVER_H
#define NEWS_PROJECT_SERVER_H

#define MAX_LEN_CREDS 32
#define MAX_LEN_LINE 128


// --------------------------------
#include <sys/socket.h>//
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>//
#include <netinet/in.h>
#include <unistd.h>//
#include <stdlib.h>//
#include <stdio.h>//
#include <string.h>// 
#include <netdb.h>

#define SERVER_PORT 9000
#define BUF_SIZE 1024
#define DNS_LINESIZE 128
// --------------------------------

struct User {
  char username[MAX_LEN_CREDS];
  char password[MAX_LEN_CREDS];
  char type[MAX_LEN_CREDS];
};

#endif //NEWS_PROJECT_SERVER_H