#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include "demon.h"
#include "thread_manager.h"

#define CONFIG_PATH        "demon.conf"

#define OPTIONS_NUMBER     4
#define MAX_COMMAND_LENGTH 100

typedef struct config {
	size_t min_thread;
	size_t max_thread;
	size_t max_con_per_thread;
	size_t shm_size;
} config;

// Fonction qui charge le fichier de configuration en mémoire.
//		Renvoie -1 en cas d'erreur, 0 en cas de succès
int load_config(config *cfg);

// Procédure executée lorsque le démon passe en mode écoute.
void tube_listening(void);

int main(void) {
	pid_t pid = fork();
	//pid_t sid;
	if (pid < 0) {
		perror("Erreur fork");
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
		return EXIT_SUCCESS;
	}
	// Le démon est créé, on s'associe a un nouveau groupe de processus 
	// afin d'être leader du groupe. Donc pas de terminal de controle.
	
	/*sid = setsid();
	if (sid < 0) {
		perror("Erreur création session"); 
		exit(EXIT_FAILURE);
	}*/
	config cfg;
	load_config(&cfg);
	
	// Création du tube nommé que le programme client utilisera.
	if (mkfifo(TUBE_IN, S_IRUSR | S_IWUSR) == -1) {
		perror("Erreur création tube demon");
		exit(EXIT_FAILURE);
	}

	if (unlink(TUBE_IN) == -1) {
      perror("Erreur suppression tube demon");
      exit(EXIT_FAILURE);
    }
	
	// Création de la mémoire partagé de sémaphores que les threads utiliseront avec un
	//	sémaphore par thread
	int shm_sem = shm_open(SHM_SEM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (shm_sem == -1) {
		perror("Erreur shm_sem");
		exit(EXIT_FAILURE);
	}
	
	if (shm_unlink(SHM_SEM_NAME) == -1) {
		perror("Erreur shm_unlink");
		exit(EXIT_FAILURE);
	}
	
	if (ftruncate(shm_fd, TAILLE_SHM) == -1) {
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}
	
	// Initilisation de MIN_THREAD threads
	pthread_m *ptr = ini_thread(cfg.min_thread, &(cfg.max_con_per_thread), &(cfg.max_thread), shm_sem);
	if (ptr == NULL) {
		perror("Erreur ini threads");
		exit(EXIT_FAILURE);
	}
	
	// Mise en écoute du démon auprès des clients
	if (tube_listening(ptr) == -1) {
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
	// PID du client qui est en train d'établir une connection
	char client_pid[PID_LENGTH + 1];
	
	// File descriptor du tube client, on ne connait pas encore son nom
	int fd_out;
	// File descriptor du demon
	int fd_in = open(TUBE_IN, O_RDONLY);
	if (fd == -1) {
		perror("Erreur ouverture tube");
		return EXIT_FAILURE;
	}
	
	ssize_t n;
	int thread_number;
	while (1) {
		// Les messages recu seront de la forme 00XXXSYNC\0 ou 00XXXEND\0 (x : pid)
		char buffer_read[PID_LENGTH + MES_LENGTH + 1];
		
		// Les messages envoyés seront de la forme RST ou SHM_NAMEXXX (X : numero du thread)
		char buffer_write[BUFFER_LENGTH];
		
		// Nom supposé du tube client
		char tube_name[strlen(TUBE_OUT) + PID_LENGTH + 1];
		strcat(tube_name, TUBE_OUT);
		
		while ((n = read(fd, buffer_read, sizeof buffer_read)) > 0 {
			// Le client envoie SYNC avec son PID
			if (strcmp(SYNC_MSG, buffer + PID_LENGTH) == 0) {
				// Récupération d'un numero de thread libre puis on averti
				//   le client du nom du shm a écouté.
				sprintf(buffer_read, "%5s", client_pid);
				strcat(tube_name, client_pid);
				
				// Ouverture du tube client
				fd_out = open(tube_name, O_WRONLY);
				if (fd_out == -1) {
					perror("Erreur ouverture tube client");
					return FUN_FAILURE;
				}
				
				if (unlink(tube_name) == -1) {
					perror("Unlink tube client");
					return FUN_FAILURE;
				}
				
				thread_number = use_thread(manager);
				if (thread_number == -1) {
					// Pas de thread dispo, on envoie RST
					if (write(fd_out, RST_MSG, sizeof RST_MSG) == -1) {
						perror("Erreur écriture tube client");
						return FUN_FAILURE;
					}
				} else {
					// Un thread est disponible, on envoie donc le nom du shm au client
					sprintf(buffer_write, %s%d, SHM_NAME, thread_number);
					if (write(fd_out, buffer_write, sizeof buffer_write) == -1) {
						perror("Erreur écriture tube client");
						return FUN_FAILURE;
					}
				}
				if (close(fd_out) == -1) {
					perror("Erreur close tube client");
					return FUN_FAILURE;
				}
			}
		}
		printf("Client fini\n");
	}
}
