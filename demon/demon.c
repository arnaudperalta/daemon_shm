#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include "demon.h"
#include "thread_manager.h"
#include "defs.h"

#define CONFIG_PATH "demon.conf"
#define ARG_DEBUG   "--debug"      

#define OPTIONS_NUMBER     4
#define MAX_COMMAND_LENGTH 100

// Variable global de debug, on la met a true si --debug est saisi
static bool debug = false;

// Fonction de nettoyage des données systeme et mémoire.
void clear_sys_structs(int sig);

struct config {
  size_t min_thread;
  size_t max_thread;
  size_t max_con_per_thread;
  size_t shm_size;
};

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], ARG_DEBUG) == 0) {
	debug = true;
    printf("daemon_shm Debug mode :\n");
    printf("--------------------------\n");
  } else {
    pid_t pid = fork();
    pid_t sid;
    if (pid < 0) {
      perror("Erreur fork");
      exit(EXIT_FAILURE);
    } else if (pid > 0) {
      return EXIT_SUCCESS;
    }
    // Le démon est créé, on s'associe a un nouveau groupe de processus 
    // afin d'être leader du groupe. Donc pas de terminal de controle.
    
    sid = setsid();
    if (sid < 0) {
      perror("Erreur création session"); 
      exit(EXIT_FAILURE);
    }
  }
  
  // Lecture du fichier de configuration
  config cfg;
  load_config(&cfg);
  
  // Initilisation de MIN_THREAD threads
  thread_m *ptr = ini_thread(cfg.min_thread, cfg.max_con_per_thread, cfg.max_thread);
  if (ptr == NULL) {
    perror("Erreur ini threads");
    exit(EXIT_FAILURE);
  }
  
  // Nettoyage des structures systèmes sur le signal SIGINT
  // On dispose aussi les structures de données.
  signal(SIGINT, clear_sys_structs);
  
  // Mise en écoute du démon auprès des clients
  if (tube_listening(&cfg, ptr) == -1) {
    perror("Erreur tube_listening");
    exit(EXIT_FAILURE);
  }
  
  return EXIT_SUCCESS;
}

int load_config(config *cfg) {
  FILE *f = fopen(CONFIG_PATH, "r");
  if (f == NULL) {
    perror("Ouverture demon.conf impossible");
    return FUN_FAILURE;
  }
  // Lecture du fichier
  int count = 0;
  count += fscanf(f, "MIN_THREAD=%zu\n", &cfg->min_thread);
  count += fscanf(f, "MAX_THREAD=%zu\n", &cfg->max_thread);
  count += fscanf(f, "MAX_CONNECT_PER_THREAD=%zu\n", &cfg->max_con_per_thread);
  count += fscanf(f, "SHM_SIZE=%zu\n", &cfg->shm_size);

  printf("Config :\n");
  printf("min_thread : %zu\n", cfg->min_thread);
  printf("max_thread : %zu\n", cfg->max_thread);
  printf("max_con_per_thread : %zu\n", cfg->max_con_per_thread);
  printf("shm_size : %zu\n\n", cfg->shm_size);
  
  if (count != OPTIONS_NUMBER) {
    perror("Erreur de syntaxe fichier config");
    return FUN_FAILURE;
  }
  
  if (fclose(f) == -1) {
    perror("Erreur fermeture demon.conf");
    return FUN_FAILURE;
  }
  return FUN_SUCCESS;
}

int tube_listening(config *cfg, thread_m *manager) {
  // File descriptor du tube client, on ne connait pas encore son nom
  int fd_client;
  
  // Les messages recu seront de la forme SYNCXXX\0 X : pid
  //   ou ENDYYY\0 y : shm_number
  char buffer_read[PID_LENGTH + MSG_LENGTH + 1];
  
  // Les messages envoyés seront de la forme RST ou SHM_NAMEXXX (X : numero du thread)
  char buffer_write[SHM_NAME_LENGTH] = {'0'};

  // Création du tube nommé que le programme client utilisera.
  if (mkfifo(TUBE_IN, S_IRUSR | S_IWUSR) == -1) {
    perror("Erreur création tube demon");
    exit(EXIT_FAILURE);
  }
  
  // File descriptor du demon
  int fd_demon = open(TUBE_IN, O_RDONLY);
  if (fd_demon == -1) {
    perror("Erreur ouverture tube");
    return EXIT_FAILURE;
  }

  while (1) {
	ssize_t n = 0;
    while ((n = read(fd_demon, buffer_read, sizeof buffer_read)) > 0) {
	  // Préfixe supposé du tube client
	  char tube_name[strlen(TUBE_OUT) + PID_LENGTH + 1];
	  strcpy(tube_name, TUBE_OUT);
	  
      // Le client envoie SYNC avec son PID
      if (strncmp(SYNC_MSG, buffer_read, strlen(SYNC_MSG)) == 0) {
        // Récupération d'un numero de thread libre puis on averti
        //   le client du nom du shm a écouté.
        if (debug) {
			printf("Connection du processus %s\n", 
				buffer_read + strlen(SYNC_MSG));
		}
		printf("buffer : %s\n", buffer_read);
        strcat(tube_name, buffer_read + strlen(SYNC_MSG));
        
        // Ouverture du tube client
        fd_client = open(tube_name, O_WRONLY);
        if (fd_client == -1) {
          perror("Erreur ouverture tube client");
          return FUN_FAILURE;
        }
        
        if (unlink(tube_name) == -1) {
          perror("Unlink tube client");
          return FUN_FAILURE;
        }
        int thread_number = use_thread(manager, cfg->max_con_per_thread);
        if (thread_number == -1) {
          // Pas de thread dispo, on envoie RST
          if (write(fd_client, RST_MSG, sizeof RST_MSG) == -1) {
            perror("Erreur écriture tube client");
            return FUN_FAILURE;
          }
          if (debug) printf("RST envoyé\n");
        } else {
          // Un thread est disponible, on envoie donc le nom du shm au client
          sprintf(buffer_write, "%s%d", SHM_NAME, thread_number);
          if (write(fd_client, buffer_write, sizeof buffer_write) == -1) {
            perror("Erreur écriture tube client");
            return FUN_FAILURE;
          }
          if (debug) printf("Thread %d choisi\n", thread_number);
        }
        if (close(fd_client) == -1) {
          perror("Erreur close tube client");
          return FUN_FAILURE;
        }
      } else if (strncmp(END_MSG, buffer_read, strlen(END_MSG)) == 0) {
        // Numero du thread où on annule une connection
        char number[THR_LENGTH + 1];
        strcpy(number, buffer_read + strlen(END_MSG));
        // On dit au thread d'arrêter son execution
        if (consume_thread(manager, atoi(number) == -1)) {
          return FUN_FAILURE;
        }
        if (debug) {
			printf("Connection terminé avec %s\n",
			    buffer_read + strlen(END_MSG));
		}
      }
    }
  }
}

void clear_sys_structs(int sig) {
	// Nettoyage tube demon
	unlink(TUBE_IN);
	// Nettoyage SHM
	sig = 0;
	int i = 0;
	char shm_name[SHM_NAME_LENGTH];
	do {
		sprintf(shm_name, "%s%d", SHM_NAME, i);
		++i;
		sig = shm_unlink(shm_name);
	} while (sig != FUN_FAILURE);
	exit(EXIT_SUCCESS);
}
