#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <amqp.h>

#include <unistd.h>

static void die_on_error(int x, char const *context) {
  if (x < 0) {
    fprintf(stderr, "%s: %s\n", context, strerror(-x));
    exit(1);
  }
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

  die_on_error(sockfd = amqp_open_socket(hostname, port), "Opening socket");
  amqp_send_header((amqp_writer_fun_t) write, sockfd);

  conn = amqp_new_connection();

  while (1) {
    amqp_frame_t frame;
    printf("Result %d\n", amqp_simple_wait_frame(conn, sockfd, &frame));
    printf("Frame type %d, channel %d\n", frame.frame_type, frame.channel);
  }

  amqp_destroy_connection(conn);

  die_on_error(close(sockfd), "Closing socket");

  return 0;
}
