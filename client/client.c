#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"

#define QUIT_CMD  "quit"
#define BEGIN     "client_shm>"
#define NO_THREAD -2

int open_client_tube(pid_t pid);
int ask_daemon(void);

int digit_count(int n);

int main(void) {
	pid_t pid = getpid();
	char tube_name[TUBE_NAME_LENGTH];
	sprinf(tube_name, "%s%d", TUBE_OUT, pid);
	
	// Ouverture du tube client
	if (open_client_tube(tube_name) == FUN_FAILURE) {
		perror("Erreur ouverture tube client");
		exit(EXIT_FAILURE);
	}
	
	// Demande de synchronisation avec le démon
	int thread_number = send_daemon(tube_name, pid, SYNC_MSG);
	if (thread_number == FUN_FAILURE) {
		perror("Erreur send_daemon");
		exit(EXIT_FAILURE);
	}
	if (thread_number == NO_THREAD) {
		perror("Plus de threads disponibles");
		exit(EXIT_FAILURE);
	}
	
	char command[MAX_CMD_LENGTH];
	int length;
	
	while ((length = scanf(BEGIN "%s", command)) != EOF && strcmp(command, QUIT_CMD) != 0) {
		// La commande lue est trop longue
		if (length > MAX_CMD_LENGTH) {
			perror("Commande trop longue.");
			continue;
		}
	}
	
	if (send_daemon(END_MSG) == -1) {
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

char *send_daemon(char *tube_name, pid_t pid, char *msg, char *shm_name) {
	// Ecriture de la commande a envoyée (OOXXXSYNC\0 ou OOXXXEND\0)
	char full_msg[PID_LENGTH + strlen(msg) + 1] = {'0'};
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
	
	// Si le message envoyé est un message de fin, on arrête la fonction
	if (strcmp(msg, END_MSG) == 0) {
		return FUN_SUCCESS;
	}
	
	// Réception de la réponse
	fd = open(tube_name, O_RDONLY);
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
	
	return FUN_SUCCESS;
}



int digit_count(int n) {
    if (n < 0) {
		return digit_count((n == INT_MIN) ? MAX_INT: -n);
	}
    if (n < 10) {
		return 1;
	}
    return 1 + digit_count(n / 10);
}
