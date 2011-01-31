#ifndef _COMMS_PRIVATE_H_
#define _COMMS_PRIVATE_H_

#include "gcode.h"

/* Do not change */
#define SENDBUFSIZE (GCODE_BLOCKSIZE + BLOCK_TERMINATOR_LEN)

typedef struct blocknode {
  struct blocknode *next;
  void *cbdata;
  char *block;
  size_t blocksize;
  long long line;
} blocknode;

void blocknode_free(blocknode *node);

struct rr_dev_t {
  rr_proto proto;
  int fd;
  /* Line currently being sent */
  unsigned long lineno;

  blocknode *sendhead[RR_PRIO_COUNT];
  blocknode *sendtail[RR_PRIO_COUNT];
  char sendbuf[SENDBUFSIZE];
  rr_prio sending_prio;
  size_t sendbuf_fill;
  size_t bytes_sent;
  
  char *recvbuf;
  size_t recvbufsize;
  size_t recvbuf_fill;
  
  blocknode **sentcache;
  size_t sentcachesize;
  
  rr_sendcb onsend;
  rr_recvcb onrecv;
  rr_replycb onreply;
  rr_errcb onerr;
  rr_boolcb want_writable;
  void *onsend_data, *onrecv_data, *onreply_data, *onerr_data, *ww_data;
};

void rr_enqueue_internal(rr_dev device, rr_prio priority, void *cbdata, const char *block, size_t nbytes, long long line);

#endif
