#include <stdio.h>
#include <stdlib.h>
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
  size_t con_left;
  int shm_fd;
  transfer *data;  // 0 de base, 1 pour une donnée reservé au demon, 2 pour le client
} thread_e;

// Structure principale stockant les threads_e
struct thread_m {
  thread_e **array;   // Tableau de thread_e de taille max_thread
  size_t count;      // Nombre de thread vivant
  size_t max_thread; // Nombre de thread maximal
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
  pthread_create(&(ptr->thread), NULL, waiting_command, ptr);
  ptr->con_left = max_con;
  
  char shmname[32];
  sprintf(shmname, "%s%ld", SHM_NAME, number);
  
  ptr->shm_fd = shm_open(shmname, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (ptr->shm_fd == -1) {
    perror("Erreur création shm");
    return NULL;
  }
  
  if (shm_unlink(shmname) == -1) {
    perror("Erreur unlink shm");
    return NULL;
  }
  // On fixe la taille de notre shm. SHM_SIZE
  if (ftruncate(ptr->shm_fd, sizeof (transfer)) == -1) {
    perror("Erreur truncate shm thread");
    return NULL;
  }
  
  ptr->data = mmap(NULL, sizeof (transfer), PROT_READ | PROT_WRITE, MAP_SHARED, ptr->shm_fd, 0);
  if (ptr->data == MAP_FAILED) {
    perror("Erreur map");
    return NULL;
  }
  
  // Initialisation du point de rendez-vous
  *(ptr->data->flag) = NEUTRAL_FLAG;
  return ptr;
}


thread_m *ini_thread(size_t min_thread, size_t max_con, size_t max_thread) {
  // Allocation de la structure thread_m
  thread_m *ptr = malloc(sizeof (thread_m));
  if (ptr == NULL) {
    return NULL;
  }
  ptr->count = min_thread;
  
  // Allocation du tableau de pointeur de thread_e
  ptr->array = malloc(sizeof (thread_e) * max_thread);
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
  
  // Lecture d'une commande
  while (*(ptr->data->flag) != DEMON_FLAG);
  sprintf(command, "%s", (char *) (ptr->data->command));
  
  // On lock le thread
  pthread_mutex_lock(&(ptr->mutex));
  // Execution
  res = popen(command, "r"); // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  // Envoi du résultat
  fgets((char *) (ptr->data->command), MAX_CMD_LENGTH, res);
  *(ptr->data->flag) = CLIENT_FLAG;
  
  // On unlock le thread
  pthread_mutex_unlock(&(ptr->mutex));
  return NULL;
}

int use_thread(thread_m *th, size_t max_con) {
  for (size_t i = 0; i < th->count; ++i) {
    // On verifie si ce thread est libre d'utilsation
    if (pthread_mutex_trylock(&(th->array[i]->mutex)) == 0) {
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
