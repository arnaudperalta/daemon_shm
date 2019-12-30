#ifndef DEMON__H
#define DEMON__H

#include "thread_manager.h"

typedef struct config config;

// Fonction qui charge le fichier de configuration en mémoire.
//    Renvoie -1 en cas d'erreur, 0 en cas de succès
extern int load_config(config *cfg);

// Fonction executée lorsque le démon passe en mode écoute.
extern int tube_listening(size_t shm_size);

#endif
