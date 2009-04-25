#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "amqp.h"
#include "amqp_framing.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

int amqp_open_socket(char const *hostname,
		     int portnumber)
{
  int sockfd;
  struct sockaddr_in addr;
  struct hostent *he;

  he = gethostbyname(hostname);
  if (he == NULL) {
    return -ENOENT;
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(portnumber);
  addr.sin_addr.s_addr = * (uint32_t *) he->h_addr_list[0];

  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    int result = -errno;
    close(sockfd);
    return result;
  }

  return sockfd;
}

int amqp_send_header(amqp_writer_fun_t writer, int context) {
  char header[8];
  header[0] = 'A';
  header[1] = 'M';
  header[2] = 'Q';
  header[3] = 'P';
  header[4] = 1;
  header[5] = 1;
  header[6] = AMQP_PROTOCOL_VERSION_MAJOR;
  header[7] = AMQP_PROTOCOL_VERSION_MINOR;
  return writer(context, &header[0], 8);
}

int amqp_simple_wait_frame(amqp_connection_state_t state,
			   int sockfd,
			   amqp_frame_t *decoded_frame)
{
  amqp_bytes_t buffer;
  char buffer_bytes[4096];
  buffer.bytes = buffer_bytes;
  while (1) {
    int result = read(sockfd, buffer_bytes, sizeof(buffer_bytes));
    if (result < 0) {
      return -errno;
    }
    buffer.len = result;
    switch ((result = amqp_handle_input(state, buffer, decoded_frame))) {
      case AMQP_FRAME_COMPLETE:
	return AMQP_FRAME_COMPLETE;
      case AMQP_FRAME_INCOMPLETE:
	break;
      default:
	return result;
    }
  }
}
