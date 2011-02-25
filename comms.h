#ifndef _REPRAP_COMMS_H_
#define _REPRAP_COMMS_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_TERMINATOR "\r\n"
#define BLOCK_TERMINATOR_LEN 2
#define REPLY_TERMINATOR "\r\n"

#define RECVBUFSIZE 256

typedef enum {
  /* Standard gcode, 'ok' response */
  RR_PROTO_SIMPLE,
  /* Simple + line numbers, checksums, mandatory feedrate */
  RR_PROTO_FIVED,
  /* 5D with different syntax for some absurd reason */
  RR_PROTO_TONOKIP,
} rr_proto;

typedef enum {
  RR_E_BLOCK_TOO_LARGE = -1,
  RR_E_WRITE_FAILED = -2,
  RR_E_UNSUPPORTED_PROTO = -3,
  RR_E_UNKNOWN_REPLY = -4,
  RR_E_UNCACHED_RESEND = -5,
  RR_E_HARDWARE_FAULT = -6,
  RR_E_UNSENT_RESEND = -7,
  RR_E_MALFORMED_RESEND_REQUEST = -8,
} rr_error;

/* Must be ordered ascending by priority */
typedef enum {
  RR_PRIO_NORMAL = 0,
  RR_PRIO_HIGH,
  RR_PRIO_RESEND,
  RR_PRIO_COUNT
} rr_prio;

typedef enum {
  RR_OK,
  RR_NOZZLE_TEMP,
  RR_BED_TEMP,
  RR_X_POS,
  RR_Y_POS,
  RR_Z_POS,
  RR_E_POS,
} rr_reply;

typedef struct rr_dev_t *rr_dev;

/* Note that strings passed to callbacks may not be null terminated. */
/* Invoked when a complete block has been sent */
/* Device, callback user data, gcode block user data, actual block
 * sent, length thereof */
typedef void (*rr_sendcb)(rr_dev, void *, void *, const char *, size_t);
/* Invoked on each line received */
/* Device, callback user data, line received, length thereof */
typedef void (*rr_recvcb)(rr_dev, void *, const char *, size_t);
/* Invoked when a known reply is received and parsed */
/* Device, callback user data, reply code, reply value */
typedef void (*rr_replycb)(rr_dev, void *, rr_reply, float);
/* Device, callback user data, boolean */
typedef void (*rr_boolcb)(rr_dev, void *, char);
/* Invoked when a reply from the device indicates an error */
/* Device, callback user data, error code, message responsible, length
 * thereof */
typedef void (*rr_errcb)(rr_dev, void *, int, const char*, size_t);

/* Initializes device with supplied params */
/* Note that want_writable may be called redundantly. */
rr_dev rr_create(rr_proto proto,
                 rr_sendcb onsend, void *onsend_data,
                 rr_recvcb onrecv, void *onrecv_data,
                 rr_replycb onreply, void *onreply_data,
                 rr_errcb onerr, void *onerr_data,
                 rr_boolcb want_writable, void *ww_data,
                 size_t resend_cache_size);
int rr_open(rr_dev device, const char *port, long speed);
void rr_reset(rr_dev device);
int rr_close(rr_dev device);
/* Deallocate */
void rr_free(rr_dev device);


/* Accessors */
/* File descriptor; <0 if not connected */
int rr_dev_fd(rr_dev device);
/* Number of lines that have been since open */
unsigned long rr_dev_lineno(rr_dev device);
/* Returns nonzero if and only if there is data waiting to be written */
int rr_dev_buffered(rr_dev device);

/* nbytes MUST be < GCODE_BLOCKSIZE */
void rr_enqueue(rr_dev device, rr_prio priority, void *cbdata, const char *block, size_t nbytes);
#define rr_enqueue_c(d, p, cb, b) rr_enqueue(d, p, cb, b, strlen(b))

int rr_handle_readable(rr_dev device);
/* Should only be called if want_writable callback has most recently
 * been passed a nonzero second argument; is guaranteed to be a noop
 * otherwise */
int rr_handle_writable(rr_dev device);

/* Blocks until all buffered data has been written */
int rr_flush(rr_dev device);

#ifdef __cplusplus
}
#endif

#endif
