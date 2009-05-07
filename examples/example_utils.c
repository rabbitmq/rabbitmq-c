#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <sys/time.h>
#include <unistd.h>

void die_on_error(int x, char const *context) {
  if (x < 0) {
    fprintf(stderr, "%s: %s\n", context, strerror(-x));
    exit(1);
  }
}

void die_on_amqp_error(amqp_rpc_reply_t x, char const *context) {
  switch (x.reply_type) {
    case AMQP_RESPONSE_NORMAL:
      return;

    case AMQP_RESPONSE_NONE:
      fprintf(stderr, "%s: missing RPC reply type!", context);
      break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      fprintf(stderr, "%s: %s\n", context,
	      x.library_errno ? strerror(x.library_errno) : "(end-of-stream)");
      break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
      switch (x.reply.id) {
	case AMQP_CONNECTION_CLOSE_METHOD: {
	  amqp_connection_close_t *m = (amqp_connection_close_t *) x.reply.decoded;
	  fprintf(stderr, "%s: server connection error %d, message: %.*s",
		  context,
		  m->reply_code,
		  (int) m->reply_text.len, (char *) m->reply_text.bytes);
	  break;
	}
	case AMQP_CHANNEL_CLOSE_METHOD: {
	  amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
	  fprintf(stderr, "%s: server channel error %d, message: %.*s",
		  context,
		  m->reply_code,
		  (int) m->reply_text.len, (char *) m->reply_text.bytes);
	  break;
	}
	default:
	  fprintf(stderr, "%s: unknown server error, method id 0x%08X", context, x.reply.id);
	  break;
      }
      break;
  }

  exit(1);
}

long long now_microseconds(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (long long) tv.tv_sec * 1000000 + (long long) tv.tv_usec;
}
