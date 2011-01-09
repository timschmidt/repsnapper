#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <sys/types.h>

typedef enum serial_error {
	SERIAL_NO_ERROR = 0,
	SERIAL_INVALID_SPEED = 1,
	SERIAL_INVALID_FILEDESC = 2,
	SERIAL_SETTING_FAILED = 3,
	SERIAL_UNKNOWN_ERROR = 4
} serial_error;

extern int serial_errno;

/* Opens serial port at path and speed if possible, or NULL on error. */
int serial_open(const char *path, long speed);

/* Returns a human-readable interpretation of a failing serial_open
 * return value */
const char* serial_strerror(int errno);

#endif
