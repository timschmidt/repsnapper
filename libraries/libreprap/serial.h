#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <sys/types.h>

/* Opens serial port at path and speed.
 * Returns -1 on error, and allocated error
 * you need to free.
 */
int serial_open(const char *path, long speed, char **error);

#endif
