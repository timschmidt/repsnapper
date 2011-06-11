#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdarg.h>

#include "comms_private.h"
#include "serial.h"
#include "fived.h"
#include "tonokip.h"

#define DEBUG

#ifdef DEBUG
#  define debug_log(a) rr_dev_log a
#else
#  define debug_log(a)
#endif

/* print to the log */
void
rr_dev_log (rr_dev dev, const char *format, ...)
{
  int len;
  va_list args;
  char buffer[4096];

  va_start (args, format);
  len = vsnprintf (buffer, 4095, format, args);
  if (dev->opt_log_cb)
    dev->opt_log_cb (dev, buffer, len, dev->opt_log_cl);
  va_end (args);
}

static void
blocknode_free (blocknode* node)
{
  free (node->block);
  free (node);
}

static void
empty_buffers (rr_dev dev)
{
  unsigned i;
  for (i = 0; i < RR_PRIO_COUNT; ++i) {
    blocknode *j = dev->sendhead[i];
    while(j != NULL) {
      blocknode *next = j->next;
      free(j);
      j = next;
    }
    dev->sendhead[i] = NULL;
    dev->sendtail[i] = NULL;
    dev->sendsize[i] = 0;
  }
  for (i = 0; i < dev->sentcachesize; ++i) {
    if (dev->sentcache[i]) {
      blocknode_free (dev->sentcache[i]);
      dev->sentcache[i] = NULL;
    }
  }
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
  dev->fd = -1;
  dev->ok_count = 0;

  dev->sentcache = calloc (dev_cmdqueue_size, sizeof (blocknode*));
  dev->sentcachesize = dev_cmdqueue_size;
  for (i = 0; i < dev->sentcachesize; ++i)
    dev->sentcache[i] = NULL;

  for (i = 0; i < RR_PRIO_COUNT; ++i) {
    dev->sendsize[i] = 0;
    dev->sendhead[i] = NULL;
    dev->sendtail[i] = NULL;
    dev->paused[i] = 0;
  }
  dev->sendbuf_fill = 0;
  dev->bytes_sent = 0;
  dev->recvbuf = calloc (RECVBUFSIZE, sizeof(char));
  dev->recvbufsize = RECVBUFSIZE;
  dev->recvbuf_fill = 0;

  rr_dev_log (dev, "; Connecting with libreprap\n");

  return dev;
}

void
rr_dev_free (rr_dev dev)
{
  rr_dev_close (dev);
  empty_buffers (dev);
  free (dev->sentcache);
  free (dev->recvbuf);
  free (dev);
}

int
rr_dev_open (rr_dev dev, const char *port, long speed)
{
  dev->fd = serial_open (port, speed);
  if (dev->fd < 0)
    return dev->fd;
  else
    return 0;
}

int
rr_dev_close (rr_dev dev)
{
  int result;

  if (dev->fd >= 0) {
    do {
      result = close (dev->fd);
    } while (result < 0 && errno == EINTR);
  }
  dev->fd = -1;
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
  switch(dev->proto) {
  case RR_PROTO_SIMPLE:
    result = fmtblock_simple(dev->sendbuf, terminated);
    break;

  case RR_PROTO_FIVED:
  case RR_PROTO_TONOKIP:
    if(node->line >= 0) {
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

  free(terminated);
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

  blocknode *node = malloc(sizeof(blocknode));
  node->next = NULL;
  node->block = malloc(nbytes);
  node->blocksize = nbytes;
  node->line = line;
  memcpy(node->block, block, nbytes);

  if (!dev->sendhead[priority]) {
    dev->sendsize[priority] = 1;
    dev->sendhead[priority] = node;
    dev->sendtail[priority] = node;
  } else {
    dev->sendsize[priority]++;
    dev->sendtail[priority]->next = node;
    dev->sendtail[priority] = node;
  }

  dev->wait_wr_cb (dev, 1, dev->wait_wr_cl);

  return 0;
}

int
rr_dev_resend (rr_dev dev, unsigned long lineno, const char *reply, size_t nbytes)
{
  unsigned long delta = (dev->lineno - 1) - lineno;

  if (delta < dev->sentcachesize) {
    blocknode *node = dev->sentcache[delta];
    assert (node);
    rr_dev_enqueue_internal (dev, RR_PRIO_RESEND, node->block, node->blocksize, lineno);
    return 0;

  } else /* Line needed for resend was not cached */
    return rr_dev_emit_error (dev, RR_E_UNCACHED_RESEND, reply, nbytes);
}

void
rr_dev_reset_lineno (rr_dev dev)
{
  dev->lineno = 0;
  rr_dev_enqueue_internal (dev, RR_PRIO_HIGH, "M110", 4, -1);
  dev->ok_count = 4;
}

void
rr_dev_reset (rr_dev dev)
{
  empty_buffers (dev);
  debug_log ((dev, "; reset dev size to zero\n"));
  rr_dev_reset_lineno (dev);
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
  rr_dev_log (dev, "; queue cmd '%s' queue %d\n",
	      buffer, rr_dev_buffered_lines (dev));
}

int
rr_dev_enqueue_cmd (rr_dev dev, rr_prio priority,
		    const char *block, int opt_nbytes)
{
  if (opt_nbytes < 0)
    opt_nbytes = strlen (block);
#ifdef DEBUG
  debug_log_block (dev, block, opt_nbytes);
#endif
  return rr_dev_enqueue_internal (dev, priority, block, opt_nbytes, -1);
}

void
rr_dev_handle_ok (rr_dev dev)
{
  int buffered = rr_dev_buffered_lines (dev);

  /* Send as many commands as we get ok's */
  dev->ok_count++;

  if (buffered < dev->dev_cmdqueue_size) {
    debug_log ((dev, "; request more %d < %d\n", buffered, dev->dev_cmdqueue_size));
    dev->more_cb (dev, dev->more_cl);
  }
  dev->wait_wr_cb (dev, 1, dev->wait_wr_cl);

#ifdef DEBUG
  { /* Check the sendsize accounts add up */
    int i = 0;
    for (i = 0; i < RR_PRIO_COUNT; ++i) {
      blocknode *p;
      int count = 0;
      for (p = dev->sendhead[i]; p; p = p->next)
	count++;
      if (count != dev->sendsize[i])
	debug_log ((dev, "; queue size mismatch: %d vs %d\n",
		    count, dev->sendsize[i]));
    }
  }
#endif
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
    dev->opt_log_cb (dev, reply, nbytes + term_bytes, dev->opt_log_cl);

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
  /* Grow receive buffer if it's full */
  if(dev->recvbuf_fill == dev->recvbufsize)
    dev->recvbuf = realloc(dev->recvbuf, 2*dev->recvbufsize);

  ssize_t result;

  do {
    result = read (dev->fd, dev->recvbuf + dev->recvbuf_fill, dev->recvbufsize - dev->recvbuf_fill);
  } while (result < 0 && errno == EINTR);

  if (result < 0)
    return result;

  dev->recvbuf_fill += result;

  /* Scan for complete reply */
  size_t scan = 0;
  size_t end = dev->recvbuf_fill;
  size_t reply_span = 0;
  size_t term_span = 0;
  size_t start = 0;

  do {
    /* How many non terminator chars and how many terminator chars after them*/
    reply_span = strcspn (dev->recvbuf + scan, REPLY_TERMINATOR);
    term_span = strspn (dev->recvbuf + scan + reply_span, REPLY_TERMINATOR);
    start = scan;

    if (0 < term_span && 0 < reply_span && (start + reply_span + 1) < end) {
      /* We have a terminator after non terminator chars */
      handle_reply (dev, dev->recvbuf + start, reply_span, term_span);
    }
    scan += reply_span + term_span;

  } while (scan < end);

  size_t rest_size = end - start;

  /* Move the rest of the buffer to the beginning */
  if (rest_size > 0) {
    dev->recvbuf_fill = rest_size;
    memmove (dev->recvbuf, dev->recvbuf + start, dev->recvbuf_fill);
  } else {
    dev->recvbuf_fill = 0;
  }

  return 0;
}

int
rr_dev_handle_writable (rr_dev dev)
{
  ssize_t result;

  if (dev->sendbuf_fill == 0) {
    if (dev->ok_count <= 0) {
      debug_log ((dev, "; writeable - wait ok count is %d, queue %d\n",
		  dev->ok_count, dev->dev_cmdqueue_size));
      /* wait until there is space in the device buffer */
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
	dev->ok_count = dev->ok_count > 0 ? dev->ok_count - 1 : 0;
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
    result = write(dev->fd, dev->sendbuf + dev->bytes_sent, dev->sendbuf_fill - dev->bytes_sent);
  } while(result < 0 && errno == EINTR);

  if (result < 0)
    return result;

  if (dev->opt_log_cb)
    dev->opt_log_cb (dev, dev->sendbuf + dev->bytes_sent,
		     dev->sendbuf_fill - dev->bytes_sent,
		     dev->opt_log_cl);

  dev->bytes_sent += result;

  if(dev->bytes_sent == dev->sendbuf_fill) {
    /* We've sent the complete block. */
    blocknode *node = dev->sendhead[dev->sending_prio];
    dev->sendhead[dev->sending_prio] = node->next;
    if (!node->next)
      dev->sendtail[dev->sending_prio] = NULL;
    dev->sendsize[dev->sending_prio]--;
    node->line = dev->lineno;
    ++(dev->lineno);

    /* Update sent cache */
    if (dev->sentcache[dev->sentcachesize - 1])
      blocknode_free (dev->sentcache[dev->sentcachesize - 1]);

    ssize_t i;
    for (i = dev->sentcachesize - 1; i > 0 ; --i)
      dev->sentcache[i] = dev->sentcache[i-1];
    dev->sentcache[0] = node;

    /* Indicate that we're ready for the next. */
    dev->sendbuf_fill = 0;
  }

  return result;
}

int rr_flush (rr_dev dev)
{
  /* Disable non-blocking mode */
  int flags;
  if ((flags = fcntl (dev->fd, F_GETFL, 0)) < 0)
    return flags;

  if (fcntl (dev->fd, F_SETFL, flags & ~O_NONBLOCK) == -1)
    return -1;

  int result = 0;
  while (rr_dev_buffered (dev) && result >= 0)
    result = rr_dev_handle_writable (dev);

  if (result >= 0)
    result = fcntl (dev->fd, F_SETFL, flags);
  else
    fcntl (dev->fd, F_SETFL, flags);

  /* Restore original mode */
  return result;
}

int
rr_dev_fd (rr_dev dev)
{
  return dev->fd;
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
/*  debug_log ((dev, "; buffered lines = %d\n", size)); */
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
