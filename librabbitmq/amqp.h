#ifndef librabbitmq_amqp_h
#define librabbitmq_amqp_h

#ifdef __cplusplus
extern "C" {
#endif

typedef int amqp_boolean_t;
typedef uint32_t amqp_method_number_t;
typedef uint32_t amqp_flags_t;
typedef uint16_t amqp_channel_t;

typedef struct amqp_bytes_t_ {
  size_t len;
  void *bytes;
} amqp_bytes_t;

typedef struct amqp_decimal_t_ {
  int decimals;
  uint32_t value;
} amqp_decimal_t;

typedef struct amqp_table_t_ {
  int num_entries;
  struct amqp_table_entry_t_ *entries;
} amqp_table_t;

typedef struct amqp_table_entry_t_ {
  amqp_bytes_t key;
  char kind;
  union {
    amqp_bytes_t bytes;
    int32_t i32;
    amqp_decimal_t decimal;
    uint64_t u64;
    amqp_table_t table;
  } value;
} amqp_table_entry_t;

typedef struct amqp_pool_blocklist_t_ {
  int num_blocks;
  void **blocklist;
} amqp_pool_blocklist_t;

typedef struct amqp_pool_t_ {
  size_t pagesize;

  amqp_pool_blocklist_t pages;
  amqp_pool_blocklist_t large_blocks;

  int next_page;
  char *alloc_block;
  size_t alloc_used;
} amqp_pool_t;

typedef struct amqp_method_t_ {
  amqp_method_number_t id;
  void *decoded;
} amqp_method_t;

typedef struct amqp_frame_t_ {
  uint8_t frame_type; /* 0 means no event */
  amqp_channel_t channel;
  union {
    amqp_method_t method;
    struct {
      uint16_t class_id;
      uint64_t body_size;
      void *decoded;
    } properties;
    amqp_bytes_t body_fragment;
  } payload;
} amqp_frame_t;

typedef enum amqp_response_type_enum_ {
  AMQP_RESPONSE_NONE = 0,
  AMQP_RESPONSE_NORMAL,
  AMQP_RESPONSE_LIBRARY_EXCEPTION,
  AMQP_RESPONSE_SERVER_EXCEPTION
} amqp_response_type_enum;

typedef struct amqp_rpc_reply_t_ {
  amqp_response_type_enum reply_type;
  amqp_method_t reply;
  int library_errno; /* if AMQP_RESPONSE_LIBRARY_EXCEPTION, then 0 here means socket EOF */
} amqp_rpc_reply_t;

typedef enum amqp_sasl_method_enum_ {
  AMQP_SASL_METHOD_PLAIN = 0
} amqp_sasl_method_enum;

/* Opaque struct. */
typedef struct amqp_connection_state_t_ *amqp_connection_state_t;

extern char const *amqp_version(void);

extern void init_amqp_pool(amqp_pool_t *pool, size_t pagesize);
extern void recycle_amqp_pool(amqp_pool_t *pool);
extern void empty_amqp_pool(amqp_pool_t *pool);

extern void *amqp_pool_alloc(amqp_pool_t *pool, size_t amount);
extern void amqp_pool_alloc_bytes(amqp_pool_t *pool, size_t amount, amqp_bytes_t *output);

extern amqp_bytes_t amqp_cstring_bytes(char const *cstr);
extern amqp_bytes_t amqp_bytes_malloc_dup(amqp_bytes_t src);

extern amqp_connection_state_t amqp_new_connection(void);
extern void amqp_set_sockfd(amqp_connection_state_t state,
			    int sockfd);
extern int amqp_tune_connection(amqp_connection_state_t state,
				int frame_max);
extern void amqp_destroy_connection(amqp_connection_state_t state);

extern int amqp_handle_input(amqp_connection_state_t state,
			     amqp_bytes_t received_data,
			     amqp_frame_t *decoded_frame);

extern amqp_boolean_t amqp_release_buffers_ok(amqp_connection_state_t state);

extern void amqp_release_buffers(amqp_connection_state_t state);

extern void amqp_maybe_release_buffers(amqp_connection_state_t state);

extern int amqp_send_frame(amqp_connection_state_t state,
			   amqp_frame_t const *frame);

extern int amqp_table_entry_cmp(void const *entry1, void const *entry2);

extern int amqp_open_socket(char const *hostname, int portnumber);

extern int amqp_send_header(amqp_connection_state_t state);

extern amqp_boolean_t amqp_frames_enqueued(amqp_connection_state_t state);

extern int amqp_simple_wait_frame(amqp_connection_state_t state,
				  amqp_frame_t *decoded_frame);

extern int amqp_simple_wait_method(amqp_connection_state_t state,
				   amqp_method_number_t expected_or_zero,
				   amqp_method_t *output);

extern int amqp_send_method(amqp_connection_state_t state,
			    amqp_channel_t channel,
			    amqp_method_number_t id,
			    void *decoded);

extern amqp_rpc_reply_t amqp_simple_rpc(amqp_connection_state_t state,
					amqp_channel_t channel,
					amqp_method_number_t request_id,
					amqp_method_number_t expected_reply_id,
					void *decoded_request_method);

extern amqp_rpc_reply_t amqp_login(amqp_connection_state_t state,
				   char const *vhost,
				   int frame_max,
				   amqp_sasl_method_enum sasl_method, ...);

struct amqp_basic_properties_t_;
extern int amqp_basic_publish(amqp_connection_state_t state,
			      amqp_bytes_t exchange,
			      amqp_bytes_t routing_key,
			      amqp_boolean_t mandatory,
			      amqp_boolean_t immediate,
			      struct amqp_basic_properties_t_ const *properties,
			      amqp_bytes_t body);

extern amqp_rpc_reply_t amqp_channel_close(amqp_connection_state_t state, int code);
extern amqp_rpc_reply_t amqp_connection_close(amqp_connection_state_t state, int code);

#ifdef __cplusplus
}
#endif

#endif
