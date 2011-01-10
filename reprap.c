#include "reprap.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "serial.h"
#include "gcode.h"

int rr_open(rr_dev *device, rr_proto proto, rr_callback sendcallback, rr_callback recvcallback, const char *port, long speed, size_t resend_cache_size) {
  device->fd = serial_open(port, speed);
  if(device->fd < 0) {
    return -1;
  }
  device->proto = proto;
  device->recvcb = recvcallback;
  device->sendcb = sendcallback;
  device->lineno = 0;
  device->recvsize = RECVBUFSIZE;
  device->recvbuf = calloc(RECVBUFSIZE, sizeof(char));
  device->recvlen = 0;
  device->sendheads[RR_PRIO_NORMAL] = NULL;
  device->sendheads[RR_PRIO_HIGH] = NULL;
  device->sendtails[RR_PRIO_NORMAL] = NULL;
  device->sendtails[RR_PRIO_HIGH] = NULL;
  device->senthead = NULL;
  device->senttail = NULL;
  device->sentcached = 0;
  device->maxsentcached = resend_cache_size;
  return 0;
}

int rr_close(rr_dev *device) {
  /* Deallocate buffers and lists */
  free(device->recvbuf);
  while(device->sendheads[RR_PRIO_HIGH]) {
    rr_blocknode *next = device->sendheads[RR_PRIO_HIGH]->next;
    free(device->sendheads[RR_PRIO_HIGH]->block);
    free(device->sendheads[RR_PRIO_HIGH]);
    device->sendheads[RR_PRIO_HIGH] = next;
  }
  while(device->sendheads[RR_PRIO_NORMAL]) {
    rr_blocknode *next = device->sendheads[RR_PRIO_NORMAL]->next;
    free(device->sendheads[RR_PRIO_NORMAL]->block);
    free(device->sendheads[RR_PRIO_NORMAL]);
    device->sendheads[RR_PRIO_NORMAL] = next;
  }
  while(device->senttail) {
    rr_blocknode *next = device->senttail->next;
    free(device->senttail->block);
    free(device->senttail);
    device->senttail = next;
  }
  
  /* Close FD */
  return close(device->fd);
}

void rr_reset(rr_dev *device) {
  device->lineno = 0;
}

int _sendblock_simple(const rr_dev *device, const char *block) {
  char buf[GCODE_BLOCKSIZE];
  int result;
  result = snprintf(buf, GCODE_BLOCKSIZE, "%s\r\n", block);
  if(result >= GCODE_BLOCKSIZE) {
    return RR_E_BLOCK_TOO_LARGE;
  }

  result = write(device->fd, buf, result-1);
  if(device->sendcb) {
    device->sendcb(buf);
  }

  if(result < 0) {
    return RR_E_WRITE_FAILED;
  }

  return 0;
}

int _sendblock_fived_line(rr_dev *device, unsigned long line, const char *block) {
  int result;
  /* Send block */
  char buf[GCODE_BLOCKSIZE];
  char checksum = 0;
  result = snprintf(buf, GCODE_BLOCKSIZE, "N%ld %s", line, block);
  ssize_t i;
  for(i = 0; i < result; i++) {
    checksum ^= buf[i];
  }
  /* TODO: Is this whitespace needed? */
  result = snprintf(buf, GCODE_BLOCKSIZE, "N%ld %s *%d\r\n", line, block, checksum);
  if(result >= GCODE_BLOCKSIZE) {
    return RR_E_BLOCK_TOO_LARGE;
  }

  result = write(device->fd, buf, result-1);
  if(device->sendcb) {
    device->sendcb(buf);
  }
  
  if(result < 0) {
    return RR_E_WRITE_FAILED;
  }

  return 0;
}

int _sendblock_fived(rr_dev *device, const char *block) {
  int result = _sendblock_fived_line(device, device->lineno, block);
  ++(device->lineno);
  return result;
}

int _sendblock(rr_dev *device, rr_blocknode *node) {
  node->prev = NULL;
  node->next = device->senthead;
  if(device->senthead) {
    device->senthead->prev = node;
  } else {
    device->senttail = node;
  }
  device->senthead = node;
  ++(device->sentcached);
  
  if(device->sentcached > device->maxsentcached) {
    rr_blocknode *prev = device->senttail->prev;
    free(device->senttail->block);
    free(device->senttail);
    device->senttail = prev;
  }

  switch(device->proto) {
  case RR_PROTO_SIMPLE:
    return _sendblock_simple(device, node->block);

  case RR_PROTO_FIVED:
    return _sendblock_fived(device, node->block);

  default:
    return RR_E_UNSUPPORTED_PROTO;
  }
}

int _readreply_common(rr_dev *device, char **reply) {
  int result = read(device->fd,
                    device->recvbuf + device->recvlen,
                    device->recvsize - device->recvlen);
  if(result <= 0) {
    return result;
  }
  size_t end = device->recvlen + result;
  size_t i;
  for(i = device->recvlen; i < end; ++i) {
    if(device->recvbuf[i] == '\n') {
      *reply = calloc(i, sizeof(char));
      strncpy(*reply, device->recvbuf, i-1);
      if((*reply)[i-1] == '\r') {
        (*reply)[i-1] = '\0';
      } else {
        (*reply)[i] = '\0';
      }
      device->recvlen = end - i;
      memmove(device->recvbuf, device->recvbuf + i, device->recvlen);
      return 1;
    }
  }
  device->recvlen = end;
  if(end >= device->recvsize) {
    device->recvbuf = realloc(device->recvbuf, 2*device->recvsize);
  }
  *reply = NULL;
  return 0;
}

int _handlereply_fived(rr_dev *device, const char *reply) {
  if(strncmp(reply, "rs", 2) == 0) {
    long long badline = atoll(reply+3);
    unsigned long delta = device->lineno - badline - 1;
    if(delta > device->maxsentcached) {
      return -1;
    }
    rr_blocknode *line = device->senthead;
    for(; delta > 0; --delta) {
      line = line->next;
    }
    _sendblock_fived_line(device, badline, line->block);
  }
  return 0;
}

int _readreply(rr_dev *device) {
  char *reply;
  int result = _readreply_common(device, &reply);

  if(result <= 0) {
    return result;
  }

  switch(device->proto) {
  case RR_PROTO_FIVED:
    _handlereply_fived(device, reply);
    break;

  default:
    break;
  }

  if(device->recvcb) {
    device->recvcb(reply);
  }
  free(reply);
  
  return 0;
}

void rr_enqueue(rr_dev *device, rr_prio priority, const char *block, size_t nbytes) {
  rr_blocknode *node = malloc(sizeof(rr_blocknode));
  node->block = calloc(nbytes+1, sizeof(char));
  strncpy(node->block, block, nbytes);
  node->block[nbytes] = '\0';
  node->prev = NULL;
  node->next = NULL;
  
  if(!device->sendheads[priority]) {
    device->sendheads[priority] = node;
  } else {
    node->prev = device->sendtails[priority];
    device->sendtails[priority]->next = node;
  }
  device->sendtails[priority] = node;
}

int timeval_compare(struct timeval x,
                    struct timeval y) {
  if(x.tv_sec < y.tv_sec)
    return -1;                  /* Less than. */
  else if(x.tv_sec > y.tv_sec)
    return 1;                   /* Greater than. */
  else if(x.tv_usec < y.tv_usec)
    return -1;                  /* Less than. */
  else if(x.tv_usec > y.tv_usec)
    return 1;                   /* Greater than. */
  else
    return 0;                   /* Equal. */
}

struct timeval timeval_subtract(struct timeval x,
                                struct timeval y) {
  struct timeval result;
  if ((x.tv_sec < y.tv_sec) ||
      ((x.tv_sec == y.tv_sec) &&
       (x.tv_usec <= y.tv_usec))) {
    /* x <= y */
    result.tv_sec = result.tv_usec = 0;
  } else {
    result.tv_sec = x.tv_sec - y.tv_sec;
    if (x.tv_usec < y.tv_usec) {
      result.tv_usec = x.tv_usec + 1000000L - y.tv_usec;
      result.tv_sec--;				/* Borrow a second. */
    } else {
      result.tv_usec = x.tv_usec - y.tv_usec;
    }
  }
  return result;
}

int rr_poll(rr_dev *device, const struct timeval *timeout) {
  fd_set reads, writes, errors;
  struct timeval now, start, elapsed, real_timeout;

  gettimeofday(&start, NULL);
  
  FD_ZERO(&reads);
  if(device->sendheads[RR_PRIO_HIGH] || device->sendheads[RR_PRIO_NORMAL]) {
    FD_ZERO(&writes);
  }
  FD_ZERO(&errors);
  
  while(1) {
    FD_SET(device->fd, &reads);
    FD_SET(device->fd, &writes);
    FD_SET(device->fd, &errors);

    /* Check/update timeout */
    gettimeofday(&now, NULL);
    elapsed = timeval_subtract(now, start);
    if(timeval_compare(*timeout, elapsed) <= 0) {
      return 0;
    }
    real_timeout = timeval_subtract(*timeout, elapsed);
    
    int rc = select(device->fd + 1, &reads, &writes, &errors, &real_timeout);

    if(rc == 0) {
      return 0;
    } else if(rc < 0) {
      return rc;
    }

    if(FD_ISSET(device->fd, &reads)) {
      _readreply(device);
    }
    if(FD_ISSET(device->fd, &writes)) {
      if(device->sendheads[RR_PRIO_HIGH]) {
        _sendblock(device, device->sendheads[RR_PRIO_HIGH]);
        device->sendheads[RR_PRIO_HIGH] = device->sendheads[RR_PRIO_HIGH]->next;
      } else if(device->sendheads[RR_PRIO_NORMAL]) {
        _sendblock(device, device->sendheads[RR_PRIO_NORMAL]);
        device->sendheads[RR_PRIO_NORMAL] = device->sendheads[RR_PRIO_NORMAL]->next;
      }
    }
    if(FD_ISSET(device->fd, &errors)) {
      /* TODO: Provide more error info */
      return -1;
    }
  }
  
  return 0;
}
