#include "fived.h"

#include <stdlib.h>
#include <string.h>

#include "comms_private.h"

int fived_handle_reply(rr_dev device, const char *reply, size_t nbytes) {
  if(!strncmp("ok", reply, 2)) {
    if(device->onreply) {
      device->onreply(device, device->onreply_data, RR_OK, 0);
      /* Parse values */
      char *i;
      for(i = (char*)reply; i < reply + nbytes; ++i) {
        switch(*i) {
        case 'T':
          device->onreply(device, device->onreply_data, RR_NOZZLE_TEMP,
                          strtof(i+2, &i));
          break;

        case 'B':
          device->onreply(device, device->onreply_data, RR_BED_TEMP,
                          strtof(i+2, &i));
          break;

        case 'C':
          break;

        case 'X':
          device->onreply(device, device->onreply_data, RR_X_POS,
                          strtof(i+2, &i));
          break;

        case 'Y':
          device->onreply(device, device->onreply_data, RR_Y_POS,
                          strtof(i+2, &i));
          break;

        case 'Z':
          device->onreply(device, device->onreply_data, RR_Z_POS,
                          strtof(i+2, &i));
          break;

        case 'E':
          device->onreply(device, device->onreply_data, RR_E_POS,
                          strtof(i+2, &i));
          break;

        default:
          if(device->onerr) {
            device->onerr(device, device->onerr_data, RR_E_UNKNOWN_REPLY, reply, nbytes);
          }
          break;
        }
      }
    }
  } else if(!strncmp("rs", reply, 2)) {
    /* Line number begins 3 bytes in */
    unsigned long resend = atoll(reply + 3);
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
  } else if(!strncmp("!!", reply, 2)) {
    if(device->onerr) {
      device->onerr(device, device->onerr_data, RR_E_HARDWARE_FAULT, reply, nbytes);
    }
    return RR_E_HARDWARE_FAULT;
  } else {
    if(device->onerr) {
      device->onerr(device, device->onerr_data, RR_E_UNKNOWN_REPLY, reply, nbytes);
    }
    return RR_E_UNKNOWN_REPLY;
  }

  return 0;
}
