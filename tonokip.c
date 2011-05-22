#include "tonokip.h"

#include <stdlib.h>
#include <string.h>

#include "comms_private.h"
#include "common.h"

int
tonokip_handle_reply (rr_dev dev, const char *reply, size_t nbytes) {

  if (!strncasecmp ("ok", reply, 2)) {
    if (dev->reply_cb)
      dev->reply_cb (dev, RR_OK, 0.0, NULL, dev->reply_cl);
    return 0;

  } else if (!strncasecmp ("Resend:", reply, 7)) {
    /* Line number begins 7 bytes in */
    return rr_dev_resend (dev, atoll (reply + 7), reply, nbytes);

  } else if (!strncmp("T:", reply, 2)) {
    if (dev->reply_cb) {
      char *point = (char *)reply + 2;
      dev->reply_cb (dev, RR_NOZZLE_TEMP, strtof (reply + 2, &point), NULL, dev->reply_cl);

      if (!strncmp("B:", point + 1, 2))
	dev->reply_cb (dev, RR_BED_TEMP, strtof (point + 3, NULL), NULL, dev->reply_cl);
    }
    return 0;

  } else
    return rr_dev_emit_error (dev, RR_E_UNKNOWN_REPLY, reply, nbytes);
}
