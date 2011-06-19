#ifndef _FIVED_H_
#define _FIVED_H_

#include "comms.h"

int fived_handle_reply(rr_dev device, const char *reply, size_t nbytes);

#endif
