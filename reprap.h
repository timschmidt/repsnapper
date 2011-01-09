#include <sys/types.h>

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

typedef struct {
  rr_proto proto;
  int fd;
  unsigned long lineno;
} rr_dev;

/* All return -1 and set errno on failure */

/* Initializes device with supplied params */
int rr_open(rr_dev *device, rr_proto proto, const char *port, long speed);

int rr_sendblock(const rr_dev *device, const char *block, size_t nbytes);

int rr_close(const rr_dev *device);
