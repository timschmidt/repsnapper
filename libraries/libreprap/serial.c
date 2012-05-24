#include "serial.h"

#ifdef WIN32
/* Pull in win32 source file instead */
#include "serialwin32.c"
#else

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>


#ifdef __linux__
//#ifdef HAVE_ASM_IOCTLS_H
#include <asm/ioctls.h>
#include <sys/ioctl.h>
//#endif

struct linux_serial_struct {
	int	type;
	int	line;
	int	port;
	int	irq;
	int	flags;
	int	xmit_fifo_size;
	int	custom_divisor;
	int	baud_base;
	unsigned short	close_delay;
	char	io_type;
	char	reserved_char[1];
	int	hub6;
	unsigned short	closing_wait; /* time to wait before closing */
	unsigned short	closing_wait2; /* no longer used... */
	unsigned char	*iomem_base;
	unsigned short	iomem_reg_shift;
	int	reserved[2];
};
#define ASYNC_SPD_MASK	0x1030
#define ASYNC_SPD_CUST	0x0030  /* Use user-specified divisor */


linux_set_speed_custom(int fd, long baud){
  struct linux_serial_struct serinfo;
  serinfo.reserved_char[0] = 0;
  if (ioctl(fd, TIOCGSERIAL, &serinfo) < 0) {
    perror("Cannot get serial info from port");
    return -1;
  }
  int speed = (int)baud;
  /* printf("old baud_base %li divisor %li = %li\n",  */
  /* 	 serinfo.baud_base , serinfo.custom_divisor, */
  /* 	 serinfo.baud_base / serinfo.custom_divisor); */
  
  if ( (serinfo.baud_base / serinfo.custom_divisor) != speed ) {
    int divisor = (int)(serinfo.baud_base  / baud);
    int closestSpeed = (int)(serinfo.baud_base / divisor);
    if (closestSpeed < speed * 98 / 100 || closestSpeed > speed * 102 / 100) {
      printf("Cannot set serial port speed to %li. Closest possible is %li\n", 
	     speed, closestSpeed);
      return -1;
    }
    serinfo.custom_divisor = divisor;
    serinfo.flags = (serinfo.flags | ASYNC_SPD_CUST);
    printf("setting baud_base %li divisor %li = %li baud\n",
	   serinfo.baud_base, serinfo.custom_divisor,
	   (long)(serinfo.baud_base / serinfo.custom_divisor));
    if (ioctl(fd, TIOCSSERIAL, &serinfo) < 0) {
      perror("Cannot set serial speed");
      return -1;
    }
  }

#ifdef B38400
  return B38400; // this is the value used to connect
#else
  printf("could not set custom baud rate %i", baud);
  return -1;
#endif
}

#endif // __linux__


// Convert between the numeric speed and the termios representation
// thereof.  Returns < 0 if argument is an unsupported speed.
static speed_t ntocf(const long l) {
#ifdef __linux__
	switch(l) {
#ifdef B0
	case 0:
		return B0;
#endif
#ifdef B50
	case 50:
		return B50;
#endif
#ifdef B75
	case 75:
		return B75;
#endif
#ifdef B110
	case 110:
		return B110;
#endif
#ifdef B134
	case 134:
		return B134;
#endif
#ifdef B150
	case 150:
		return B150;
#endif
#ifdef B200
	case 200:
		return B200;
#endif
#ifdef B300
	case 300:
		return B300;
#endif
#ifdef B600
	case 600:
		return B600;
#endif
#ifdef B1200
	case 1200:
		return B1200;
#endif
#ifdef B1800
	case 1800:
		return B1800;
#endif
#ifdef B2400
	case 2400:
		return B2400;
#endif
#ifdef B4800
	case 4800:
		return B4800;
#endif
#ifdef B9600
	case 9600:
		return B9600;
#endif
#ifdef B19200
	case 19200:
		return B19200;
#endif
#ifdef B38400
	case 38400:
	  /* reset ??? */
	  /* ioctl(mHandle, TIOCGSERIAL, &ss); */
	  /* ss.flags &= ~ASYNC_SPD_MASK; */
	  /* ioctl(mHandle, TIOCSSERIAL, &ss); */
	  return B38400;
#endif
#ifdef B57600
	case 57600:
		return B57600;
#endif
#ifdef B115200
	case 115200:
		return B115200;
#endif
#ifdef B230400
	case 230400:
		return B230400;
#endif
#ifdef B460800
	case 460800:
		return B460800;
#endif
#ifdef B500000
	case 500000:
		return B500000;
#endif
#ifdef B576000
	case 576000:
		return B576000;
#endif
#ifdef B921600
	case 921600:
		return B921600;
#endif
#ifdef B1000000
	case 1000000:
		return B1000000;
#endif
#ifdef B1152000
	case 1152000:
		return B1152000;
#endif
#ifdef B1500000
	case 1500000:
		return B1500000;
#endif
#ifdef B2000000
	case 2000000:
		return B2000000;
#endif
#ifdef B2500000
	case 2500000:
		return B2500000;
#endif
#ifdef B3000000
	case 3000000:
		return B3000000;
#endif
#ifdef B3500000
	case 3500000:
		return B3500000;
#endif
#ifdef B4000000
	case 4000000:
		return B4000000;
#endif
	default:
	  printf( "non-standard baudrate %i\n",  l);
	  return -1;
	}
#else /* non-linux : */
	return (speed_t) l;
#endif // __linux__
}

// Repeated many times to allow errors to be isolated to the specific
// setting that failed to apply.  Returns < 0 on failure.
static int
serial_set_attrib(int fd, struct termios* attribp) {
	if(tcsetattr(fd, TCSANOW, attribp) < 0) {
		return -1;
        }
	return 0;
}

static int
serial_init(int fd, long speed, char **detail)
{
  int status;
  struct termios attribs;
  // Initialize attribs
  if(tcgetattr(fd, &attribs) < 0) {
    int tmp = errno;
    *detail = "not a serial device";
    close(fd);
    errno = tmp;
    return -1;
  }

  /* Handle software flow control bytes from machine */
  attribs.c_iflag |= IXOFF;
  serial_set_attrib(fd, &attribs);
  if((status = serial_set_attrib(fd, &attribs)) < 0) {
    *detail = "serial device has no flow control";
    return status;
  }

  /* Set speed */
  {
    speed_t cfspeed = ntocf(speed);
    if(cfsetispeed(&attribs, cfspeed) < 0) {
#ifdef __linux__
      if (linux_set_speed_custom(fd, speed) < 0){
	*detail = "can't set input speed";
	return -1;
      }
#else
      *detail = "can't set input speed";
      return -1;
#endif
    }
    serial_set_attrib(fd, &attribs);
    if(cfsetospeed(&attribs, cfspeed) < 0) {
      *detail = "can't set ouput speed";
      return -1;
    }
    serial_set_attrib(fd, &attribs);
  }
  
  /* Set non-canonical mode */
  attribs.c_cc[VTIME] = 0;
  if((status = serial_set_attrib(fd, &attribs)) < 0) {
    *detail = "can't set non-canonical mode (vtime)";
    return status;
  }
  attribs.c_cc[VMIN] = 0;
  if((status = serial_set_attrib(fd, &attribs)) < 0) {
    *detail = "can't set non-canonical mode (vmin)";
    return status;
  }
  cfmakeraw(&attribs);
  if((status = serial_set_attrib(fd, &attribs)) < 0) {
    *detail = "can't set raw mode";
    return status;
  }
  
  /* Prevents DTR from being dropped, resetting the MCU when using
   * an Arduino bootloader */
  attribs.c_cflag &= ~HUPCL;
  if((status = serial_set_attrib(fd, &attribs)) < 0) {
    *detail = "can't prevent DTR from being dropped";
    return status;
  }
  
  return 0;
}

/* Returns a prepared FD for the serial device specified, or some
 * value < 0 if an error occurred. */
Serial serial_open(const char *path, long speed, char **error)
{
  int fd;
  int flags;
  int status;
  char *detail = NULL;

  *error = NULL;
  do {
    fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
  } while(fd < 0 && errno == EINTR);
  if(fd < 0)
  {
    detail = "failed to open socket path";
    goto err_out;
  }

  if((status = serial_init(fd, speed, &detail)) < 0)
    goto err_out;

  if((flags = fcntl(fd, F_GETFL, 0)) < 0) {
    detail = "getting socket flags";
    goto err_out;
  }
  
  if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    detail = "setting non-blocking flag on socket";
    goto err_out;
  }
  return fd;
 err_out:
  {
    const char *errstr = strerror(errno);
    *error = malloc(strlen(detail) + strlen (path) + strlen(errstr) + 64);
    strcpy (*error, "Error opening port '");
    strcat (*error, path);
    strcat (*error, "' - ");
    strcat (*error, detail);
    strcat (*error, " : ");
    strcat (*error, errstr);
  }
  close (fd);
  return -1;
}

int
serial_close(Serial fd)
{
  int result;

  if (SERIAL_INVALID_CHECK(fd) >= 0) {
    do {
      result = close (fd);
    } while (result < 0 && errno == EINTR);
  }

  return result;
}


int
serial_setDTR(Serial fd, short dtr)
{
  /* printf("DTR %i\n",dtr); */
  char TIOCM_DTR_str[4];
  sprintf(TIOCM_DTR_str, "%u", TIOCM_DTR);
  if (dtr==1)
    ioctl(fd, TIOCMBIS, TIOCM_DTR_str);
  else if (dtr==0)
    ioctl(fd, TIOCMBIC, TIOCM_DTR_str);
  
}

#endif // !WIN32
