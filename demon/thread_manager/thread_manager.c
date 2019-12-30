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

#define MAX_ARG_COUNT 20
#define MAX_ARG_LENGTH 20
#define ARG_SEPARATOR " "

// Structure element stockant un thread avec son shm, son mutex etc ...
typedef struct thread_e {
  pthread_t thread;
  pthread_mutex_t mutex;
  size_t con_left; // Connection restante
  size_t shm_size;
  void *data;  // 0 de base, 1 pour une donnée reservé au demon, 2 pour le client
  volatile bool end; // Entier de controle qui indique la fin d'une connection
} thread_e;

// Structure principale stockant les threads_e
struct thread_m {
  thread_e **array;   // Tableau de thread_e de taille max_thread
  size_t count;       // Nombre de thread vivant
  size_t min_thread;  // Nombre de thread minimum
  size_t max_thread;  // Nombre de thread maximal
  size_t max_con;     // Nombre de connexion par thread
  size_t shm_size;
};

thread_e *ini_thread_element(size_t number, size_t max_con, size_t shm_size){
  thread_e *ptr = malloc(sizeof (thread_e));
  if (ptr == NULL) {
    return NULL;
  }
  if (pthread_mutex_init(&(ptr->mutex), NULL) != 0) {
    perror("Error pthread mutex");
    return NULL;
  }
  ptr->con_left = max_con;
  ptr->shm_size = shm_size;
  
  char shmname[SHM_INFO_LENGTH];
  sprintf(shmname, "%s%ld", SHM_NAME, number);
  
  int shm_fd = shm_open(shmname, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (shm_fd == FUN_FAILURE) {
    perror("Erreur création shm");
    return NULL;
  }

  // On fixe la taille de notre shm. SHM_SIZE
  if (ftruncate(shm_fd, (long int) shm_size) == FUN_FAILURE) {
    perror("Erreur truncate shm thread");
    return NULL;
  }

  ptr->data = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED
      , shm_fd, 0);
  if (ptr->data == MAP_FAILED) {
    perror("Erreur map");
    return NULL;
  }
  
  if (close(shm_fd) == FUN_FAILURE) {
    return NULL;
  }
  
  // Initialisation du point de rendez-vous et du flag d'arrêt
  ptr->end = false;
  int *flag = (int *) ptr->data;
  *flag = NEUTRAL_FLAG;
  
  // Lancement du thread
  pthread_create(&(ptr->thread), NULL, waiting_command, ptr);
  
  return ptr;
}


thread_m *ini_thread(size_t min_thread, size_t max_con, size_t max_thread
    , size_t shm_size) {
  // Allocation de la structure thread_m
  thread_m *ptr = malloc(sizeof (thread_m));
  if (ptr == NULL) {
    return NULL;
  }
  ptr->count = min_thread;
  ptr->min_thread = min_thread;
  ptr->max_thread = max_thread;
  ptr->shm_size = shm_size;
  // Allocation du tableau de pointeur de thread_e
  ptr->array = calloc(1, sizeof (thread_e *) * max_thread);
  if (ptr->array == NULL) {
    return NULL;
  }
  
  // Allocation de "min_thread" threads
  for (size_t i = 0; i < min_thread; ++i) {
    ptr->array[i] = ini_thread_element(i, max_con, shm_size);
    if (ptr->array[i] == NULL) {
      return NULL;
    }
  }
  
  return ptr;
}

void *waiting_command(void *arg) {
  thread_e *ptr = (thread_e *) arg;
  
  while (!ptr->end) {
    // Lecture d'une commande
    volatile int *flag = (volatile int *) ptr->data;
    char *data = (char *) ptr->data + sizeof *flag;
    // En attente d'une ressource destiné au thread ou d'une demande
    //  d'arrêt immédiat.
    while (*flag != DEMON_FLAG && !ptr->end);
    // Si c'est un arrêt immédiat, on s'arrête.
    if (ptr->end) return NULL;

    // Execution de la commande
    if (fork_execute(data, ptr->shm_size)
        == FUN_FAILURE) {
      perror("Erreur fork_execute");
      return NULL;
    }
    // On assigne la ressource écrite pour le client
    *flag = CLIENT_FLAG;
  }
  return NULL;
}

int use_thread(thread_m *th) {
  for (size_t i = 0; i < th->count; ++i) {
    // On verifie si ce thread est libre d'utilsation
    if (th->array[i] != NULL &&
        pthread_mutex_trylock(&(th->array[i]->mutex)) == 0) {
      // Un thread a été trouvé
      pthread_mutex_unlock(&(th->array[i]->mutex));
      --th->array[i]->con_left;
      return (int) i;
    }
  }
  // Pas de thread dispo, on regarde si on a atteint le max
  if (th->count < th->max_thread) {
    th->array[th->count] = ini_thread_element(th->count, th->max_con
        , th->shm_size);
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
    char shmname[SHM_INFO_LENGTH];
    sprintf(shmname, "%s%ld", SHM_NAME, number);
    free(th->array[number]->data);
    free(th->array[number]);
    th->array[number] = NULL;
    if (th->count == th->min_thread) {
      // Réinitialisation du thread
      th->array[number] = ini_thread_element(number, th->max_con, th->shm_size);
    }
    return FUN_SUCCESS;
  }
  if (th->array[number]->con_left > 1) {
    th->array[number]->con_left -= 1;
  }
  return FUN_SUCCESS;
}

int fork_execute(char *command, size_t size) {
  // Split des arguments dans un tableau de tableau de caractère
  char *arr[MAX_ARG_COUNT];
  size_t i = 0;
  char *token = strtok(command, ARG_SEPARATOR);
  while (token != NULL) {
    arr[i] = token;
    ++i;
    token = strtok(NULL, ARG_SEPARATOR);
  }
  arr[i] = NULL;
  
  int p[2];
  if (pipe(p) == FUN_FAILURE) {
    perror("Erreur pipe");
    return FUN_FAILURE;
  }
  switch (fork()) {
    case -1:
      perror("Erreur fork");
      return FUN_FAILURE;
    case 0:
      // On ne lira rien sur ce prosessus
      if (close(p[0]) == FUN_FAILURE) {
        perror("Erreur close fils");
        return FUN_FAILURE;
      }
      // Le file descriptor de stdout est écrasé par une copie du fd du pipe
      if (dup2(p[1], STDOUT_FILENO) == FUN_FAILURE) {
        perror("Erreur dup2 stdout");
        return FUN_FAILURE;
      }
      // Le file descriptor de stderr est écrasé par une copie du fd du pipe
      if (dup2(p[1], STDERR_FILENO) == FUN_FAILURE) {
        perror("Erreur dup2 stderr");
        return FUN_FAILURE;
      }
      execv(arr[0], arr);
      // La commande a échouée
      printf("Invalid command.\n");
      exit(EXIT_SUCCESS);
      break;
    default:
      // On n'écrira rien sur ce processus
      if (close(p[1]) == FUN_FAILURE) {
        return FUN_FAILURE;
      }
      ssize_t r = 0;
      if ((r = read(p[0], command, size)) == FUN_FAILURE) {
        perror("Erreur lecture pipe");
        return FUN_SUCCESS;
      }
      command[r] = '\0';
      break;
  }
  return FUN_SUCCESS;
}

void lock_thread(thread_m *th, size_t n) {
  pthread_mutex_lock(&(th->array[n]->mutex));
}

void unlock_thread(thread_m *th, size_t n) {
  pthread_mutex_unlock(&(th->array[n]->mutex));
}

void thread_dispose(thread_m *th) {
  for (size_t i = 0; i < th->count; ++i) {
    if (th->array[i] != NULL) {
      // On arrête le thread
      th->array[i]->end = true;
      // On le désalloue
      pthread_join(th->array[i]->thread, NULL);
      free(th->array[i]);
      th->array[i] = NULL;
    }
  }
  free(th->array);
  free(th);
  th = NULL;
}
