#ifndef CLIENT__H
#define CLIENT__H

extern int create_client_tube(char *tube_name);

extern int open_client_tube(char *tube_name);

extern int open_demon_tube(void);

extern int send_demon(int fd_tube, size_t label, char *msg);

extern int receive_demon(int fd_client, char *shm_name);

int send_thread(char *shm_name, char *command);

int receive_thread(char *shm_name, char *result);

#endif
