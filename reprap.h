#include <sys/types.h>
#include <sys/select.h>

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

typedef enum {
  RR_PRIO_NORMAL,
  RR_PRIO_HIGH,
  RR_PRIO_COUNT
} rr_prio;

typedef void (*rr_callback)(char *);

typedef struct blocknode {
  char *block;
  struct blocknode *prev, *next;
} rr_blocknode;

#define RECVBUFSIZE 256
typedef struct {
  rr_proto proto;
  int fd;
  rr_blocknode *sendheads[RR_PRIO_COUNT];
  rr_blocknode *sendtails[RR_PRIO_COUNT];
  rr_blocknode *senthead;
  rr_blocknode *senttail;
  unsigned sentcached, maxsentcached;
  unsigned long lineno;
  char *recvbuf;
  size_t recvlen, recvsize;
  rr_callback sendcb, recvcb;
} rr_dev;

/* open/close return -1 and set errno on failure */
/* Initializes device with supplied params */
int rr_open(rr_dev *device, rr_proto proto, rr_callback sendcallback, rr_callback recvcallback, const char *port, long speed, size_t resend_cache_size);
/* Close port and deallocate buffers */
int rr_close(rr_dev *device);

/* Return to initial state */
void rr_reset(rr_dev *device);

void rr_enqueue(rr_dev *device, rr_prio priority, const char *block, size_t nbytes);

int rr_poll(rr_dev *device, const struct timeval *timeout);
