#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_SIZE 100

int open_stat(const struct stat st);

int main(void) {
  // Récupération du répertoire courant
  char cwd[100] = "";
  if(getcwd(cwd, MAX_SIZE) == NULL) {
    printf("getcwd");
    exit(EXIT_FAILURE);
  }
  DIR *dr = opendir(cwd);
  if(dr == NULL){
    exit(EXIT_FAILURE);
  }
  struct dirent *drf = readdir(dr);
  struct stat st;
  // Pour chaque fichier du répertoire on liste les statistiques
  while( drf != NULL) {
    stat(drf -> d_name, &st);
    open_stat(st);
    printf("nom du fichier: %s\n",drf -> d_name);
    drf = readdir(dr);
  }
  closedir(dr);
	exit(EXIT_SUCCESS);
}

int open_stat(const struct stat st) {
  printf("device : %ld\n", st.st_dev);
  printf("numéro i-noeud : %ld\n", st.st_ino);
  char file_type[40] = "";
  if (S_ISREG(st.st_mode)) {
    strcpy(file_type, "fichier régulier");
  }
  else if (S_ISDIR(st.st_mode)) {
    strcpy(file_type, "répertoire");
  }
  printf("mode :%s\n", file_type);
  printf("droit:");
  printf("%s", S_ISDIR(st.st_mode) ? "d" : "-");
  printf("%s", (st.st_mode & S_IRUSR) ? "r" : "-");
  printf("%s", (st.st_mode & S_IWUSR) ? "w" : "-");
  printf("%s", (st.st_mode & S_IXUSR) ? "x" : "-");
  printf("%s", (st.st_mode & S_IRGRP) ? "r" : "-");
  printf("%s", (st.st_mode & S_IWGRP) ? "w" : "-");
  printf("%s", (st.st_mode & S_IXGRP) ? "x" : "-");
  printf("%s", (st.st_mode & S_IROTH) ? "r" : "-");
  printf("%s", (st.st_mode & S_IWOTH) ? "w" : "-");
  printf("%s", (st.st_mode & S_IXOTH) ? "x" : "-");
  printf("\n");
  printf("nombre de liens: %ld\n", st.st_nlink);
  struct passwd* ps = getpwuid(st.st_uid);
  printf("propriétaire: %s\n",ps -> pw_name);
  struct group* gp = getgrgid(st.st_gid);
  printf("groupe: %s\n", gp -> gr_name);
  printf("taille du fichier: %ld octet(s)\n", st.st_size);
  printf("date dernière modification: %s",ctime(&(st.st_mtime)));
  
  return EXIT_SUCCESS;
}
