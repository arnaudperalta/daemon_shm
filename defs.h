#ifndef DEFS__H
#define DEFS__H

// Fichier Header contenant des définitions communes au client et au démon

#define MAX_CMD_LENGTH   100
#define TUBE_NAME_LENGTH 32
#define PID_LENGTH       5
#define MSG_LENGTH       4
#define BUFFER_SIZE      32
#define SHM_SIZE         1024
#define FUN_SUCCESS      0
#define FUN_FAILURE      -1

// Flag décrivant la nature des données contenu dans le shm.
#define NEUTRAL_FLAG 0 // SHM Initialisée
#define DEMON_FLAG   1 // Client a entré la commande
#define CLIENT_FLAG  2 // Client peut récupérer le resultat

#define SYNC_MSG  "SYNC"
#define END_MSG   "END"
#define RST_MSG   "RST"
#define SHM_NAME  "../shm/shm_thread_"
#define TUBE_IN   "../tubes/daemon_in"
#define TUBE_OUT  "../tubes/tube_"

// Structure de données transféré dans le shm.
typedef struct transfer {
	volatile int flag;
	volatile 
} transfer;

#endif
