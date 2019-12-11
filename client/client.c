#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "defs.h"
#include "client.h"

#define QUIT_CMD  "quit"
#define BEGIN     "client_shm>"
#define NO_THREAD -2

// Fonction locale qui compte le nombre de décimal d'un entier
int digit_count(int n);

int main(void) {
  pid_t pid = getpid();
  char shm_name[SHM_NAME_LENGTH];
  char tube_name[TUBE_NAME_LENGTH];
  sprintf(tube_name, "%s%d", TUBE_OUT, pid);
  
  // Ouverture du tube client
  if (open_client_tube(tube_name) == FUN_FAILURE) {
    perror("Erreur ouverture tube client");
    exit(EXIT_FAILURE);
  }
  
  // Demande de synchronisation avec le démon
  if (send_daemon(pid, SYNC_MSG) == FUN_FAILURE) {
    perror("Erreur send_daemon");
    exit(EXIT_FAILURE);
  }

  int thread_number = receive_daemon(tube_name, shm_name);
  if (thread_number == NO_THREAD) {
    perror("Plus de threads disponibles");
    exit(EXIT_FAILURE);
  }
  
  char command[MAX_CMD_LENGTH];
  char result[MAX_RES_LENGTH];
  int length;

  // Début du prompt
  while ((length = scanf(BEGIN "%s", command)) != EOF && strcmp(command, QUIT_CMD) != 0) {
    // La commande lue est trop longue
    if (length > MAX_CMD_LENGTH) {
      perror("Commande trop longue.");
      continue;
    }
    if (send_thread(shm_name, command) == FUN_FAILURE) {
      perror("Erreur send_thread");
      exit(EXIT_FAILURE);
    }
    if (receive_thread(shm_name, result) == FUN_FAILURE) {
      perror("Erreur receive_thread");
      exit(EXIT_FAILURE);
    }
  }
  
  if (send_daemon(pid, END_MSG) == -1) {
    exit(EXIT_FAILURE);
  }
  
  return EXIT_SUCCESS;
}

int open_client_tube(char *tube_name) {
  
  // Création du tube nommé que le programme démon utilisera.
  if (mkfifo(tube_name, S_IRUSR | S_IWUSR) == -1) {
    perror("Erreur création tube client");
    return FUN_FAILURE;
  }

  if (unlink(tube_name) == -1) {
      perror("Erreur suppression tube client");
      return FUN_FAILURE;
    }
    
    return FUN_SUCCESS;
}

int send_daemon(pid_t pid, char *msg) {
  // Ecriture de la commande a envoyée (OOXXXSYNC\0 ou OOXXXEND\0)
  char full_msg[PID_LENGTH + strlen(msg) + 1];
  memset(full_msg, '0', PID_LENGTH + strlen(msg) + 1);
  sprintf(full_msg + PID_LENGTH - digit_count(pid), "%d%s", pid, msg);
  
  // Envoi du message
  int fd = open(TUBE_IN, O_WRONLY);
  if (fd == -1) {
    perror("Erreur ouverture tube démon");
    return FUN_FAILURE;
  }
  
  if (write(fd, full_msg, sizeof full_msg) == -1) {
    perror("Erreur écriture tube démon");
    return FUN_FAILURE;
  }
  
  if (close(fd) == -1) {
    perror("Erreur close tube démon");
    return FUN_FAILURE;
  }
  
  return FUN_SUCCESS;
}

int receive_daemon(char *tube_name, char *shm_name) {
  // Réception de la réponse
  int fd = open(tube_name, O_RDONLY);
  if (fd == -1) {
    perror("Erreur ouverture tube client");
    return FUN_FAILURE;
  }
  
  char reponse[BUFFER_SIZE];
  if (read(fd, reponse, sizeof reponse) != sizeof (int)) {
    perror("Erreur read tube client");
    return FUN_FAILURE;
  }
  if (strcmp(reponse, RST_MSG) == 0) {
    return NO_THREAD;
  }
  
  strcpy(shm_name, reponse);

  if (close(fd) == -1) {
    perror("Erreur close tube démon");
    return FUN_FAILURE;
  }
  
  return FUN_SUCCESS;
}

int send_thread(char *shm_name, char *command) {
  int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (shm_fd == FUN_FAILURE) {
    perror("Erreur shm_open send_thread");
    return FUN_FAILURE;
  }
  
  transfer *ptr = mmap(NULL, sizeof (transfer), PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr == MAP_FAILED) {
    perror("Erreur mmap send_thread");
    return FUN_FAILURE;
  }

  strcpy(ptr->command, command);
  ptr->flag = DEMON_FLAG;

  if (close(shm_fd) == -1) {
    perror("Erreur close send_thread");
    return FUN_FAILURE;
  }

  return FUN_SUCCESS;
}

int receive_thread(char *shm_name, char *result) {
  int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (shm_fd == FUN_FAILURE) {
    perror("Erreur shm_open receive_thread");
    return FUN_FAILURE;
  }
  
  transfer *ptr = mmap(NULL, sizeof (transfer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr == MAP_FAILED) {
    perror("Erreur mmap receive_thread");
    return FUN_FAILURE;
  }

  // Attente du résultat
  while (ptr->flag != CLIENT_FLAG);
  strcpy(result, ptr->result);
  ptr->flag = NEUTRAL_FLAG;

  if (close(shm_fd) == -1) {
    perror("Erreur close receive_thread");
    return FUN_FAILURE;
  }

  return FUN_SUCCESS;
}

int digit_count(int n) {
  if (n < 10) {
    return 1;
  }
  return 1 + digit_count(n / 10);
}
