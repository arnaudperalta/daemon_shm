#ifndef CLIENT__H
#define CLIENT__H

int open_client_tube(char *tube_name);

int send_daemon(pid_t pid, char *msg);

int receive_daemon(char *tube_name, char *shm_name);

int send_thread(char *shm_name, char *command);

int receive_thread(char *shm_name, char *result);

#endif
