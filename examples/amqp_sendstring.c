#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <unistd.h>

static void die_on_error(int x, char const *context) {
  if (x < 0) {
    fprintf(stderr, "%s: %s\n", context, strerror(-x));
    exit(1);
  }
}

static void die_on_amqp_error(amqp_rpc_reply_t x, char const *context) {
  switch (x.reply_type) {
    case AMQP_RESPONSE_NORMAL:
      return;

    case AMQP_RESPONSE_NONE:
      fprintf(stderr, "%s: missing RPC reply type!", context);
      break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      fprintf(stderr, "%s: %s\n", context, strerror(x.library_errno));
      break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
      switch (x.reply.id) {
	case AMQP_CONNECTION_CLOSE_METHOD: {
	  amqp_connection_close_t *m = (amqp_connection_close_t *) x.reply.decoded;
	  fprintf(stderr, "%s: server connection error %d, message: %*s",
		  context,
		  m->reply_code,
		  (int) m->reply_text.len,
		  (char *) m->reply_text.bytes);
	  break;
	}
	case AMQP_CHANNEL_CLOSE_METHOD: {
	  amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
	  fprintf(stderr, "%s: server channel error %d, message: %*s",
		  context,
		  m->reply_code,
		  (int) m->reply_text.len,
		  (char *) m->reply_text.bytes);
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

int main(int argc, char const * const *argv) {
  char const *hostname;
  int port;
  char const *exchange;
  char const *routingkey;
  char const *messagebody;

  int sockfd;
  amqp_connection_state_t conn;

  if (argc < 6) {
    fprintf(stderr, "Usage: amqp_sendstring host port exchange routingkey messagebody\n");
    return 1;
  }

  hostname = argv[1];
  port = atoi(argv[2]);
  exchange = argv[3];
  routingkey = argv[4];
  messagebody = argv[5];

  conn = amqp_new_connection();

  die_on_error(sockfd = amqp_open_socket(hostname, port), "Opening socket");
  amqp_set_sockfd(conn, sockfd);
  die_on_amqp_error(amqp_login(conn, "/", 131072, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
		    "Logging in");

  die_on_error(amqp_basic_publish(conn,
				  amqp_cstring_bytes(exchange),
				  amqp_cstring_bytes(routingkey),
				  0,
				  0,
				  NULL,
				  amqp_cstring_bytes(messagebody)),
	       "Publishing");

  printf("Waiting for frames...\n");
  while (1) {
    amqp_frame_t frame;
    int result = amqp_simple_wait_frame(conn, &frame);
    printf("Result %d\n", result);
    printf("Frame type %d, channel %d\n", frame.frame_type, frame.channel);
    if (result == 0) break;
    amqp_maybe_release_buffers(conn);
  }

  amqp_destroy_connection(conn);

  die_on_error(close(sockfd), "Closing socket");

  return 0;
}
