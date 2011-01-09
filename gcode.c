#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "gcode.h"

/* Finds the next non-whitespace character. */
int next_dark(char *buffer, size_t len, size_t i) {
  for(; i < len; ++i) {
    switch(buffer[i]) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
      break;
      
    default:
      return i;
    }
  }
  return len;
}

gcblock *parse_block(char *buffer, unsigned len) {
  size_t i = next_dark(buffer, len, 0);
  if(i == len) {
    return NULL;
  }

  gcblock *block = malloc(sizeof(gcblock));
  block->next = NULL;
  block->line = 0;
  block->optdelete = 0;
  block->words = NULL;
  block->wordcnt = 0;
  /* We can't fill these in here */
  block->index = 0;
  block->real_line = 0;

  /* Check for optional delete */
  if(buffer[i] == '/') {
    block->optdelete = 1;
    i = next_dark(buffer, len, i + 1);
  }

  /* Check for line number */
  if((buffer[i] == 'N' || buffer[i] == 'n') && (i + 1) < len) {
    i = next_dark(buffer, len, len);
    char* endptr;
    block->line = strtol(buffer + i, &endptr, 10);
    i = next_dark(buffer, len, endptr - buffer);
  }

  /* Parse words */
  unsigned allocsize = 0;
  while(i < len) {
    /* Skip comments */
    while(buffer[i] == '(') {
      for(; i < len && buffer[i] != ')'; ++i);
      i = next_dark(buffer, len, i + 1);
      if(i >= len) {
        return block;
      }
    }
    if(buffer[i] == ';') {
      return block;
    }
      
    if(block->wordcnt == allocsize) {
      allocsize = 2*(allocsize ? allocsize : 2);
      block->words = realloc(block->words, allocsize * sizeof(gcword));
    }
    
    block->words[block->wordcnt].letter = toupper(buffer[i]);
    i = next_dark(buffer, len, i + 1);
    if(i == len) {
      free(block->words);
      free(block);
      return NULL;
    }

    /* TODO: Spaces in numbers */
    /* TODO: Gn.m support */
    {
      char *endptr;
      
      block->words[block->wordcnt].num = strtof(buffer + i, &endptr);
      
      if(endptr == buffer + i) {
        free(block->words);
        free(block);
        return NULL;
      }

      i = next_dark(buffer, len, endptr - buffer);
    }
    ++(block->wordcnt);
  }

  return block;
}

float dot(const point a, const point b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

float length(const point a) {
  return sqrt(dot(a, a));
}

float angle(const point a, const point b) {
  return acosf(dot(a, b) / (length(a) * length(b)));
}

void vinc(point *a, const point b) {
  a->x += b.x;
  a->y += b.y;
  a->z += b.z;
}

point vmul(const float a, const point b) {
  point ret = {a*b.x, a*b.y, a*b.z};
  return ret;
}
