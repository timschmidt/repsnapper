#include "reprap.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "serial.h"
#include "gcode.h"

int rr_open(rr_dev *device, rr_proto proto, const char *port, long speed) {
  device->proto = proto;
  device->fd = serial_open(port, speed);
  device->lineno = 0;
  if(device->fd < 0) {
    return -1;
  }
  return 0;
}

int rr_sendblock(const rr_dev *device, const char *block, size_t nbytes) {
  if(nbytes >= GCODE_BLOCKSIZE) {
    return RR_E_BLOCK_TOO_LARGE;
  }
  int result;
  switch(device->proto) {
  case RR_PROTO_SIMPLE:
    result = write(device->fd, block, nbytes);
    if(result >= 0) {
      result = write(device->fd, "\r\n", 2);
    }
    break;

  case RR_PROTO_FIVED: {
    char buf[GCODE_BLOCKSIZE];
    char cstr[GCODE_BLOCKSIZE];
    char checksum = 0;
    size_t i;
    for(i = 0; i < nbytes; i++) {
      checksum ^= block[i];
    }
    strncpy(cstr, block, nbytes);
    cstr[nbytes] = '\0';
    /* TODO: Is this whitespace needed? */
    result = snprintf(buf, GCODE_BLOCKSIZE, "N%ld %s *%d\r\n", device->lineno, cstr, checksum);
    if(result >= GCODE_BLOCKSIZE) {
      return RR_E_BLOCK_TOO_LARGE;
    }
    result = write(device->fd, buf, result);
    break;
  }

  default:
    return RR_E_UNSUPPORTED_PROTO;
  }
  if(result < 0) {
    return RR_E_WRITE_FAILED;
  }
  /* TODO: Confirm receipt and resend if needed */
  return 0;
}

int rr_close(const rr_dev *device) {
  return close(device->fd);
}
