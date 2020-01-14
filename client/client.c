#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "defs.h"

#define QUIT_CMD  "quit"
#define BEGIN     "client>"

#define NO_THREAD          -2
#define BUFFER_SIZE        128
#define MAX_DIGIT_SHM_SIZE 30

// Fonction de création du tube client pour recevoir les informations de
// connection à la shm
// Renvoie -1 en cas d'erreur, 0 en cas de succès
int create_client_tube(char *tube_name);

// Fonction d'ouverture du tube client
// Renvoie -1 en cas d'erreur, 0 en cas de succès
int open_client_tube(char *tube_name);

// Fonction d'ouverture du tube du démon pour communiquer avec lui
// Renvoie -1 en cas d'erreur, 0 en cas de succès
int open_demon_tube(void);

// Fonction d'envoie d'informations au démon à travers le tube démon
// On envoye une commande de la forme SYNCXXX\0 ou ENDYYY\0
// XXX : pid, YYY : numéro shm
// Renvoie -1 en cas d'erreur, 0 en cas de succès
int send_demon(int fd_tube, size_t label, char *msg);

// Fonction de réception de donnée à partir du tube du démon
// Renvoie -1 en cas d'erreur, -2 si aucun thread n'est disponible
// Sinon renvoie la taille de la shm associé au thread.
int receive_demon(int fd_client, char *shm_name);

// Fonction d'envoie de commande au thread associé au client
int send_thread(char *shm_name, char *command, size_t shm_size);

// Fonction de récuperation du résultat de la commande exécuté
// de la commande envoyé au thread associé.
int receive_thread(char *shm_name, size_t shm_size);

// Fonction locale qui compte le nombre de décimal d'un entier
int digit_count(int n);

// Fonction locale qui envoit le message de fin de connection au démon
void send_end(int sig);

// Variables globale utiles pour la fonction appellé par le signal
static int fd_demon;
static char shm_name[SHM_INFO_LENGTH];

int main(void) {
  char tube_name[TUBE_NAME_LENGTH];
  
  pid_t pid = getpid();
  sprintf(tube_name, "%s%d", TUBE_OUT, pid);
  
  // Création et ouverture du tube client
  if (create_client_tube(tube_name) == FUN_FAILURE) {
    perror("Erreur create_client_tube");
    exit(EXIT_FAILURE);
  }
  
  // Ouverture tube demon
  fd_demon = open_demon_tube();
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
  int shm_size = receive_demon(fd_client, shm_name);
  if (shm_size == FUN_FAILURE) {
    perror("Erreur receive_demon");
    exit(EXIT_FAILURE);
  } else if (shm_size == NO_THREAD) {
    printf("Fermeture du programme, pas de thread disponible\n");
    exit(EXIT_SUCCESS);
  }
  
  char buffer[BUFFER_SIZE];

  // Récupération du signal en cas d'interruption du programme anormal
  signal(SIGINT, send_end);
  signal(SIGTERM, send_end);

  // Début du prompt
  printf(BEGIN);
  // Saisie de la commande et vérification du "quit"
  while (fgets(buffer, BUFFER_SIZE, stdin) != NULL 
      && strncmp(buffer, QUIT_CMD, strlen(QUIT_CMD)) != 0) {
    // On enlève le caractère de retour à la ligne
    buffer[strlen(buffer) - 1] = '\0';
    
    if (send_thread(shm_name, buffer, (size_t) shm_size) == FUN_FAILURE) {
      perror("Erreur send_thread");
      exit(EXIT_FAILURE);
    }
    if (receive_thread(shm_name, (size_t) shm_size) == FUN_FAILURE) {
      perror("Erreur receive_thread");
      exit(EXIT_FAILURE);
    }
    printf(BEGIN);
  }
  
  send_end(0);
  
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
  char reponse[SHM_INFO_LENGTH];
  if (read(fd_client, reponse, sizeof reponse) == FUN_FAILURE) {
    perror("Erreur read tube client");
    return FUN_FAILURE;
  }
  // Si le message envoyé est RST, aucun thread n'est disponible
  if (strcmp(reponse, RST_MSG) == 0) {
    return NO_THREAD;
  }
  
  // Lecture de la taille de shm
  char size[MAX_DIGIT_SHM_SIZE];
  int i = 0;
  while (reponse[i] <= '9') {
    size[i] = reponse[i];
    ++i;
  }
  size[i] = '\0';
  int shm_size = atoi(size);
  if (shm_size == 0) {
    perror("Mauvais taille de shm");
    return FUN_FAILURE;
  }
  
  // Copie du nom de la shm
  strcpy(shm_name, reponse + i - 1);
  
  // On ne réutilisera pas ce tube
  if (close(fd_client) == FUN_FAILURE) {
    perror("Erreur fermeture tube client");
    return FUN_FAILURE;
  }
  
  return shm_size;
}

int send_thread(char *shm_name, char *command, size_t shm_size) {
  int shm_fd = shm_open(shm_name, O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
  if (shm_fd == FUN_FAILURE) {
    perror("Erreur shm_open send_thread");
    return FUN_FAILURE;
  }
  
  if (ftruncate(shm_fd, (long int) shm_size) == FUN_FAILURE) {
    perror("Erreur ftruncate");
    return FUN_FAILURE;
  }
  
  void *ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr == MAP_FAILED) {
    perror("Erreur mmap send_thread");
    return FUN_FAILURE;
  }
  
  char *cmdptr = (char *) ptr + sizeof (int);
  // Copie de la commande a exécuter dans la shm
  // On exclu 4 octets à cause du int indiquant la nature de la donnée
  strncpy(cmdptr, command, shm_size - sizeof(int));
  
  // On indique que la donnée est destinée au démon
  volatile int *flag = (int *) ptr;
  *flag = DEMON_FLAG;

  if (close(shm_fd) == FUN_FAILURE) {
    perror("Erreur close send_thread");
    return FUN_FAILURE;
  }

  return FUN_SUCCESS;
}

int receive_thread(char *shm_name, size_t shm_size) {
  int shm_fd = shm_open(shm_name, O_RDWR | O_EXCL, S_IRUSR | S_IWUSR);
  if (shm_fd == FUN_FAILURE) {
    perror("Erreur shm_open receive_thread");
    return FUN_FAILURE;
  }
  
  if (ftruncate(shm_fd, (long int) shm_size) == FUN_FAILURE) {
    perror("Erreur ftruncate");
    return FUN_FAILURE;
  }
  
  void *ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (ptr == MAP_FAILED) {
    perror("Erreur mmap receive_thread");
    return FUN_FAILURE;
  }

  // Attente du résultat
  volatile int *flag = (int *) ptr;
  while (*flag != CLIENT_FLAG);
  
  char *result = (char *) ptr + sizeof (int);
  size_t i = 0;
  while (result[i] != '\0') {
    putchar(result[i]);
    ++i;
  }
  *flag = NEUTRAL_FLAG;
  result[0] = '\0';

  if (close(shm_fd) == FUN_FAILURE) {
    perror("Erreur close receive_thread");
    return FUN_FAILURE;
  }

  return FUN_SUCCESS;
}

void send_end(int sig) {
  (void) sig;
  // Envoi de END car plus de commande a executé
  if (send_demon(fd_demon, (size_t) atoi(shm_name + strlen(SHM_NAME))
      , END_MSG) == FUN_FAILURE) {
    perror("Erreur send_demon");
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

int digit_count(int n) {
  if (n < 10) {
    return 1;
  }
  return 1 + digit_count(n / 10);
}
