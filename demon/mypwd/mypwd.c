#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_PATH_LENGTH 512

int main(void) {
  char path[MAX_PATH_LENGTH] = "";
  char temp[MAX_PATH_LENGTH] = "";
  
  // Ouverture du répertoire courant
  DIR *cdr = opendir(".");
  if(cdr == NULL){
    exit(EXIT_FAILURE);
  }
  
  // Récupération de l'inode et du numéro de disque du répertoire courant
  struct stat st;
  long unsigned int inode, disknumber;
  if (stat(".", &st) != 0) {
      perror("stat");
      exit(EXIT_FAILURE);
  }
  inode = st.st_ino;
  disknumber = st.st_dev;
  
  // On ouvre le répertoire parent
  if (chdir("..") == -1) {
    exit(EXIT_FAILURE);
  }
  if (stat(".", &st) != 0) {
      perror("stat");
      exit(EXIT_FAILURE);
  }
  // On parcourt jusqu'au dossier racine
  while (inode != st.st_ino || disknumber != st.st_dev) {
    DIR *pdr = opendir(".");
    struct dirent *pdr_read = readdir(pdr);
    if (stat(pdr_read -> d_name, &st) != 0) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    while (inode != st.st_ino || disknumber != st.st_dev) {
      pdr_read = readdir(pdr);
      if (stat(pdr_read -> d_name, &st) != 0) {
        perror("stat");
        exit(EXIT_FAILURE);
      }
    }
    strcpy(temp,path);
    sprintf(path, "/%s", pdr_read -> d_name);
    strcat(path,temp);
    if (stat(".", &st) != 0) {
      perror("stat");
      exit(EXIT_FAILURE);
    }
    inode = st.st_ino;
    disknumber = st.st_dev;
    if (chdir("..") == -1) {
      exit(EXIT_FAILURE);
    }
    if (stat(".", &st) != 0) {
      perror("stat");
      exit(EXIT_FAILURE);
    }
  }
  printf("%s\n", path);
  
  return EXIT_SUCCESS;
}
