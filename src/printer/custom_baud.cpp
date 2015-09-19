#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_ASM_TERMBITS_H
#include <cstdlib>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/termbits.h>
#endif

bool set_custom_baudrate(int device_fd, int baudrate) {
#if HAVE_ASM_TERMBITS_H

  struct termios2 options;

  if (ioctl(device_fd, TCGETS2, &options) < 0) {
    return false;
  }

  options.c_cflag &= CBAUD;
  options.c_cflag |= BOTHER;

  options.c_ispeed = baudrate;
  options.c_ospeed = baudrate;

  if (ioctl(device_fd, TCSETS2, &options) < 0) {
    return false;
  }

  return true;

#else

  return false;

#endif
}
