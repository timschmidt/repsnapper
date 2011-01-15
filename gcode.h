#ifndef _GCODE_H_
#define _GCODE_H_

#include <math.h>

#define GCODE_BLOCKSIZE 256

typedef struct {
  float x, y, z;
} point;

float dot(const point a, const point b);

float length(const point a);

float angle(const point a, const point b);

void vinc(point *a, const point b);

typedef struct gcword {
  char letter;
  float num;
} gcword;

typedef struct gcblock {
  struct gcblock *next;
  char *text;
  char optdelete;
  unsigned line, real_line, index;
  gcword *words;
  unsigned wordcnt;
} gcblock;

gcblock *parse_block(char *buffer, unsigned len);

#endif
