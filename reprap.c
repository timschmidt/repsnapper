#include "reprap.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "serial.h"
#include "gcode.h"

typedef struct rr_blocknode {
  struct rr_blocknode *next;
  void *cbdata;
  const char *block;
  size_t blocksize;
} rr_blocknode;

struct rr_dev_t {
  rr_proto proto;
  int fd;
  unsigned long lineno;

  rr_blocknode *sendhead[RR_PRIO_COUNT];
  rr_blocknode *sendtail[RR_PRIO_COUNT];
  size_t bytes_sent;
  
  char *recvbuf;
  
  char **sentcache;
  size_t sentcachesize;
  
  rr_sendcb onsend;
  rr_recvcb onrecv;
  rr_boolcb want_writable;
  void *onsend_data, *onrecv_data, *ww_data;
};

int rr_open(rr_dev *deviceptr,
            rr_proto proto,
            rr_sendcb onsend, void *onsend_data,
            rr_recvcb onrecv, void *onrecv_data,
            rr_boolcb want_writable, void *ww_data,
            const char *port, long speed,
            size_t resend_cache_size) {
  *deviceptr = malloc(sizeof(struct rr_dev_t));
  rr_dev device = *deviceptr;

  device->sentcache = calloc(resend_cache_size, sizeof(char*));
  device->sentcachesize = resend_cache_size;
  unsigned i;
  for(i = 0; i < resend_cache_size; ++i) {
    device->sentcache[i] = NULL;
  }
  for(i = 0; i < RR_PRIO_COUNT; ++i) {
    device->sendhead[i] = NULL;
    device->sendtail[i] = NULL;
  }
  device->recvbuf = calloc(RECVBUFSIZE, sizeof(char));
  
  device->fd = serial_open(port, speed);
  if(device->fd < 0) {
    return -1;
  }
  
  device->proto = proto;
  device->onsend = onsend;
  device->onsend_data = onsend_data;
  device->onrecv = onrecv;
  device->onrecv_data = onrecv_data;
  device->want_writable = want_writable;
  device->ww_data = ww_data;
  device->lineno = 0;
  return 0;
}

int rr_close(rr_dev device) {
  /* Deallocate buffers */
  unsigned i;
  for(i = 0; i < RR_PRIO_COUNT; ++i) {
    rr_blocknode *j = device->sendhead[i];
    while(j != NULL) {
      rr_blocknode *next = j->next;
      free(j);
      j = next;
    }
    free(device->sendhead[i]);
  }
  free(device->recvbuf);
  for(i = 0; i < device->sentcachesize; ++i) {
    if(device->sentcache[i]) {
      free(device->sentcache[i]);
    }
  }
  free(device->sentcache);
  
  /* Close FD */
  return close(device->fd);
}

int fmtblock_simple(char *buf, const char *block) {
  /* \r\n\0 is 3 bytes */
  char work[GCODE_BLOCKSIZE+3];
  int result;
  result = snprintf(work, GCODE_BLOCKSIZE+3, "%s\r\n", block);
  if(result >= GCODE_BLOCKSIZE+3) {
    return RR_E_BLOCK_TOO_LARGE;
  }
  int len = (result > GCODE_BLOCKSIZE+2) ? result - 1 : result;
  strncpy(buf, work, len);

  return len;
}

int fmtblock_fived(char *buf, const char *block, unsigned long line) {
  /* \r\n\0 is 3 bytes */
  char work[GCODE_BLOCKSIZE+3];
  int result;
  char checksum = 0;
  result = snprintf(work, GCODE_BLOCKSIZE+1, "N%ld %s", line, block);
  if(result >= GCODE_BLOCKSIZE+1) {
    return RR_E_BLOCK_TOO_LARGE;
  }
  ssize_t i;
  for(i = 0; i < result; ++i) {
    checksum ^= work[i];
  }
  /* TODO: Is this whitespace needed? */
  result = snprintf(work, GCODE_BLOCKSIZE+3, "N%ld %s *%d\r\n", line, block, checksum);
  if(result >= GCODE_BLOCKSIZE+3) {
    return RR_E_BLOCK_TOO_LARGE;
  }

  int len = (result > GCODE_BLOCKSIZE+2) ? result - 1 : result;
  strncpy(buf, work, len);

  return len;
}

int fmtblock(rr_dev device, char *buf, const char *block) {
  switch(device->proto) {
  case RR_PROTO_SIMPLE:
    return fmtblock_simple(buf, block);

  case RR_PROTO_FIVED:
    return fmtblock_fived(buf, block, device->lineno);

  default:
    return RR_E_UNSUPPORTED_PROTO;
  }
}

void rr_enqueue(rr_dev device, rr_prio priority, void *cbdata, const char *block, size_t nbytes) {
  rr_blocknode *node = malloc(sizeof(rr_blocknode));
  node->next = NULL;
  node->cbdata = cbdata;
  node->block = block;
  node->blocksize = nbytes;
  
  if(!device->sendhead[priority]) {
    device->sendhead[priority] = node;
  } else {
    device->sendtail[priority]->next = node;
  }
}
