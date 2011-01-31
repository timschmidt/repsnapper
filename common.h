#ifndef _COMMON_H_
#define _COMMON_H_

#include "comms.h"

int resend(rr_dev device, unsigned long lineno, const char *reply, size_t nbytes);

#endif
