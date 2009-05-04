#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "amqp.h"
#include "amqp_framing.h"
#include "amqp_private.h"

#include <assert.h>

int amqp_basic_publish(amqp_connection_state_t state,
		       amqp_bytes_t exchange,
		       amqp_bytes_t routing_key,
		       amqp_boolean_t mandatory,
		       amqp_boolean_t immediate,
		       amqp_basic_properties_t const *properties,
		       amqp_bytes_t body)
{
  amqp_frame_t f;
  size_t body_offset;
  size_t usable_body_payload_size = state->frame_max - (HEADER_SIZE + FOOTER_SIZE);

  amqp_basic_publish_t m =
    (amqp_basic_publish_t) {
      .exchange = exchange,
      .routing_key = routing_key,
      .mandatory = mandatory,
      .immediate = immediate
    };

  amqp_basic_properties_t default_properties;

  AMQP_CHECK_RESULT(amqp_send_method(state, 1, AMQP_BASIC_PUBLISH_METHOD, &m));

  if (properties == NULL) {
    memset(&default_properties, 0, sizeof(default_properties));
    properties = &default_properties;
  }

  f.frame_type = AMQP_FRAME_HEADER;
  f.channel = 1;
  f.payload.properties.class_id = AMQP_BASIC_CLASS;
  f.payload.properties.body_size = body.len;
  f.payload.properties.decoded = (void *) properties;
  AMQP_CHECK_RESULT(amqp_send_frame(state, &f));

  body_offset = 0;
  while (1) {
    int remaining = body.len - body_offset;
    assert(remaining >= 0);

    if (remaining == 0)
      break;

    f.frame_type = AMQP_FRAME_BODY;
    f.channel = 1;
    f.payload.body_fragment.bytes = BUF_AT(body, body_offset);
    if (remaining >= usable_body_payload_size) {
      f.payload.body_fragment.len = usable_body_payload_size;
    } else {
      f.payload.body_fragment.len = remaining;
    }

    body_offset += f.payload.body_fragment.len;
    AMQP_CHECK_RESULT(amqp_send_frame(state, &f));
  }

  return 0;
}

amqp_rpc_reply_t amqp_channel_close(amqp_connection_state_t state, int code) {
  amqp_channel_close_t s =
    (amqp_channel_close_t) {
      .reply_code = code,
      .reply_text = {.len = 0, .bytes = NULL},
      .class_id = 0,
      .method_id = 0
    };
  return amqp_simple_rpc(state,
			 1,
			 AMQP_CHANNEL_CLOSE_METHOD,
			 AMQP_CHANNEL_CLOSE_OK_METHOD,
			 &s);
}

amqp_rpc_reply_t amqp_connection_close(amqp_connection_state_t state, int code) {
  amqp_connection_close_t s =
    (amqp_connection_close_t) {
      .reply_code = code,
      .reply_text = {.len = 0, .bytes = NULL},
      .class_id = 0,
      .method_id = 0
    };
  return amqp_simple_rpc(state,
			 0,
			 AMQP_CONNECTION_CLOSE_METHOD,
			 AMQP_CONNECTION_CLOSE_OK_METHOD,
			 &s);
}
