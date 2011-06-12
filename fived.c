#include "fived.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "comms_private.h"

static void
float_reply (rr_dev dev, char **i, rr_reply type)
{
  if (dev->reply_cb)
    dev->reply_cb (dev, type, strtof (*i + 2, i), NULL, dev->reply_cl);
}

int
fived_handle_reply (rr_dev dev, const char *reply, size_t nbytes)
{
  if (!strncasecmp ("ok", reply, 2)) {
    rr_dev_handle_ok (dev);

    /* Parse values */
    char *i;
    for (i = (char*)reply + 2; i < reply + nbytes; ++i) {
      if (isspace (*i))
	continue;
      switch (toupper (*i)) {
      case 'T':
	float_reply (dev, &i, RR_NOZZLE_TEMP);
	break;
      case 'B':
	float_reply (dev, &i, RR_BED_TEMP);
	break;
      case 'C':
	break;
      case 'X':
	float_reply (dev, &i, RR_X_POS);
	break;
      case 'Y':
	float_reply (dev, &i, RR_Y_POS);
	break;
      case 'Z':
	float_reply (dev, &i, RR_Z_POS);
	break;
      case 'E':
	float_reply (dev, &i, RR_E_POS);
	break;
      default:
	return rr_dev_emit_error (dev, RR_E_UNKNOWN_REPLY, reply, nbytes);
      }
    }
    return 0;

  } else if (!strncasecmp ("rs", reply, 2) ||
	     !strncasecmp ("resend", reply, 6)) {
    /* check where the line number starts */
    size_t n_start = strcspn (reply, "123456789");
    if (n_start) {
      long long lineno = strtoll (reply + n_start, NULL, 10);
      /* check if lineno is in the range of sent lines*/
      if (lineno < dev->lineno)
        return rr_dev_resend (dev, lineno, reply, nbytes);
      else {
	if (dev->lineno <= 1) {
	  /* oh dear - most likely we re-connected to a device that
	     had problems mid-flow, now we need to re-send the
	     line-number reset as if it was this line-no it is asking
	     for a re-send of, or there will be no peace */
	  rr_dev_log (dev, "; resetting confused firmware with synthetic resend of line %d\n",
		      dev->lineno);
	  rr_dev_enqueue_internal (dev, RR_PRIO_HIGH, "M110", 4, lineno);
	}
	return rr_dev_emit_error (dev, RR_E_UNSENT_RESEND, reply, nbytes);
      }
    } else
      return rr_dev_emit_error (dev, RR_E_MALFORMED_RESEND_REQUEST, reply, nbytes);

  } else if (!strncmp("!!", reply, 2)) {
    return rr_dev_emit_error (dev, RR_E_HARDWARE_FAULT, reply, nbytes);

  } else if (!strncasecmp ("start", reply, 5)) {
    return rr_dev_handle_start (dev);

  } else
    return rr_dev_emit_error (dev, RR_E_UNKNOWN_REPLY, reply, nbytes);
}
