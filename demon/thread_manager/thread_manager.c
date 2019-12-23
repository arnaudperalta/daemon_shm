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
#include "defs.h"
#include "thread_manager.h"

// Structure element stockant un thread avec son shm, son mutex etc ...
typedef struct thread_e {
  pthread_t thread;
  pthread_mutex_t mutex;
  size_t con_left; // Connection restante
  transfer *data;  // 0 de base, 1 pour une donnée reservé au demon, 2 pour le client
  volatile bool end; // Entier de controle qui indique la fin d'une connection
} thread_e;

// Structure principale stockant les threads_e
struct thread_m {
  thread_e **array;   // Tableau de thread_e de taille max_thread
  size_t count;      // Nombre de thread vivant
  size_t min_thread; // Nombre de thread minimum
  size_t max_thread; // Nombre de thread maximal
  size_t max_con; // Nombre de connexion par thread
};

thread_e *ini_thread_element(size_t number, size_t max_con) {
  thread_e *ptr = malloc(sizeof (thread_e));
  if (ptr == NULL) {
    return NULL;
  }
  if (pthread_mutex_init(&(ptr->mutex), NULL) != 0) {
    perror("Error pthread mutex");
    return NULL;
  }
  ptr->con_left = max_con;
  
  char shmname[SHM_NAME_LENGTH];
  sprintf(shmname, "%s%ld", SHM_NAME, number);
  
  int shm_fd = shm_open(shmname, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (shm_fd == -1) {
    perror("Erreur création shm");
    return NULL;
  }

  // On fixe la taille de notre shm. SHM_SIZE
  if (ftruncate(shm_fd, sizeof ptr->data) == -1) {
    perror("Erreur truncate shm thread");
    return NULL;
  }

  ptr->data = mmap(NULL, sizeof ptr->data, PROT_READ | PROT_WRITE, 
      MAP_SHARED, shm_fd, 0);
  if (ptr->data == MAP_FAILED) {
    perror("Erreur map");
    return NULL;
  }
  
  if (close(shm_fd) == FUN_FAILURE) {
    return NULL;
  }
  
  // Initialisation du point de rendez-vous et du flag d'arrêt
  ptr->end = false;
  ptr->data->flag = NEUTRAL_FLAG;
  
  // Lancement du thread
  pthread_create(&(ptr->thread), NULL, waiting_command, ptr);
  
  return ptr;
}


thread_m *ini_thread(size_t min_thread, size_t max_con, size_t max_thread) {
  // Allocation de la structure thread_m
  thread_m *ptr = malloc(sizeof (thread_m));
  if (ptr == NULL) {
    return NULL;
  }
  ptr->count = min_thread;
  ptr->min_thread = min_thread;
  ptr->max_thread = max_thread;
  // Allocation du tableau de pointeur de thread_e
  ptr->array = calloc(1, sizeof (thread_e *) * max_thread);
  if (ptr->array == NULL) {
    return NULL;
  }
  
  // Allocation de "min_thread" threads
  for (size_t i = 0; i < min_thread; ++i) {
    ptr->array[i] = ini_thread_element(i, max_con);
    if (ptr->array[i] == NULL) {
      return NULL;
    }
  }
  
  return ptr;
}

void *waiting_command(void *arg) {
  thread_e *ptr = (thread_e *) arg;
  
  // Pointeur de fichier contenant le stdout de la commande a executé
  FILE *res;
  char command[MAX_CMD_LENGTH];
  
  while (!ptr->end) {
    // Lecture d'une commande
    while (ptr->data->flag != DEMON_FLAG);
    sprintf(command, "%s", (char *) (ptr->data->command));
    
    // On lock le thread
    pthread_mutex_lock(&(ptr->mutex));
    // Execution
    res = popen(command, "r"); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    // Envoi du résultat
    if (fgets((char *) (ptr->data->result), MAX_CMD_LENGTH, res) == NULL) {
      perror("Erreur fgets");
      return NULL;
    }
    ptr->data->flag = CLIENT_FLAG;
    
    // On unlock le thread
    pthread_mutex_unlock(&(ptr->mutex));
  }
  return NULL;
}

int use_thread(thread_m *th, size_t max_con) {
  for (size_t i = 0; i < th->max_thread; ++i) {
    // On verifie si ce thread est libre d'utilsation
    if (th->array[i] == NULL ||
        pthread_mutex_trylock(&(th->array[i]->mutex)) == 0) {
      // Un thread a été trouvé
      pthread_mutex_unlock(&(th->array[i]->mutex));
      --th->array[i]->con_left;
      return (int) i;
    }
  }
  // Pas de thread dispo, on regarde si on a atteint le max
  if (th->count < th->max_thread) {
    th->array[th->count] = ini_thread_element(th->count, max_con);
    ++th->count;
    return (int) th->count - 1;
  }
  
  return -1;
}

int consume_thread(thread_m *th, size_t number) {
  // On regarde si le thread à atteint sa fin de vie
  if (th->array[number]->con_left == 1) {
    // On indique au thread qu'il s'arrête totalement
    th->array[number]->end = true;
    if (pthread_join(th->array[number]->thread, NULL) != 0) {
      perror("pthread_join");
      return FUN_FAILURE;
    }
    char shmname[SHM_NAME_LENGTH];
    sprintf(shmname, "%s%ld", SHM_NAME, number);
    free(th->array[number]->data);
    free(th->array[number]);
    th->array[number] = NULL;
    if (th->count == th->min_thread) {
      // Réinitialisation du thread
      th->array[number] = ini_thread_element(number, th->max_con);
    }
    return FUN_SUCCESS;
  }
  if (th->array[number]->con_left > 1) {
    th->array[number]->con_left -= 1;
  }
  return FUN_SUCCESS;
}
