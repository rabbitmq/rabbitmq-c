#ifndef librabbitmq_amqp_private_h
#define librabbitmq_amqp_private_h

#ifdef __cplusplus
extern "C" {
#endif

struct amqp_connection_state_t_ {
  amqp_pool_t pool;
  amqp_bytes_t inbound_buffer;
  amqp_bytes_t outbound_buffer;
  size_t frame_offset;
  size_t frame_size_target; /* 0 for unknown, still waiting for header */
  amqp_boolean_t reset_required;
};

#define CHECK_LIMIT(b, o, l, v) ({ if ((o + l) > (b).len) { return -EFAULT; } (v); })
#define BUF_AT(b, o) (&(((uint8_t *) (b).bytes)[o]))

#define D_8(b, o) CHECK_LIMIT(b, o, 1, * (uint8_t *) BUF_AT(b, o))
#define D_16(b, o) CHECK_LIMIT(b, o, 2, ({uint16_t v; memcpy(&v, BUF_AT(b, o), 2); ntohs(v);}))
#define D_32(b, o) CHECK_LIMIT(b, o, 4, ({uint32_t v; memcpy(&v, BUF_AT(b, o), 4); ntohl(v);}))
#define D_64(b, o) ({				\
  uint64_t hi = D_32(b, o);			\
  uint64_t lo = D_32(b, o + 4);			\
  hi << 32 | lo;				\
})

#define D_BYTES(b, o, l) CHECK_LIMIT(b, o, l, BUF_AT(b, o))

#define E_8(b, o, v) CHECK_LIMIT(b, o, 1, * (uint8_t *) BUF_AT(b, o) = (v))
#define E_16(b, o, v) CHECK_LIMIT(b, o, 2, ({uint16_t vv = htons(v); memcpy(BUF_AT(b, o), &vv, 2);}))
#define E_32(b, o, v) CHECK_LIMIT(b, o, 4, ({uint32_t vv = htonl(v); memcpy(BUF_AT(b, o), &vv, 4);}))
#define E_64(b, o, v) ({			\
  E_32(b, o, (uint32_t) (v >> 32));		\
  E_32(b, o, (uint32_t) (v & 0xFFFFFFFF));	\
})

#define E_BYTES(b, o, l, v) CHECK_LIMIT(b, o, l, memcpy(BUF_AT(b, o), (v), (l)))

extern int amqp_decode_table(amqp_bytes_t encoded,
			     amqp_pool_t *pool,
			     amqp_table_t *output,
			     int *offsetptr);

extern int amqp_encode_table(amqp_bytes_t encoded,
			     amqp_table_t *input,
			     int *offsetptr);

#ifdef __cplusplus
}
#endif

#endif
