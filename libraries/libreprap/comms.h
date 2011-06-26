#ifndef _REPRAP_COMMS_H_
#define _REPRAP_COMMS_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_TERMINATOR "\r\n"
#define BLOCK_TERMINATOR_LEN 2
#define REPLY_TERMINATOR "\r\n"

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
  RR_PRIO_COUNT,
  RR_PRIO_SENTCACHE, // internal impl. detail
  RR_PRIO_ALL_QUEUES
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

/* Got a reply we can parse, of type 'type' and value 'value' */
typedef void (*rr_reply_fn)   (rr_dev dev, int type, float value,
			       void *expansion, void *closure);
/* Called when we should write more to the device */
typedef void (*rr_more_fn)    (rr_dev dev, void *closure);
/* Called on error - with a suitable message etc. */
typedef void (*rr_error_fn)   (rr_dev dev, int error_code, const char *msg, size_t len, void *closure);
/* Called when we cannot write more, to wait for buffer space */
typedef void (*rr_wait_wr_fn) (rr_dev dev, int wait, void *closure);
/* We sent or recieved some data - tell the world so we can log it */
typedef void (*rr_log_fn)     (rr_dev dev, const char *buffer, size_t len, void *closure);

/* Initializes device with supplied params */
/* Note that want_writable may be called redundantly. */
rr_dev rr_dev_create (rr_proto   proto,
		      /* how many commands can we push */
		      size_t     dev_cmdqueue_size,
		      /* callbacks cf. above */
		      rr_reply_fn   reply_cb,   void *reply_cl,
		      rr_more_fn    more_cb,    void *more_cl,
		      rr_error_fn   error_cb,   void *error_cl,
		      /* notify when socket is writeable */
		      rr_wait_wr_fn wait_wr_cb, void *wait_wr_cl,
		      /* optional (or NULL, NULL) */
		      rr_log_fn     opt_log_cb, void *opt_log_cl);
void rr_dev_free     (rr_dev dev);

int  rr_dev_open  (rr_dev dev, const char *port, long speed);
int  rr_dev_close (rr_dev dev);
void rr_dev_reset_lineno (rr_dev dev);
void rr_dev_reset (rr_dev dev);

/* Accessors */
/* File descriptor; <0 if not connected */
int           rr_dev_fd     (rr_dev dev);

/* Number of lines that have been since open */
unsigned long rr_dev_lineno (rr_dev dev);

/* Returns nonzero if and only if there is data waiting to be written */
int           rr_dev_buffered (rr_dev dev);

/* Returns count of commands that are buffered & un-written */
int           rr_dev_buffered_lines (rr_dev dev);

/* Returns nonzero if dev can accept more data */
int           rr_dev_write_more (rr_dev dev);

/* Pause or resume a priority queue */
void          rr_dev_set_paused (rr_dev dev, int priority, int paused);

/* I/O abstraction for integration into
   system mainloops in combination with
   wait_wr_cb */

/* Call when there is data to read */
int rr_dev_handle_readable(rr_dev dev);
/* Call when there is space to write */
int rr_dev_handle_writable(rr_dev dev);

/* use to check if it is sensible to enqueue more commands */
int rr_dev_queue_size (rr_dev dev);

/* nbytes MUST be < GCODE_BLOCKSIZE, or -1 to for strlen(block)  */
/* returns non-zero on failure */
int rr_dev_enqueue_cmd (rr_dev dev, rr_prio priority, const char *block, int opt_nbytes);

/* Blocks until all buffered data has been written */
int rr_flush (rr_dev dev);

#ifdef __cplusplus
}
#endif

#endif
