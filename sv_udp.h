/*******************************************************************************
 * LEI UC 2022-2023 // Network communications (RC)
 * Johnny Fernandes 2021190668, João Macedo 2021220627
 * All comments in this segment are in english
 *******************************************************************************/

# ifndef SV_UDP_H
# define SV_UDP_H


// Functio prototypes
void *udp_thread(void *arg);
void save_users();
void add_user(const char *username, const char *password, const char *type);
void remove_user(const char *username);

# endif // SV_UDP_H