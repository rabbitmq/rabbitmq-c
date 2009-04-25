#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "amqp.h"
#include "amqp_framing.h"
#include "amqp_private.h"

#define INITIAL_FRAME_MAX 65536

amqp_connection_state_t amqp_new_connection(void) {
  amqp_connection_state_t state =
    (amqp_connection_state_t) calloc(1, sizeof(struct amqp_connection_state_t_));
  init_amqp_pool(&state->pool, INITIAL_FRAME_MAX);
  state->inbound_buffer.len = INITIAL_FRAME_MAX;
  state->inbound_buffer.bytes = malloc(INITIAL_FRAME_MAX);
  state->outbound_buffer.len = INITIAL_FRAME_MAX;
  state->outbound_buffer.bytes = malloc(INITIAL_FRAME_MAX);
  state->frame_size_target = 0;
  state->reset_required = 0;
  return state;
}

void amqp_tune_connection(amqp_connection_state_t state,
			  int frame_max)
{
  empty_amqp_pool(&state->pool);
  init_amqp_pool(&state->pool, frame_max);
  state->inbound_buffer.len = frame_max;
  realloc(state->inbound_buffer.bytes, frame_max);
  state->outbound_buffer.len = frame_max;
  realloc(state->outbound_buffer.bytes, frame_max);
}

void amqp_destroy_connection(amqp_connection_state_t state) {
  empty_amqp_pool(&state->pool);
  free(state->inbound_buffer.bytes);
  free(state->outbound_buffer.bytes);
  free(state);
}

int amqp_handle_input(amqp_connection_state_t state,
		      amqp_bytes_t received_data,
		      amqp_frame_t *decoded_frame)
{
  if (state->reset_required) {
    size_t unconsumed_byte_count = state->frame_offset - state->frame_size_target;
    recycle_amqp_pool(&state->pool);
    memmove(state->inbound_buffer.bytes,
	    D_BYTES(state->inbound_buffer,
		    state->frame_size_target,
		    unconsumed_byte_count),
	    unconsumed_byte_count);
    state->frame_offset = unconsumed_byte_count;
    state->frame_size_target = 0;
    state->reset_required = 0;
  }

  E_BYTES(state->inbound_buffer, state->frame_offset, received_data.len, received_data.bytes);
  state->frame_offset += received_data.len;

  if (state->frame_size_target == 0) {
    if (state->frame_offset < 7) {
      return AMQP_FRAME_INCOMPLETE;
    }

    state->frame_size_target = D_32(state->inbound_buffer, 3) + 8; /* 7 for header, 1 for footer */
  }

  if (state->frame_offset < state->frame_size_target) {
    return AMQP_FRAME_INCOMPLETE;
  }

  if (D_8(state->inbound_buffer, state->frame_size_target - 1) != AMQP_FRAME_END) {
    return -EINVAL;
  }

  state->reset_required = 1;

  decoded_frame->frame_type = D_8(state->inbound_buffer, 0);
  decoded_frame->channel = D_16(state->inbound_buffer, 1);
  switch (decoded_frame->frame_type) {
    case AMQP_FRAME_METHOD: {
      amqp_bytes_t encoded;
      int decode_result;

      encoded.len = state->frame_size_target - 12; /* 7 for header, 4 for method id, 1 for footer */
      encoded.bytes = D_BYTES(state->inbound_buffer, 11, encoded.len);

      decoded_frame->payload.method.id = D_32(state->inbound_buffer, 7);
      decode_result = amqp_decode_method(decoded_frame->payload.method.id,
					 &state->pool,
					 encoded,
					 &decoded_frame->payload.method.decoded);
      if (decode_result != 0) return decode_result;

      return AMQP_FRAME_COMPLETE;
    }
    case AMQP_FRAME_HEADER: {
      amqp_bytes_t encoded;
      int decode_result;

      encoded.len = state->frame_size_target - 20; /* 7 for header, 12 for prop hdr, 1 for footer */
      encoded.bytes = D_BYTES(state->inbound_buffer, 19, encoded.len);

      decoded_frame->payload.properties.class_id = D_16(state->inbound_buffer, 7);
      decode_result = amqp_decode_properties(decoded_frame->payload.properties.class_id,
					     &state->pool,
					     encoded,
					     &decoded_frame->payload.properties.decoded);
      if (decode_result != 0) return decode_result;

      return AMQP_FRAME_COMPLETE;
    }
    case AMQP_FRAME_BODY: {
      size_t fragment_len = state->frame_size_target - 8; /* 7 for header, 1 for footer */
      decoded_frame->payload.body_fragment.len = fragment_len;
      decoded_frame->payload.body_fragment.bytes = D_BYTES(state->inbound_buffer, 7, fragment_len);
      return AMQP_FRAME_COMPLETE;
    }
    default:
      /* Ignore the frame */
      return AMQP_FRAME_INCOMPLETE;
  }
}

int amqp_send_frame(amqp_connection_state_t state,
		    amqp_writer_fun_t writer,
		    int context,
		    amqp_frame_t const *frame)
{
  amqp_bytes_t encoded;
  int payload_len;
  int separate_body;

  E_8(state->outbound_buffer, 0, frame->frame_type);
  E_16(state->outbound_buffer, 1, frame->channel);
  switch (frame->frame_type) {
    case AMQP_FRAME_METHOD: {
      encoded.len = state->outbound_buffer.len - 8;
      encoded.bytes = D_BYTES(state->outbound_buffer, 7, encoded.len);
      payload_len = amqp_encode_method(frame->payload.method.id,
				       frame->payload.method.decoded,
				       encoded);
      separate_body = 0;
      break;
    }
    case AMQP_FRAME_HEADER: {
      encoded.len = state->outbound_buffer.len - 8;
      encoded.bytes = D_BYTES(state->outbound_buffer, 7, encoded.len);
      payload_len = amqp_encode_properties(frame->payload.properties.class_id,
					   frame->payload.properties.decoded,
					   encoded);
      separate_body = 0;
      break;
    }
    case AMQP_FRAME_BODY: {
      encoded = frame->payload.body_fragment;
      payload_len = encoded.len;
      separate_body = 1;
      break;
    }
    default:
      return -EINVAL;
  }

  if (payload_len < 0) {
    return payload_len;
  }

  E_32(state->outbound_buffer, 3, payload_len);

  if (separate_body) {
    int result = writer(context, state->outbound_buffer.bytes, 7);
    char frame_end_byte = AMQP_FRAME_END;
    if (result < 0) return result;
    result = writer(context, encoded.bytes, payload_len);
    if (result < 0) return result;
    return writer(context, &frame_end_byte, 1);
  } else {
    E_8(state->outbound_buffer, payload_len + 7, AMQP_FRAME_END);
    return writer(context, state->outbound_buffer.bytes, payload_len + 8);
  }
}
