#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

void copy_file(FILE* src, FILE* dest, size_t b, size_t e);

int main(int argc, char ** argv) {
	int opt;
  char s_path[100] = "", d_path[100] = "";
  size_t b = 0, e = 0; 
  char write_type[2] = "w";
  FILE* s_f;
  FILE* d_f;
  bool v = false;
  if(argc < 3) {
    exit(EXIT_FAILURE);
  }
	while ((opt = getopt(argc, argv, "vabe")) !=-1) {
		switch(opt) {
			case('v'):
        v = true;
			  break;
      case('a'):
        if(v) {
          v = false;
        }
        strcpy(write_type, "a");
        break;
      case('b'):
        if(sscanf(argv[optind],"%zu", &b) != 1) {
          exit(EXIT_FAILURE);
        }
        break;
      case('e'):
        if(sscanf(argv[optind],"%zu", &e) != 1) {
          exit(EXIT_FAILURE);
        }
        break;
			default:
			  break;
		  }
	}
  strcpy(s_path, argv[optind]);
  strcpy(d_path, argv[optind + 1]);
  s_f = fopen(s_path, "r");
  if(s_f == NULL) {
    exit(EXIT_FAILURE);
  }
  d_f = fopen(d_path,"r");
  if(d_f != NULL) {
    if(v) {
      fclose(s_f);
      exit(EXIT_FAILURE);
    }
  }
  d_f = fopen(d_path,write_type);
  copy_file(s_f, d_f, b, e);
  fclose(s_f);
  fclose(d_f);
	
  return EXIT_SUCCESS;
}

void copy_file(FILE* src, FILE* dest, size_t b, size_t e) {
  long lSize;
  int c;
  fseek(src , 0 , SEEK_END);
  lSize = ftell(src);
  rewind (src);
  lSize = lSize - (long int) b;
  lSize = lSize - (long int) e;
  fseek(src , (long int) b , SEEK_CUR);
  c = fgetc(src);
  while(c != EOF && lSize > 0) {
    fputc(c, dest);
    c = fgetc(src);
    --lSize;
  }
}
