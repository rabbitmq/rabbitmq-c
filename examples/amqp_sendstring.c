#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include "amqp.h"

int main(int argc, char const * const *argv) {
  amqp_connection_state_t conn = amqp_new_connection();
  amqp_destroy_connection(conn);
  return 0;
}
