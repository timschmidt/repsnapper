#include "fived.h"

#include <stdlib.h>
#include <string.h>

#include "comms_private.h"
#include "common.h"

int fived_handle_reply(rr_dev device, const char *reply, size_t nbytes) {
  if(!strncasecmp("ok", reply, 2)) {
    if(device->onreply) {
      device->onreply(device, device->onreply_data, RR_OK, 0);
      /* Parse values */
      char *i;
      for(i = (char*)reply; i < reply + nbytes; ++i) {
        switch(toupper(*i)) {
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
  } else if(!strncasecmp("rs", reply, 2) || !strncasecmp("resend", reply, 6)) {
    /* check where the line number starts */
    size_t n_start = strcspn(reply, "123456789");
    if(n_start) {
      long long lineno = strtoll(reply + n_start, NULL, 10);
      /* check if lineno is in the range of sent lines*/
      if(lineno < device->lineno && strncmp("-", reply + n_start - 1, 1)) {
        return resend(device, lineno, reply, nbytes);
      } else {
        if(device->onerr) {
          device->onerr(device, device->onerr_data, RR_E_UNSENT_RESEND, reply, nbytes);
        }
        return RR_E_UNSENT_RESEND;
      }
    } else {
      if(device->onerr) {
        device->onerr(device, device->onerr_data, RR_E_MALFORMED_RESEND_REQUEST, reply, nbytes);
      }
    return RR_E_MALFORMED_RESEND_REQUEST;
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
