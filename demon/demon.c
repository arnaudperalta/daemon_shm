#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define TUBE_IN     "../tubes/daemon_shm_in"
#define CONFIG_PATH "demon.conf"

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
	
	return EXIT_SUCCESS;
}

int load_config(config *cfg) {
	FILE *f = fopen(CONFIG_PATH, "r");
	if (f == NULL) {
		perror("Ouverture demon.conf impossible");
		return -1;
	}
	// Lecture du fichier
	int count = 0;
	count += fscanf(f, "MIN_THREAD=%zu\n", &cfg->min_thread);
	count += fscanf(f, "MAX_THREAD=%zu\n", &cfg->max_thread);
	count += fscanf(f, "MAX_CONNECT_PER_THREAD=%zu\n", &cfg->max_con_per_thread);
	count += fscanf(f, "SHM_SIZE=%zu\n", &cfg->shm_size);
	
	if (count != 4) {
		perror("Erreur de syntaxe fichier config");
		return -1;
	}
	
	if (fclose(f) == -1) {
		perror("Erreur fermeture demon.conf");
		return -1;
	}
	return 0;
}

void tube_listening(void) {
	
}
