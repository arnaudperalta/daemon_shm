#ifndef DEFS__H
#define DEFS__H

// Fichier Header contenant des définitions communes au client et au démon
#define TUBE_NAME_LENGTH 32
#define SHM_INFO_LENGTH  32
#define PID_LENGTH       7
#define THR_LENGTH       5
#define MSG_LENGTH       4
#define FUN_SUCCESS      0
#define FUN_FAILURE      -1

// Flag décrivant la nature des données contenu dans le shm.
#define NEUTRAL_FLAG 0 // SHM Initialisée
#define DEMON_FLAG   1 // Client a entré la commande
#define CLIENT_FLAG  2 // Client peut récupérer le resultat

#define SYNC_MSG  "SYNC"
#define END_MSG   "END"
#define RST_MSG   "RST"
#define SHM_NAME  "/shm_thread_"  //dans /dev/shm/
#define TUBE_IN   "../tubes/daemon_in"
#define TUBE_OUT  "../tubes/tube_"

#endif
