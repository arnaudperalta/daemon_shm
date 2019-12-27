#ifndef CLIENT__H
#define CLIENT__H

extern int create_client_tube(char *tube_name);

extern int open_client_tube(char *tube_name);

extern int open_demon_tube(void);

extern int send_demon(int fd_tube, size_t label, char *msg);

// Renvoie -1 en cas d'erreur, -2 si aucun thread n'est disponible
// Sinon renvoie la taille de la shm associé au thread.
extern int receive_demon(int fd_client, char *shm_name);

extern int send_thread(char *shm_name, char *command, size_t shm_size);

extern int receive_thread(char *shm_name, size_t shm_size);

#endif
