#include "util.h"

#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

#define DEV_PATH "/dev/"
#define DEV_PREFIXES ((char*[]){"ttyUSB", "ttyACM", NULL})
#define GUESSES 8

char **rr_enumerate_ports() {
  size_t size = 4, fill = 0;
  char **ports = malloc(size * sizeof(char*));
  DIR *devdir = opendir(DEV_PATH);
  if(!devdir) {
    return NULL;
  }
  
  struct dirent *file;
  while((file = readdir(devdir))) {
    size_t i;
    for(i = 0; DEV_PREFIXES[i]; ++i) {
      char *prefix = DEV_PREFIXES[i];
      if(!strncmp(file->d_name, prefix, strlen(prefix))) {
        /* TODO: Open connection and interrogate device */
        if(fill >= size) {
          size *= 2;
          ports = realloc(ports, size * sizeof(char*));
        }
        ports[fill] = malloc(strlen(file->d_name)+strlen(DEV_PATH)+1);
        strcpy(ports[fill], DEV_PATH);
        strcat(ports[fill], file->d_name);
        ++fill;
      }
    }
  }
  closedir(devdir);

  if(fill >= size) {
    ++size;
    ports = realloc(ports, size * sizeof(char*));
  }
  ports[fill] = NULL;
  
  return ports;
}
