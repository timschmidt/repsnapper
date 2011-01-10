#include "reprap.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "serial.h"
#include "gcode.h"

int rr_open(rr_dev *device, rr_proto proto, rr_recvcb recvcallback, const char *port, long speed) {
  device->fd = serial_open(port, speed);
  if(device->fd < 0) {
    return -1;
  }
  device->proto = proto;
  device->recvcb = recvcallback;
  device->lineno = 0;
  device->recvsize = RECVBUFSIZE;
  device->recvbuf = calloc(RECVBUFSIZE, sizeof(char));
  device->recvlen = 0;
  device->sendheads[RR_PRIO_NORMAL] = NULL;
  device->sendheads[RR_PRIO_HIGH] = NULL;
  device->sendtails[RR_PRIO_NORMAL] = NULL;
  device->sendtails[RR_PRIO_HIGH] = NULL;
  return 0;
}

int rr_close(rr_dev *device) {
  /* Deallocate memory */
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
  
  /* Close FD */
  return close(device->fd);
}

void rr_reset(rr_dev *device) {
  device->lineno = 0;
}

int _sendblock_simple(const rr_dev *device, const char *block) {
  size_t nbytes = strlen(block);
  if(nbytes >= GCODE_BLOCKSIZE) {
    return RR_E_BLOCK_TOO_LARGE;
  }
  int result;
  result = write(device->fd, block, nbytes);
  if(result >= 0) {
    result = write(device->fd, "\r\n", 2);
  }

  if(result < 0) {
    return RR_E_WRITE_FAILED;
  }

  return 0;
}

int _sendblock_fived(const rr_dev *device, const char *block) {
  int result;
  /* Send block */
  char buf[GCODE_BLOCKSIZE];
  char checksum = 0;
  result = snprintf(buf, GCODE_BLOCKSIZE, "N%ld %s", device->lineno, block);
  ssize_t i;
  for(i = 0; i < result; i++) {
    checksum ^= buf[i];
  }
  /* TODO: Is this whitespace needed? */
  result = snprintf(buf, GCODE_BLOCKSIZE, "N%ld %s *%d\r\n", device->lineno, block, checksum);
  if(result >= GCODE_BLOCKSIZE) {
    return RR_E_BLOCK_TOO_LARGE;
  }
  result = write(device->fd, buf, result);
  
  if(result < 0) {
    return RR_E_WRITE_FAILED;
  }

  return 0;
}

int _sendblock(const rr_dev *device, const char *block) {
  switch(device->proto) {
  case RR_PROTO_SIMPLE:
    return _sendblock_simple(device, block);

  case RR_PROTO_FIVED:
    return _sendblock_fived(device, block);

  default:
    return RR_E_UNSUPPORTED_PROTO;
  }
}

int _readreply(rr_dev *device) {
  read(device->fd, device->recvbuf, device->recvsize);
  /* TODO */
  return 0;
}

void rr_enqueue(rr_dev *device, rr_prio priority, const char *block, size_t nbytes) {
  rr_blocknode *node = malloc(sizeof(rr_blocknode));
  node->block = calloc(nbytes+1, sizeof(char));
  strncpy(node->block, block, nbytes);
  node->block[nbytes] = '\0';
  node->next = NULL;
  
  if(!device->sendheads[priority]) {
    device->sendheads[priority] = node;
  } else {
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
        _sendblock(device, device->sendheads[RR_PRIO_HIGH]->block);
        rr_blocknode *next = device->sendheads[RR_PRIO_HIGH]->next;
        free(device->sendheads[RR_PRIO_HIGH]->block);
        free(device->sendheads[RR_PRIO_HIGH]);
        device->sendheads[RR_PRIO_HIGH] = next;
      } else if(device->sendheads[RR_PRIO_NORMAL]) {
        _sendblock(device, device->sendheads[RR_PRIO_NORMAL]->block);
        rr_blocknode *next = device->sendheads[RR_PRIO_NORMAL]->next;
        free(device->sendheads[RR_PRIO_NORMAL]->block);
        free(device->sendheads[RR_PRIO_NORMAL]);
        device->sendheads[RR_PRIO_NORMAL] = next;
      }
    }
    if(FD_ISSET(device->fd, &errors)) {
      /* TODO: Provide more error info */
      return -1;
    }
  }
  
  return 0;
}
