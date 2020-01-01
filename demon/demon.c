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
#include "thread_manager.h"
#include "defs.h"

#define CONFIG_PATH "demon.conf"
#define ARG_DEBUG   "--debug"      

#define OPTIONS_NUMBER     4
#define MAX_COMMAND_LENGTH 100

// Variable global de debug, on la met a true si --debug est saisi
static bool debug = false;

// Variable global de la structe du manager de thread, elle est global
// pour qu'elle puisse être manipuler après un signal.
static thread_m *manager;

// Fonction de nettoyage des données systeme et mémoire.
void clear_sys_structs(int sig);

// Structure permettant de stocker la configuration lu sur demon.conf
typedef struct config {
  size_t min_thread;
  size_t max_thread;
  size_t max_con_per_thread;
  size_t shm_size;
} config;

// Fonction qui charge le fichier de configuration en mémoire.
// Renvoie -1 en cas d'erreur, 0 en cas de succès
int load_config(config *cfg);

// Fonction executée lorsque le démon passe en mode écoute.
// Renvoie -1 en cas d'erreur, 0 en cas de succès
// Elle est prévu pour ne jamais s'arrêter, seul les signaux
// peuvent y mettre fin.
int tube_listening(size_t shm_size);

// Fonction main du démon
// Si le paramètre --debug est entré, on ne fork pas et on affiche
// tout les messages de débug.
int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], ARG_DEBUG) == 0) {
	debug = true;
    printf("demon Debug mode :\n");
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
  manager = ini_thread(cfg.min_thread, cfg.max_con_per_thread
      , cfg.max_thread, cfg.shm_size);
  if (manager == NULL) {
    perror("Erreur ini threads");
    exit(EXIT_FAILURE);
  }
  
  // Nettoyage des structures systèmes sur les signaux SIGINT et SIGTERM
  // On nettoie aussi les structures de données.
  signal(SIGINT, clear_sys_structs);
  signal(SIGTERM, clear_sys_structs);
  
  // Mise en écoute du démon auprès des clients
  if (tube_listening(cfg.shm_size) == -1) {
    perror("Erreur tube_listening");
    exit(EXIT_FAILURE);
  }
  
  return EXIT_SUCCESS;
}

int load_config(config *cfg) {
  // Ouverture du fichier
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

  if (debug) {
	  printf("Config :\n");
	  printf("min_thread : %zu\n", cfg->min_thread);
	  printf("max_thread : %zu\n", cfg->max_thread);
	  printf("max_con_per_thread : %zu\n", cfg->max_con_per_thread);
	  printf("shm_size : %zu\n\n", cfg->shm_size);
  }
  
  // Si le nombre de paramètres lu n'est pas le bon, on s'arrête
  if (count != OPTIONS_NUMBER) {
    perror("Erreur de syntaxe fichier config");
    return FUN_FAILURE;
  }
  
  // On ferme le fichier
  if (fclose(f) == -1) {
    perror("Erreur fermeture demon.conf");
    return FUN_FAILURE;
  }
  return FUN_SUCCESS;
}

int tube_listening(size_t shm_size) {
  // File descriptor du tube client, on ne connait pas encore son nom
  int fd_client;
  
  // Les messages recu seront de la forme SYNCXXX\0 X : pid
  //   ou ENDYYY\0 y : shm_number
  char buffer_read[PID_LENGTH + MSG_LENGTH + 1];
  
  // Les messages envoyés seront de la forme RST ou XXXSHM_NAMEYYY
  // (XXX : taille de la shm, YYY : numero du thread)
  char buffer_write[SHM_INFO_LENGTH] = {'0'};

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
	// Lecture sur le tube d'une potentiel message d'un client
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
        int thread_number = use_thread(manager);
        if (thread_number == FUN_FAILURE) {
          // Pas de thread dispo, on envoie RST
          if (write(fd_client, RST_MSG, sizeof RST_MSG) == -1) {
            perror("Erreur écriture tube client");
            return FUN_FAILURE;
          }
          if (debug) printf("RST envoyé\n");
        } else {
          // Un thread est disponible, on envoie donc le nom du shm au client
          // puis on lock le thread associé.
          sprintf(buffer_write, "%zu%s%d", shm_size, SHM_NAME
				, thread_number);
          if (write(fd_client, buffer_write, sizeof buffer_write) == -1) {
            perror("Erreur écriture tube client");
            return FUN_FAILURE;
          }
          if (debug) printf("Thread n°%d choisi\n", thread_number);
        }
        if (close(fd_client) == -1) {
          perror("Erreur close tube client");
          return FUN_FAILURE;
        }
      } else if (strncmp(END_MSG, buffer_read, strlen(END_MSG)) == 0) {
        // Numero du thread où on annule une connection
        char number[THR_LENGTH + 1];
        strcpy(number, buffer_read + strlen(END_MSG));
        // On dit au thread d'arrêter son execution et on unlock
        // le thread.
        int ret_value = consume_thread(manager, (size_t) atoi(number));
        if (ret_value == FUN_FAILURE) {
		  perror("Erreur consume_thread");
          return FUN_FAILURE;
        } 
        if (debug) {
		  printf("Connection terminé dans le thread n°%s\n",
		    buffer_read + strlen(END_MSG));
		}
		if (ret_value == 1 && debug) {
			printf("Thread n°%s réinitialisé\n", 
			    buffer_read + strlen(END_MSG));
		} else if (ret_value == 2 && debug) {
			printf("Thread n°%s terminé\n", 
			    buffer_read + strlen(END_MSG));
		}
      }
    }
  }
}

void clear_sys_structs(int sig) {
	// Nettoyage de la mémoire
	if (manager != NULL) {
		thread_dispose(manager);
	}
	// Nettoyage tube demon
	unlink(TUBE_IN);
	// Nettoyage de dev/shm/
	sig = 0;
	int i = 0;
	char shm_name[SHM_INFO_LENGTH];
	do {
		sprintf(shm_name, "%s%d", SHM_NAME, i);
		++i;
		sig = shm_unlink(shm_name);
	} while (sig != FUN_FAILURE);
	exit(EXIT_SUCCESS);
}
