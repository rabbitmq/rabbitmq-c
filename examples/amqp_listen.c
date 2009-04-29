#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <stdint.h>
#include <amqp.h>
#include <amqp_framing.h>

#include <unistd.h>

#include "example_utils.h"

/* Private: compiled out in NDEBUG mode */
extern void amqp_dump(void const *buffer, size_t len);

int main(int argc, char const * const *argv) {
  char const *hostname;
  int port;
  char const *exchange;
  char const *bindingkey;

  int sockfd;
  amqp_connection_state_t conn;

  amqp_rpc_reply_t result;
  amqp_bytes_t queuename;

  if (argc < 5) {
    fprintf(stderr, "Usage: amqp_listen host port exchange bindingkey\n");
    return 1;
  }

  hostname = argv[1];
  port = atoi(argv[2]);
  exchange = argv[3];
  bindingkey = argv[4];

  conn = amqp_new_connection();

  die_on_error(sockfd = amqp_open_socket(hostname, port), "Opening socket");
  amqp_set_sockfd(conn, sockfd);
  die_on_amqp_error(amqp_login(conn, "/", 131072, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
		    "Logging in");

  {
    amqp_queue_declare_t s =
      (amqp_queue_declare_t) {
        .ticket = 0,
	.queue = {.len = 0, .bytes = NULL},
	.passive = 0,
	.durable = 0,
	.exclusive = 0,
	.auto_delete = 1,
	.nowait = 0,
	.arguments = {.num_entries = 0, .entries = NULL}
      };
    die_on_amqp_error(result = amqp_simple_rpc(conn, 1, AMQP_QUEUE_DECLARE_METHOD,
					       AMQP_QUEUE_DECLARE_OK_METHOD, &s),
		      "Declaring queue");
    amqp_queue_declare_ok_t *r = (amqp_queue_declare_ok_t *) result.reply.decoded;
    queuename = amqp_bytes_malloc_dup(r->queue);
    if (queuename.bytes == NULL) {
      die_on_error(-ENOMEM, "Copying queue name");
    }
  }

  {
    amqp_queue_bind_t s =
      (amqp_queue_bind_t) {
        .ticket = 0,
	.queue = queuename,
	.exchange = amqp_cstring_bytes(exchange),
	.routing_key = amqp_cstring_bytes(bindingkey),
	.nowait = 0,
	.arguments = {.num_entries = 0, .entries = NULL}
      };
    die_on_amqp_error(result = amqp_simple_rpc(conn, 1, AMQP_QUEUE_BIND_METHOD,
					       AMQP_QUEUE_BIND_OK_METHOD, &s),
		      "Binding queue");
  }

  {
    amqp_basic_consume_t s =
      (amqp_basic_consume_t) {
        .ticket = 0,
	.queue = queuename,
	.consumer_tag = {.len = 0, .bytes = NULL},
	.no_local = 0,
	.no_ack = 1,
	.exclusive = 0,
	.nowait = 0
      };
    die_on_amqp_error(result = amqp_simple_rpc(conn, 1, AMQP_BASIC_CONSUME_METHOD,
					       AMQP_BASIC_CONSUME_OK_METHOD, &s),
		      "Consuming");
  }

  {
    amqp_frame_t frame;
    int result;

    while (1) {
      amqp_maybe_release_buffers(conn);
      result = amqp_simple_wait_frame(conn, &frame);
      printf("Result %d\n", result);
      if (result <= 0) goto shutdown;

    analyse_frame:
      printf("Frame type %d, channel %d\n", frame.frame_type, frame.channel);
      if (frame.frame_type == AMQP_FRAME_METHOD) {
	printf("Method %s\n", amqp_method_name(frame.payload.method.id));
	if (frame.payload.method.id == AMQP_BASIC_DELIVER_METHOD) {
	  amqp_basic_deliver_t *d = (amqp_basic_deliver_t *) frame.payload.method.decoded;
	  amqp_basic_properties_t *p;
	  printf("Delivery %llu, exchange %.*s routingkey %.*s\n",
		 d->delivery_tag,
		 (int) d->exchange.len, (char *) d->exchange.bytes,
		 (int) d->routing_key.len, (char *) d->routing_key.bytes);

	  result = amqp_simple_wait_frame(conn, &frame);
	  if (result <= 0) goto shutdown;
	  if (frame.frame_type != AMQP_FRAME_HEADER) {
	    fprintf(stderr, "Expected header!");
	    abort();
	  }
	  p = (amqp_basic_properties_t *) frame.payload.properties.decoded;
	  if (p->_flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
	    printf("Content-type: %.*s\n",
		   (int) p->content_type.len, (char *) p->content_type.bytes);
	  }
	  printf("----\n");

	  while (1) {
	    result = amqp_simple_wait_frame(conn, &frame);
	    if (result <= 0) goto shutdown;
	    if (frame.frame_type != AMQP_FRAME_BODY) {
	      printf("====\n");
	      goto analyse_frame;
	    }
	    amqp_dump(frame.payload.body_fragment.bytes,
		      frame.payload.body_fragment.len);
	  }
	}
      }
    }
  }

 shutdown:
  die_on_amqp_error(amqp_channel_close(conn, AMQP_REPLY_SUCCESS), "Closing channel");
  die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
  amqp_destroy_connection(conn);
  die_on_error(close(sockfd), "Closing socket");

  return 0;
}
