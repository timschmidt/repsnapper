#include "comms.h"
#include "comms_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "serial.h"
#include "gcode.h"
#include "fived.h"


void blocknode_free(blocknode* node) {
  free(node->block);
  free(node);
}

int rr_dev_fd(rr_dev device) {
  return device->fd;
}
unsigned long rr_dev_lineno(rr_dev device) {
  return device->lineno;
}
int rr_dev_buffered(rr_dev device) {
  unsigned i;
  for(i = 0; i < RR_PRIO_COUNT; ++i) {
    if(device->sendhead[i]) {
      return 1;
    }
  }
  
  return device->sendbuf_fill;
}

rr_dev rr_create(rr_proto proto,
                 rr_sendcb onsend, void *onsend_data,
                 rr_recvcb onrecv, void *onrecv_data,
                 rr_replycb onreply, void *onreply_data,
                 rr_errcb onerr, void *onerr_data,
                 rr_boolcb want_writable, void *ww_data,
                 size_t resend_cache_size) {
  rr_dev device = malloc(sizeof(struct rr_dev_t));

  device->sentcache = calloc(resend_cache_size, sizeof(blocknode*));
  device->sentcachesize = resend_cache_size;
  unsigned i;
  for(i = 0; i < resend_cache_size; ++i) {
    device->sentcache[i] = NULL;
  }
  for(i = 0; i < RR_PRIO_COUNT; ++i) {
    device->sendhead[i] = NULL;
    device->sendtail[i] = NULL;
  }
  device->sendbuf_fill = 0;
  device->bytes_sent = 0;
  device->recvbuf = calloc(RECVBUFSIZE, sizeof(char));
  device->recvbufsize = RECVBUFSIZE;
  device->recvbuf_fill = 0;
  device->proto = proto;
  device->onsend = onsend;
  device->onsend_data = onsend_data;
  device->onrecv = onrecv;
  device->onrecv_data = onrecv_data;
  device->onreply = onreply;
  device->onreply_data = onreply_data;
  device->onerr = onerr;
  device->onerr_data = onerr_data;
  device->want_writable = want_writable;
  device->ww_data = ww_data;
  device->lineno = 0;
  device->fd = -1;
  
  return device;
}

int rr_open(rr_dev device, const char *port, long speed) {
  device->fd = serial_open(port, speed);
  if(device->fd < 0) {
    return device->fd;
  }
  return 0;
}

void empty_buffers(rr_dev device) {
  unsigned i;
  for(i = 0; i < RR_PRIO_COUNT; ++i) {
    blocknode *j = device->sendhead[i];
    while(j != NULL) {
      blocknode *next = j->next;
      free(j);
      j = next;
    }
    free(device->sendhead[i]);
    device->sendhead[i] = NULL;
  }
  for(i = 0; i < device->sentcachesize; ++i) {
    if(device->sentcache[i]) {
      blocknode_free(device->sentcache[i]);
      device->sentcache[i] = NULL;
    }
  }
}

void rr_reset(rr_dev device) {
  empty_buffers(device);
  device->lineno = 0;
}

int rr_close(rr_dev device) {
  int result;
  do {
    result = close(device->fd);
  } while(result < 0 && errno == EINTR);
  device->fd = -1;
  rr_reset(device);
  return result;
}

void rr_free(rr_dev device) {
  empty_buffers(device);
  free(device->sentcache);
  free(device->recvbuf);
  free(device);
}

ssize_t fmtblock_simple(char *buf, const char *block) {
  char work[SENDBUFSIZE+1];
  int result;
  result = snprintf(work, SENDBUFSIZE+1, "%s" BLOCK_TERMINATOR, block);
  if(result >= SENDBUFSIZE+1) {
    return RR_E_BLOCK_TOO_LARGE;
  }
  size_t len = (result > SENDBUFSIZE) ? SENDBUFSIZE : result;
  strncpy(buf, work, len);

  return len;
}

ssize_t fmtblock_fived(char *buf, const char *block, unsigned long line) {
  char work[SENDBUFSIZE+1];
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
  result = snprintf(work, SENDBUFSIZE+1, "N%ld %s*%d" BLOCK_TERMINATOR, line, block, checksum);
  if(result >= SENDBUFSIZE+1) {
    return RR_E_BLOCK_TOO_LARGE;
  }

  size_t len = (result > SENDBUFSIZE) ? SENDBUFSIZE : result;
  strncpy(buf, work, len);

  return len;
}

ssize_t fmtblock(rr_dev device, blocknode *node) {
  char *terminated = calloc(node->blocksize + 1, sizeof(char));
  strncpy(terminated, node->block, node->blocksize);
  terminated[node->blocksize] = '\0';

  ssize_t result;
  switch(device->proto) {
  case RR_PROTO_SIMPLE:
    result = fmtblock_simple(device->sendbuf, terminated);
    break;

  case RR_PROTO_FIVED:
  case RR_PROTO_TONOKIP:
    if(node->line >= 0) {
      /* Block has an explicit line number; may be out of sequence
       * (i.e. a resend) */
      result = fmtblock_fived(device->sendbuf, terminated, node->line);
    } else {
      result = fmtblock_fived(device->sendbuf, terminated, device->lineno);
    }
    break;

  default:
    result = RR_E_UNSUPPORTED_PROTO;
    break;
  }

  free(terminated);
  return result;
}

void rr_enqueue_internal(rr_dev device, rr_prio priority, void *cbdata, const char *block, size_t nbytes, long long line) {
  blocknode *node = malloc(sizeof(blocknode));
  node->next = NULL;
  node->cbdata = cbdata;
  node->block = malloc(nbytes);
  memcpy(node->block, block, nbytes);
  node->blocksize = nbytes;
  node->line = line;
  
  if(!device->sendhead[priority]) {
    device->sendhead[priority] = node;
    device->want_writable(device, device->ww_data, 1);
  } else {
    device->sendtail[priority]->next = node;
  }
}

void rr_enqueue(rr_dev device, rr_prio priority, void *cbdata, const char *block, size_t nbytes) {
  rr_enqueue_internal(device, priority, cbdata, block, nbytes, -1);
}

int handle_reply(rr_dev device, const char *reply, size_t nbytes) {
  if(device->onrecv) {
    device->onrecv(device, device->onrecv_data, reply, nbytes);
  }

  /* TODO: Fixed and generalized parsing */
  /* All protos do this */
  if(!strncmp("ok", reply, 2)) {
    if(device->onreply) {
      device->onreply(device, device->onreply_data, RR_OK, 0);
      /* Parse reply */
      char *i;
      for(i = (char*)reply; i < reply + nbytes; ++i) {
        switch(*i) {
        case 'T':
          device->onreply(device, device->onreply_data, RR_NOZZLE_TEMP,
                          strtof(i+2, &i));
          break;

        case 'B':
          device->onreply(device, device->onreply_data, RR_BED_TEMP,
                          strtof(i+2, &i));
          break;

        case 'C':
          break;

        case 'X':
          device->onreply(device, device->onreply_data, RR_X_POS,
                          strtof(i+2, &i));
          break;

        case 'Y':
          device->onreply(device, device->onreply_data, RR_Y_POS,
                          strtof(i+2, &i));
          break;

        case 'Z':
          device->onreply(device, device->onreply_data, RR_Z_POS,
                          strtof(i+2, &i));
          break;

        case 'E':
          device->onreply(device, device->onreply_data, RR_E_POS,
                          strtof(i+2, &i));
          break;

        default:
          if(device->onerr) {
            device->onerr(device, device->onerr_data, RR_E_UNKNOWN_REPLY, reply, nbytes);
          }
          break;
        }
      }
    }
  }

  switch(device->proto) {
  case RR_PROTO_FIVED:
    return fived_handle_reply(device, reply, nbytes);
    break;

  default:
    return RR_E_UNSUPPORTED_PROTO;
  }

  return 0;
}

int rr_handle_readable(rr_dev device) {
  const size_t termlen = strlen(REPLY_TERMINATOR);
  /* Grow receive buffer if it's full */
  if(device->recvbuf_fill == device->recvbufsize) {
    device->recvbuf = realloc(device->recvbuf, 2*device->recvbufsize);
  }

  ssize_t result;
  size_t scan = (device->recvbuf_fill > termlen) ?
                 device->recvbuf_fill - termlen  :
                 0;
  do {
    result = read(device->fd, device->recvbuf + device->recvbuf_fill, device->recvbufsize - device->recvbuf_fill);
  } while(result < 0 && errno == EINTR);
  if(result < 0) {
    return result;
  }
  device->recvbuf_fill += result;

  /* Scan for complete reply */
  size_t start = 0;
  size_t end = device->recvbuf_fill - termlen;
  for(; scan < end; ++scan) {
    if(0 == strncmp(device->recvbuf + scan, REPLY_TERMINATOR, termlen)) {
      /* We have a terminator */
      handle_reply(device, device->recvbuf + start, scan - start);
      scan += termlen;
      start = scan;
    }
  }

  /* Move incomplete reply to beginning of buffer */
  memmove(device->recvbuf, device->recvbuf+start, device->recvbuf_fill - start);
  device->recvbuf_fill -= start;

  return 0;
}

int rr_handle_writable(rr_dev device) {
  ssize_t result;
  if(device->sendbuf_fill == 0) {
    /* Last block is gone; prepare to send a new block */
    int prio;
    blocknode *node = NULL;
    for(prio = RR_PRIO_COUNT - 1; prio >= 0; --prio) {
      node = device->sendhead[prio];
      if(node) {
        /* We have a block to send! Get it ready. */
        device->bytes_sent = 0;
        result = fmtblock(device, node);
        if(result < 0) {
          /* FIXME: This will confuse code expecting errno to be set */
          return result;
        }
        device->sendbuf_fill = result;
        device->sending_prio = prio;
        break;
      }
    }
    if(!node) {
      /* No data to write */
      device->want_writable(device, device->ww_data, 0);
      return 0;
    }
  }

  /* Perform write */
  do {
    result = write(device->fd, device->sendbuf + device->bytes_sent, device->sendbuf_fill - device->bytes_sent);
  } while(result < 0 && errno == EINTR);
  
  if(result < 0) {
    return result;
  }

  device->bytes_sent += result;

  if(device->bytes_sent == device->sendbuf_fill) {
    /* We've sent the complete block. */
    blocknode *node = device->sendhead[device->sending_prio];
    if(device->onsend) {
      device->onsend(device, device->onsend_data, node->cbdata, device->sendbuf, device->sendbuf_fill);
    }
    device->sendhead[device->sending_prio] = node->next;
    node->line = device->lineno;
    ++(device->lineno);

    /* Update sent cache */
    if(device->sentcache[device->sentcachesize - 1]) {
      blocknode_free(device->sentcache[device->sentcachesize - 1]);
    }
    ssize_t i;
    for(i = device->sentcachesize - 1; i > 0 ; --i) {
      device->sentcache[i] = device->sentcache[i-1];
    }
    device->sentcache[0] = node;

    /* Indicate that we're ready for the next. */
    device->sendbuf_fill = 0;
  }

  return result;
}

int rr_flush(rr_dev device) {
  /* Disable non-blocking mode */
  int flags;
  if((flags = fcntl(device->fd, F_GETFL, 0)) < 0) {
    return flags;
  }
  if(fcntl(device->fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
    return -1;
  }

  int result = 0;
  while(rr_dev_buffered(device) && result >= 0) {
    result = rr_handle_writable(device);
  }

  if(result >= 0) {
    result = fcntl(device->fd, F_SETFL, flags);
  } else {
    fcntl(device->fd, F_SETFL, flags);
  }

  /* Restore original mode */
  return result;
}
