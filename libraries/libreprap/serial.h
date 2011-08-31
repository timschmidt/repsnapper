#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#endif

#ifndef WIN32
typedef int Serial;
#define SERIAL_INVALID_INIT (-1)
/* Return -1 for an invalid Serial instance, 0 if valid */
#define SERIAL_INVALID_CHECK(s) ((s) < 0 ? -1 : 0)
#else
typedef HANDLE Serial;
#define SERIAL_INVALID_INIT (NULL)
#define SERIAL_INVALID_CHECK(s) ((s) == NULL ? -1 : 0)
#endif

/* Opens serial port at path and speed.
 * Returns -1 on error, and allocated error
 * you need to free.
 */
Serial serial_open(const char *path, long speed, char **error);
/* Close a serial port. Can be called with an INVALID Serial instance */
int serial_close(Serial fd);

#endif
