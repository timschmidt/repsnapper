#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>

#include "comms_private.h"
#include "serial.h"
#include "fived.h"
#include "tonokip.h"

/* print to the log */
void
rr_dev_log (rr_dev dev, int debug_level, const char *format, ...)
{
  int len;
  va_list args;
  char buffer[4096];

  if (debug_level > dev->debug_output)
    return;

  va_start (args, format);
  len = vsnprintf (buffer, 4095, format, args);
  access (buffer, 0);
  if (dev->opt_log_cb)
    dev->opt_log_cb (dev, RR_LOG_MSG, buffer, len, dev->opt_log_cl);
  va_end (args);
}

static void
blocknode_free (blocknode* node)
{
  free (node->block);
  free (node);
}

static void
rr_dev_append_to_queue (rr_dev dev, rr_prio priority,
			blocknode *node)
{
  if (!dev->sendhead[priority]) {
    dev->sendsize[priority] = 1;
    dev->sendhead[priority] = node;
    dev->sendtail[priority] = node;
  } else {
    dev->sendsize[priority]++;
    dev->sendtail[priority]->next = node;
    dev->sendtail[priority] = node;
  }
}

static void
rr_dev_prepend_to_queue (rr_dev dev, rr_prio priority,
			 blocknode *node)
{
  node->next = dev->sendhead[priority];
  dev->sendhead[priority] = node;
  if (!node->next) {
    dev->sendtail[priority] = node;
    dev->sendsize[priority] = 1;
  } else
    dev->sendsize[priority]++;
}

static blocknode *
rr_dev_pop_from_queue (rr_dev dev, rr_prio priority)
{
  blocknode *node = dev->sendhead[priority];

  if (!node)
    return NULL;

  dev->sendhead[priority] = node->next;
  if (!node->next)
    dev->sendtail[priority] = NULL;
  dev->sendsize[priority]--;

  return node;
}

static void
empty_queue (rr_dev dev, int prio)
{
  while (dev->sendhead[prio]) {
    blocknode *node = rr_dev_pop_from_queue (dev, prio);
    blocknode_free (node);
  }
  assert (dev->sendhead[prio] == NULL);
  assert (dev->sendtail[prio] == NULL);
  dev->sendsize[prio] = 0;
}

static void
empty_buffers (rr_dev dev)
{
  unsigned int i;
  for (i = 0; i < RR_PRIO_ALL_QUEUES; ++i)
    empty_queue (dev, i);
}

rr_dev
rr_dev_create (rr_proto      proto,
	       /* how many commands can we push */
	       size_t        dev_cmdqueue_size,
	       /* callbacks cf. above */
	       rr_reply_fn   reply_cb,   void *reply_cl,
	       rr_more_fn    more_cb,    void *more_cl,
	       rr_error_fn   error_cb,   void *error_cl,
	       /* notify when socket is writeable */
	       rr_wait_wr_fn wait_wr_cb, void *wait_wr_cl,
	       /* optional (or NULL, NULL) */
	       rr_log_fn     opt_log_cb, void *opt_log_cl)
{
  unsigned int i;
  rr_dev dev;

  /* Required callbacks */
  if (!reply_cb || !more_cb || !error_cb || !wait_wr_cb)
    return NULL;

  dev = malloc (sizeof(struct rr_dev_t));
  if (!dev)
    return NULL;

  dev->proto = proto;
  dev->dev_cmdqueue_size = dev_cmdqueue_size;
  dev->reply_cb = reply_cb;
  dev->reply_cl = reply_cl;
  dev->more_cb = more_cb;
  dev->more_cl = more_cl;
  dev->error_cb = error_cb;
  dev->error_cl = error_cl;
  dev->wait_wr_cb = wait_wr_cb;
  dev->wait_wr_cl = wait_wr_cl;
  dev->opt_log_cb = opt_log_cb;
  dev->opt_log_cl = opt_log_cl;

  dev->lineno = 0;
  dev->fd = SERIAL_INVALID_INIT;
  dev->send_next = 0;
  dev->init_send_count = dev->dev_cmdqueue_size;
  dev->debug_output = 0;
  if (getenv("RR_DEBUG"))
    dev->debug_output = atoi (getenv("RR_DEBUG"));

  dev->sentcachesize = dev_cmdqueue_size * 4 + 64;

  for (i = 0; i < RR_PRIO_ALL_QUEUES; ++i) {
    dev->sendsize[i] = 0;
    dev->sendhead[i] = NULL;
    dev->sendtail[i] = NULL;
    dev->paused[i] = 0;
  }
  dev->sendbuf_fill = 0;
  dev->bytes_sent = 0;
  dev->recvbufsize = INITIAL_RECVBUFSIZE;
  dev->recvbuf = calloc (dev->recvbufsize + 1, sizeof (char));
  dev->recvbuf_fill = 0;

  rr_dev_log (dev, RR_DEBUG_ALWAYS, "Connecting with libreprap\n");

  return dev;
}

void
rr_dev_free (rr_dev dev)
{
  rr_dev_close (dev);
  empty_buffers (dev);
  free (dev->recvbuf);
  free (dev);
}

int
rr_dev_open (rr_dev dev, const char *port, long speed)
{
  char *error = NULL;
  dev->fd = serial_open (port, speed, &error);
  if (SERIAL_INVALID_CHECK(dev->fd) < 0)
  {
      rr_dev_log (dev, RR_DEBUG_ALWAYS, "Failed to open device %s", error ? error : "<no error>");
      fprintf (stderr, "%s\n", error ? error : "<null>");
      return -1;
  }
  else
    return 0;
}

int
rr_dev_close (rr_dev dev)
{
  int result = serial_close(dev->fd);
  dev->fd = SERIAL_INVALID_INIT;
  rr_dev_reset (dev);
  return result;
}

static ssize_t
fmtblock_simple (char *buf, const char *block)
{
  char work[SENDBUFSIZE+1];
  int result;
  result = snprintf(work, SENDBUFSIZE+1, "%s" BLOCK_TERMINATOR, block);
  if (result >= SENDBUFSIZE+1)
    return RR_E_BLOCK_TOO_LARGE;
  size_t len = (result > SENDBUFSIZE) ? SENDBUFSIZE : result;
  strncpy(buf, work, len);

  return len;
}

ssize_t fmtblock_fived(char *buf, const char *block, unsigned long line)
{
  char work[SENDBUFSIZE+1];
  int result;
  char checksum = 0;
  result = snprintf(work, GCODE_BLOCKSIZE+1, "N%ld %s", line, block);
  if (result >= GCODE_BLOCKSIZE+1)
    return RR_E_BLOCK_TOO_LARGE;
  ssize_t i;
  for (i = 0; i < result; ++i)
    checksum ^= work[i];

  result = snprintf(work, SENDBUFSIZE+1, "N%ld %s*%d\n", line, block, checksum);
  if (result >= SENDBUFSIZE+1)
    return RR_E_BLOCK_TOO_LARGE;

  size_t len = (result > SENDBUFSIZE) ? SENDBUFSIZE : result;
  strncpy(buf, work, len);

  return len;
}

static ssize_t
fmtblock (rr_dev dev, blocknode *node)
{
  char *terminated = calloc(node->blocksize + 1, sizeof(char));
  strncpy(terminated, node->block, node->blocksize);
  terminated[node->blocksize] = '\0';

  ssize_t result;
  switch (dev->proto) {
  case RR_PROTO_SIMPLE:
    result = fmtblock_simple(dev->sendbuf, terminated);
    break;

  case RR_PROTO_FIVED:
  case RR_PROTO_TONOKIP:
    if (node->line >= 0) {
      /* Block has an explicit line number; may be out of sequence
       * (i.e. a resend) */
      result = fmtblock_fived(dev->sendbuf, terminated, node->line);
    } else {
      result = fmtblock_fived(dev->sendbuf, terminated, dev->lineno);
    }
    break;

  default:
    result = RR_E_UNSUPPORTED_PROTO;
    break;
  }

  free (terminated);
  return result;
}

int
rr_dev_enqueue_internal (rr_dev dev, rr_prio priority,
			 const char *block, size_t nbytes,
			 long long line)
{
  char *comment;
  /*
   * comments are normal in gcode, preceeded by a ';'
   * but we don't want to send those either
   */
  if ((comment = strchr (block, ';')))
    nbytes = comment - block;

  /*
   * Elide both newlines, and whitespace that gets in the way
   * of checksums that are valid.
   */
  while (nbytes > 0 && isspace (block[nbytes - 1]))
    nbytes--;

  if (nbytes == 0)
    return 0;

  blocknode *node = malloc(sizeof(blocknode));
  node->next = NULL;
  node->block = malloc (nbytes + 1);
  node->blocksize = nbytes;
  node->line = line;
  memcpy(node->block, block, nbytes);
  node->block[nbytes] = '\0';

  rr_dev_append_to_queue (dev, priority, node);

  dev->wait_wr_cb (dev, 1, dev->wait_wr_cl);

  return 0;
}

int
rr_dev_resend (rr_dev dev, unsigned long lineno, const char *reply, size_t nbytes)
{
  int resent = 0;
  blocknode *node;

  /* sent cache slot 0 is most recent */
  while (1) {
    if (!(node = rr_dev_pop_from_queue (dev, RR_PRIO_SENTCACHE)))
      break;
    rr_dev_log (dev, RR_DEBUG_HIGH, "pop sent node line %d '%s' (%d bytes)\n",
		(int)node->line, node->block, node->blocksize);
    if (node->line >= lineno) {
      rr_dev_prepend_to_queue (dev, RR_PRIO_RESEND, node);
      resent++;
    } else { /* put it back and look elsewhere */
      rr_dev_prepend_to_queue (dev, RR_PRIO_SENTCACHE, node);
      break;
    }
  }

  if (resent == 0) {
    /* Perhaps line is in the resend queue, and we got an:
     *     rs: 3
     *     rs: 6
     * type sequence so try peel forward the resend queue.
     */

    while (1) {
      if (!(node = rr_dev_pop_from_queue (dev, RR_PRIO_RESEND)))
	break;
      rr_dev_log (dev, RR_DEBUG_HIGH,
		  "pop resend node line %d '%s' (%d bytes)\n",
	  (int)node->line, node->block, node->blocksize);
      if (node->line < lineno) {
	rr_dev_prepend_to_queue (dev, RR_PRIO_SENTCACHE, node);
	resent++;
      } else { /* put it back and give up */
	rr_dev_prepend_to_queue (dev, RR_PRIO_RESEND, node);
	break;
      }
    }

    if (resent == 0) {
      rr_dev_log (dev, RR_DEBUG_ALWAYS,
		  "re-send request for unknown (too old) line %ld from cache size %d\n",
		  lineno, dev->sendsize[RR_PRIO_SENTCACHE]);
      rr_dev_emit_error (dev, RR_E_UNCACHED_RESEND, reply, nbytes);
    }
  }

  dev->send_next = 1;
  dev->wait_wr_cb (dev, 1, dev->wait_wr_cl);

  return 0;
}

void
rr_dev_reset_lineno (rr_dev dev)
{
  dev->lineno = 0;
  /* we invalidate all these line numbers */
  empty_queue (dev, RR_PRIO_RESEND);
  empty_queue (dev, RR_PRIO_SENTCACHE);
  rr_dev_enqueue_internal (dev, RR_PRIO_HIGH, "M110", 4, -1);
  dev->init_send_count = dev->dev_cmdqueue_size - 1;
}

void
rr_dev_reset (rr_dev dev)
{
  empty_buffers (dev);
  rr_dev_log (dev, RR_DEBUG_MEDIUM, "reset dev size to zero\n");
  rr_dev_reset_lineno (dev);
  dev->recvbuf_fill = 0;
}

void
rr_dev_reset_device (rr_dev dev)
{
  serial_setDTR(dev->fd, 1);
  serial_setDTR(dev->fd, 0);
}

static void
debug_log_block (rr_dev dev, const char *block, int nbytes)
{
  char buffer[4096], *p;
  int num = nbytes >= 4096 ? 4095 : nbytes;
  strncpy (buffer, block, num);
  buffer[num] = '\0';
  if ((p = strchr (buffer, '\n')))
    *p = '\0';
  if ((p = strchr (buffer, '\r')))
    *p = '\0';
  rr_dev_log (dev, RR_DEBUG_MEDIUM, "API enqueue of '%s' queue size %d\n",
	      buffer, rr_dev_buffered_lines (dev));
}

int
rr_dev_enqueue_cmd (rr_dev dev, rr_prio priority,
		    const char *block, int opt_nbytes)
{
  if (opt_nbytes < 0)
    opt_nbytes = strlen (block);
  if (dev->debug_output >= RR_DEBUG_MEDIUM)
    debug_log_block (dev, block, opt_nbytes);

  return rr_dev_enqueue_internal (dev, priority, block, opt_nbytes, -1);
}

void
rr_dev_handle_ok (rr_dev dev)
{
  int buffered = rr_dev_buffered_lines (dev);

  /* Send as many commands as we get ok's */
  if (dev->init_send_count > 0)
    dev->init_send_count--;
  dev->send_next = 1;

  if (buffered < dev->dev_cmdqueue_size) {
    rr_dev_log (dev, RR_DEBUG_MEDIUM,
		"request more %d < %d\n", buffered, dev->dev_cmdqueue_size);
    dev->more_cb (dev, dev->more_cl);
  }
  dev->wait_wr_cb (dev, 1, dev->wait_wr_cl);

  if (dev->debug_output > RR_DEBUG_ALWAYS)
    { /* Check the sendsize accounts add up */
      int i = 0;
      for (i = 0; i < RR_PRIO_COUNT; ++i) {
	blocknode *p;
	int count = 0;
	for (p = dev->sendhead[i]; p; p = p->next) {
	  if (!p->next && dev->sendtail[i] != p)
	    rr_dev_log (dev, RR_DEBUG_MEDIUM,
			"Error: queue (%d) broken tail pointer %p vs %p\n",
			i, p, dev->sendtail[i]);
	  count++;
	}
	if (count != dev->sendsize[i])
	  rr_dev_log (dev, RR_DEBUG_MEDIUM,
		      "Error: queue (%d) size mismatch: %d vs %d\n",
		      i, count, dev->sendsize[i]);
      }
    }
}

int
rr_dev_handle_start (rr_dev dev)
{
  /*
   * This is non-intuitive. If we reset the controller, when we next send
   * a command sequence, on the first command we will get a 'start',
   * meaning we should reset the line number. Problem is we then send
   * the rest of the command sequence and get another 'start' in mid
   * flow for some controllers, which gets us out of sync. Ergo we need
   * to reset the line number with a command each time we hit one of
   * these.
   */
  rr_dev_reset_lineno (dev);
  return 0;
}

static int
handle_reply (rr_dev dev, const char *reply, size_t nbytes, size_t term_bytes)
{
  if (dev->opt_log_cb)
    dev->opt_log_cb (dev, RR_LOG_RECV, reply, nbytes + term_bytes, dev->opt_log_cl);

  switch(dev->proto) {
  case RR_PROTO_FIVED:
    return fived_handle_reply (dev, reply, nbytes);

  case RR_PROTO_TONOKIP:
    return tonokip_handle_reply (dev, reply, nbytes);

  case RR_PROTO_SIMPLE:
    if (!strncasecmp ("ok", reply, 2) && dev->reply_cb) {
      rr_dev_handle_ok (dev);
      dev->reply_cb (dev, RR_OK, 0.0, NULL, dev->reply_cl);
    } else if (dev->error_cb)
      dev->error_cb (dev, RR_E_UNKNOWN_REPLY, reply, nbytes, dev->error_cl);
    return 0;

  default:
    return RR_E_UNSUPPORTED_PROTO;
  }

  return 0;
}

int
rr_dev_handle_readable (rr_dev dev)
{
  int i;

  /* Grow receive buffer if it's full */
  if (dev->recvbuf_fill >= dev->recvbufsize) {
    dev->recvbufsize *= 2;
    dev->recvbuf = realloc (dev->recvbuf, dev->recvbufsize + 1);
  }

  ssize_t result;

  do {
    result = read (dev->fd, dev->recvbuf + dev->recvbuf_fill, dev->recvbufsize - dev->recvbuf_fill);
  } while (result < 0 && errno == EINTR);

  if (result < 0)
    return result;

  dev->recvbuf_fill += result;

  /* string terminate */
  dev->recvbuf[dev->recvbuf_fill] = '\0';

  /* validate the stream is ascii and of the correct length */
  for (i = 0; i < dev->recvbuf_fill; i++) {
    if (dev->recvbuf[i] == '\0' || !isascii (dev->recvbuf[i])) {
      rr_dev_log (dev, RR_DEBUG_ALWAYS,
		  "invalid char in recvbuf at char %d (0x%2x) full "
		  "msg: '%s', truncating here", i, dev->recvbuf[i],
		  dev->recvbuf);
      dev->recvbuf_fill = i;
      dev->recvbuf[dev->recvbuf_fill] = '\0';
      break;
    }
  }

  /* spot control characters */
  if (dev->recvbuf[0] == 1) {
    rr_dev_log (dev, RR_DEBUG_ALWAYS,
		"unusual - control character 0x%2x 0x%2x\n",
		dev->recvbuf[0], dev->recvbuf[1]);
    memmove (dev->recvbuf, dev->recvbuf + 2, dev->recvbuf_fill - 2);
    dev->recvbuf_fill -= 2;
  }

  /* Scan the buffer and shift it down if we detect a full command */
  size_t reply_span, term_span;
  while (1) {
    /* How many non terminator chars and how many terminator chars after them */
    reply_span = strcspn (dev->recvbuf, REPLY_TERMINATOR);
    term_span = strspn (dev->recvbuf + reply_span, REPLY_TERMINATOR);

    if (term_span > 0) {
      if (reply_span > 0)
	handle_reply (dev, dev->recvbuf, reply_span, term_span);
      /* else - perhaps a prepended \n having sent in reaction to \r previously */

      size_t len = reply_span + term_span;
      //assert (dev->recvbuf_fill >= len);
      if (dev->recvbuf_fill < len) {
	return -1;
      }
      dev->recvbuf_fill -= len;
      memmove (dev->recvbuf, dev->recvbuf + len, dev->recvbuf_fill + 1);
      continue;
    }
    break;
  }

  return 0;
}

/* free extra elements at the end of the sentcache */
static void
shrink_sentcache (rr_dev dev)
{
  int i;
  blocknode *next, *l = dev->sendhead[RR_PRIO_SENTCACHE];
  for (i = 0; i < dev->sentcachesize; i++)
    l = l ? l->next : NULL;
  if (!l)
    return;
  dev->sendtail[RR_PRIO_SENTCACHE] = l;
  next = l->next;
  l->next = NULL;

  for (l = next; l; l = next) {
    next = l->next;
    blocknode_free (l);
    dev->sendsize[RR_PRIO_SENTCACHE]--;
  }
}

int
rr_dev_handle_writable (rr_dev dev)
{
  ssize_t result;

  if (dev->sendbuf_fill == 0) {
    if (dev->init_send_count <= 0 && !dev->send_next) {
      rr_dev_log (dev, RR_DEBUG_MEDIUM,
		  "writeable - init count is %d, no send_next queue %d resend %d\n",
		  dev->init_send_count, dev->dev_cmdqueue_size,
		  dev->sendsize[RR_PRIO_RESEND]);
      /* wait until there is space in the device buffer and/or an ok */
      dev->wait_wr_cb (dev, 0, dev->wait_wr_cl);
      return 0;
    }

    /* Last block is gone; prepare to send a new block */
    int prio;
    blocknode *node = NULL;
    for (prio = RR_PRIO_COUNT - 1; prio >= 0; --prio) {

      if (dev->paused[prio])
	continue;

      node = dev->sendhead[prio];
      if (node) {
        /* We have a block to send! Get it ready. */
        dev->bytes_sent = 0;
        result = fmtblock (dev, node);
        if (result < 0) {
          /* FIXME: This will confuse code expecting errno to be set */
          return result;
        }
	if (result == 0)
	  rr_dev_log (dev, RR_DEBUG_ALWAYS,
		      "unusual error - nothing in block to write\n");

	dev->send_next = 0;
        dev->sendbuf_fill = result;
        dev->sending_prio = prio;
        break;
      }
    }
    if (!node) {
      /* No data to write */
      dev->wait_wr_cb (dev, 0, dev->wait_wr_cl);
      return 0;
    }
  }

  /* Perform write */
  do {
    result = write (dev->fd, dev->sendbuf + dev->bytes_sent, dev->sendbuf_fill - dev->bytes_sent);
  } while (result < 0 && errno == EINTR);

  if (result < 0)
    return result;

  if (dev->opt_log_cb)
    dev->opt_log_cb (dev, RR_LOG_SEND, dev->sendbuf + dev->bytes_sent,
		     dev->sendbuf_fill - dev->bytes_sent,
		     dev->opt_log_cl);

  dev->bytes_sent += result;

  if (dev->bytes_sent == dev->sendbuf_fill) {
    /* We've sent the complete block. */
    blocknode *node = rr_dev_pop_from_queue (dev, dev->sending_prio);

    if (node->line < 0) {
      node->line = dev->lineno;
      ++(dev->lineno);
    }

    /* Update sent cache */
    assert (node->block != NULL);
    rr_dev_prepend_to_queue (dev, RR_PRIO_SENTCACHE, node);
    if (dev->sendsize[RR_PRIO_SENTCACHE] > (dev->sentcachesize * 3 / 2))
      shrink_sentcache (dev);

    /* Indicate that we're ready for the next. */
    dev->sendbuf_fill = 0;
  }

  return result;
}

int rr_flush (rr_dev dev)
{
#ifndef WIN32
  /* Disable non-blocking mode */
  int flags;
  if ((flags = fcntl (dev->fd, F_GETFL, 0)) < 0)
    return flags;

  if (fcntl (dev->fd, F_SETFL, flags & ~O_NONBLOCK) == -1)
    return -1;
#endif

  int result = 0;
  while (rr_dev_buffered (dev) && result >= 0)
    result = rr_dev_handle_writable (dev);

#ifndef WIN32
  /* Restore original mode */
  if (result >= 0)
    result = fcntl (dev->fd, F_SETFL, flags);
  else
    fcntl (dev->fd, F_SETFL, flags);
#endif

  return result;
}

#ifndef WIN32
int
rr_dev_fd (rr_dev dev)
{
  return dev->fd;
}
#endif

int
rr_dev_is_connected (rr_dev dev)
{
  return SERIAL_INVALID_CHECK(dev->fd) < 0 ? 0 : 1;
}

unsigned long
rr_dev_lineno (rr_dev dev)
{
  return dev->lineno;
}

int
rr_dev_buffered_lines (rr_dev dev)
{
  int i, size = 0;
  for (i = 0; i < RR_PRIO_COUNT; i++)
    size += dev->sendsize[i];
  return size;
}

int
rr_dev_buffered (rr_dev dev)
{
  if (rr_dev_buffered_lines (dev) > 0)
    return 1;
  else
    return dev->sendbuf_fill;
}

int
rr_dev_write_more (rr_dev dev)
{
  /* arbitrarily keep a cmdqueue of the same size internally */
  return rr_dev_buffered_lines (dev) < dev->dev_cmdqueue_size;
}

void
rr_dev_enable_debugging (rr_dev dev, int debug_output)
{
  dev->debug_output = debug_output;
}

void
rr_dev_set_paused (rr_dev dev, int priority, int paused)
{
  dev->paused[priority] = paused;

  /* re-start client's writing */
  if (!paused) {
    dev->more_cb (dev, dev->more_cl);
    dev->wait_wr_cb (dev, 1, dev->wait_wr_cl);
  }
}

/* Helper for emitting and returning errors */
rr_error
rr_dev_emit_error (rr_dev dev, rr_error err, const char *block, int nbytes)
{
  if (dev->error_cb)
    dev->error_cb (dev, err, block, nbytes, dev->error_cl);
  return err;
}

/* What an horrible method - the protocol is so bad flow control
   wise we can jam up, and need to be coaxed back into life */
void
rr_dev_kick (rr_dev dev)
{
  dev->send_next = 1;
  dev->init_send_count = dev->dev_cmdqueue_size - 1;
  dev->wait_wr_cb (dev, 1, dev->wait_wr_cl);
}
