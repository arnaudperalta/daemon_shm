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
#define BEGIN     "client>"
#define NO_THREAD -2

// Fonction locale qui compte le nombre de décimal d'un entier
int digit_count(int n);

int main(void) {
  char shm_name[SHM_NAME_LENGTH];
  char tube_name[TUBE_NAME_LENGTH];
  
  pid_t pid = getpid();
  sprintf(tube_name, "%s%d", TUBE_OUT, pid);
  
  // Création et ouverture du tube client
  if (create_client_tube(tube_name) == FUN_FAILURE) {
    perror("Erreur create_client_tube");
    exit(EXIT_FAILURE);
  }
  
  // Ouverture tube demon
  int fd_demon = open_demon_tube();
  if (fd_demon == FUN_FAILURE) {
    perror("Erreur open_demon_tube");
    exit(EXIT_FAILURE);
  }
  
  // Demande de synchronisation avec le démon
  if (send_demon(fd_demon, (size_t) pid, SYNC_MSG) == FUN_FAILURE) {
    perror("Erreur send_demon");
    exit(EXIT_FAILURE);
  }
  
  // Ouverture du tube
  int fd_client = open_client_tube(tube_name);
  if (fd_client == FUN_FAILURE) {
    perror("Erreur open_client_tube");
    exit(EXIT_FAILURE);
  }

  // Réception du numéro du shm et fermeture du tube
  int thread_number = receive_demon(fd_client, shm_name);
  if (thread_number == FUN_FAILURE) {
    perror("Erreur receive_demon");
    exit(EXIT_FAILURE);
  } else if (thread_number == NO_THREAD) {
    printf("Fermeture du programme, pas de thread disponible\n");
    exit(EXIT_SUCCESS);
  }
  
  char command[MAX_CMD_LENGTH];
  char result[MAX_RES_LENGTH];
  int length;

  // Début du prompt
  printf(BEGIN);
  while ((length = scanf("%s", command)) != EOF 
      && strcmp(command, QUIT_CMD) != 0) {
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
    printf("%s\n", result);
    printf(BEGIN);
  }
  
  // Envoi de END car plus de commande a executé
  if (send_demon(fd_demon, (size_t) pid, END_MSG) == FUN_FAILURE) {
    perror("Erreur send_demon");
    exit(EXIT_FAILURE);
  }
  
  return EXIT_SUCCESS;
}

int create_client_tube(char *tube_name) {
  // Création du tube nommé que le programme démon utilisera.
  if (mkfifo(tube_name, S_IRUSR | S_IWUSR) == FUN_FAILURE) {
    perror("Erreur création tube client");
    return FUN_FAILURE;
  }
  return FUN_SUCCESS;
}

int open_client_tube(char *tube_name) {
  int fd = open(tube_name, O_RDONLY);
  if (fd == FUN_FAILURE) {
    perror("Erreur ouverture tube démon");
    return FUN_FAILURE;
  }
  return fd;
}

int open_demon_tube(void) {
  int fd = open(TUBE_IN, O_WRONLY);
  if (fd == FUN_FAILURE) {
    perror("Erreur ouverture tube demon");
    return FUN_FAILURE;
  }
  return fd;
}

int send_demon(int fd_tube, size_t label, char *msg) {
  // Ecriture de la commande a envoyée (SYNCXXX\0 ou ENDYYY\0)
  // XXX : pid, YYY : numéro shm
  char full_msg[(int) strlen(msg) + digit_count((int) label) + 1];
  sprintf(full_msg, "%s%zu", msg, label);
  
  if (write(fd_tube, full_msg, sizeof full_msg) == FUN_FAILURE) {
    perror("Erreur écriture tube démon");
    return FUN_FAILURE;
  }
  
  return FUN_SUCCESS;
}

int receive_demon(int fd_client, char *shm_name) {
  // Réception de la réponse
  char reponse[SHM_NAME_LENGTH];
  if (read(fd_client, reponse, sizeof reponse) == FUN_FAILURE) {
    perror("Erreur read tube client");
    return FUN_FAILURE;
  }
  // Si le message envoyé est RST, aucun thread n'est disponible
  if (strcmp(reponse, RST_MSG) == 0) {
    return NO_THREAD;
  }
  
  // Copie du nom de la shm
  strcpy(shm_name, reponse);
  
  // On ne réutilisera pas ce tube
  if (close(fd_client) == FUN_FAILURE) {
    perror("Erreur fermeture tube client");
    return FUN_FAILURE;
  }
  
  return FUN_SUCCESS;
}

int send_thread(char *shm_name, char *command) {
  int shm_fd = shm_open(shm_name, O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
  if (shm_fd == FUN_FAILURE) {
    perror("Erreur shm_open send_thread");
    return FUN_FAILURE;
  }
  
  if (ftruncate(shm_fd, sizeof(transfer)) == FUN_FAILURE) {
    perror("Erreur ftruncate");
    return FUN_FAILURE;
  }
  
  transfer *ptr = mmap(NULL, sizeof (transfer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr == MAP_FAILED) {
    perror("Erreur mmap send_thread");
    return FUN_FAILURE;
  }

  strcpy(ptr->command, command);
  ptr->flag = DEMON_FLAG;

  if (close(shm_fd) == FUN_FAILURE) {
    perror("Erreur close send_thread");
    return FUN_FAILURE;
  }

  return FUN_SUCCESS;
}

int receive_thread(char *shm_name, char *result) {
  int shm_fd = shm_open(shm_name, O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
  if (shm_fd == FUN_FAILURE) {
    perror("Erreur shm_open receive_thread");
    return FUN_FAILURE;
  }
  
  if (ftruncate(shm_fd, sizeof(transfer)) == FUN_FAILURE) {
    perror("Erreur ftruncate");
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

  if (close(shm_fd) == FUN_FAILURE) {
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
