#include "tonokip.h"

#include <stdlib.h>
#include <string.h>

#include "comms_private.h"

int tonokip_handle_reply(rr_dev device, const char *reply, size_t nbytes) {
  if(!strncmp("ok", reply, 2)) {
    if(device->onreply) {
      device->onreply(device, device->onreply_data, RR_OK, 0);
    }
  } else if(!strncmp("Resend:", reply, 7)) {
    /* Line number begins 7 bytes in */
    unsigned long resend = atoll(reply + 7);
    unsigned long delta = (device->lineno - 1) - resend;
    if(delta < device->sentcachesize) {
      blocknode *node = device->sentcache[delta];
      rr_enqueue_internal(device, RR_PRIO_RESEND, node->cbdata, node->block, node->blocksize, resend);
    } else {
      /* Line needed for resend was not cached */
      if(device->onerr) {
        device->onerr(device, device->onerr_data, RR_E_UNCACHED_RESEND, reply, nbytes);
      }
      return RR_E_UNCACHED_RESEND;
    }
  } else if(!strncmp("T:", reply, 2)) {
    if(device->onreply) {
      char *point;
      device->onreply(device, device->onreply_data, RR_BED_TEMP,
                      strtof(reply+2, &point));
      if(!strncmp("B:", point+1, 2)) {
        device->onreply(device, device->onreply_data, RR_BED_TEMP,
                        strtof(point+3, NULL));
      }
    }
  } else {
    if(device->onerr) {
      device->onerr(device, device->onerr_data, RR_E_UNKNOWN_REPLY, reply, nbytes);
    }
    return RR_E_UNKNOWN_REPLY;
  }

  return 0;
}
