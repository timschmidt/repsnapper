#include "common.h"

#include "comms_private.h"

int resend(rr_dev device, unsigned long lineno, const char *reply, size_t nbytes) {
  unsigned long delta = (device->lineno - 1) - lineno;
  if(delta < device->sentcachesize) {
    blocknode *node = device->sentcache[delta];
    rr_enqueue_internal(device, RR_PRIO_RESEND, node->cbdata, node->block, node->blocksize, lineno);
  } else {
    /* Line needed for resend was not cached */
    if(device->onerr) {
      device->onerr(device, device->onerr_data, RR_E_UNCACHED_RESEND, reply, nbytes);
    }
    return RR_E_UNCACHED_RESEND;
  }
  
  return 0;
}
