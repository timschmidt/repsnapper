#ifndef _REPRAP_COMMS_H_
#define _REPRAP_COMMS_H_

#include <sys/types.h>

#define BLOCK_TERMINATOR "\r\n"
#define BLOCK_TERMINATOR_LEN 2
#define REPLY_TERMINATOR "\r\n"

#define RECVBUFSIZE 256
/* Do not change */
#define SENDBUFSIZE (GCODE_BLOCKSIZE + BLOCK_TERMINATOR_LEN)

typedef enum {
  /* Standard gcode, 'ok' response */
  RR_PROTO_SIMPLE,
  /* Simple + line numbers, checksums, mandatory feedrate */
  RR_PROTO_FIVED,
} rr_proto;

typedef enum {
  RR_E_BLOCK_TOO_LARGE = -1,
  RR_E_WRITE_FAILED = -2,
  RR_E_UNSUPPORTED_PROTO = -3,
} rr_error;

/* Must be ordered ascending by priority */
typedef enum {
  RR_PRIO_NORMAL = 0,
  RR_PRIO_HIGH,
  RR_PRIO_RESEND,
  RR_PRIO_COUNT
} rr_prio;

typedef struct rr_dev_t *rr_dev;

/* Device, callback user data, gcode block user data, actual block
 * sent, length thereof */
typedef void (*rr_sendcb)(rr_dev, void *, void *, const char *, size_t);
/* Device, callback user data, line received, length thereof */
typedef void (*rr_recvcb)(rr_dev, void *, const char *, size_t);
/* Device, callback user data, boolean */
typedef void (*rr_boolcb)(rr_dev, void *, char);

/* Initializes device with supplied params */
/* Note that want_writable may be called redundantly. */
rr_dev rr_create(rr_proto proto,
                 rr_sendcb onsend, void *onsend_data,
                 rr_recvcb onrecv, void *onrecv_data,
                 rr_boolcb want_writable, void *ww_data,
                 size_t resend_cache_size);
int rr_open(rr_dev device, const char *port, long speed);
/* Close port and deallocate buffers */
int rr_close(rr_dev device);

/* Accessors */
/* File descriptor */
int rr_dev_fd(rr_dev device);
/* Number of lines that have been since open */
unsigned long rr_dev_lineno(rr_dev device);

/* nbytes MUST be < GCODE_BLOCKSIZE */
void rr_enqueue(rr_dev device, rr_prio priority, void *cbdata, const char *block, size_t nbytes);

int rr_handle_readable(rr_dev device);
/* Should only be called if want_writable callback has most recently
 * been passed a nonzero second argument */
int rr_handle_writable(rr_dev device);

#endif
