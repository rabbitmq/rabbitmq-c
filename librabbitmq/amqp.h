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

typedef struct amqp_frame_t_ {
  uint8_t frame_type; /* 0 means no event */
  amqp_channel_t channel;
  union {
    struct {
      amqp_method_number_t id;
      void *decoded;
    } method;
    struct {
      uint16_t class_id;
      void *decoded;
    } properties;
    amqp_bytes_t body_fragment;
  } payload;
} amqp_frame_t;

#define AMQP_EXCEPTION_CATEGORY_UNKNOWN 0
#define AMQP_EXCEPTION_CATEGORY_CONNECTION 1
#define AMQP_EXCEPTION_CATEGORY_CHANNEL 2

/* Opaque struct. */
typedef struct amqp_connection_state_t_ *amqp_connection_state_t;

extern char const *amqp_version(void);

extern void init_amqp_pool(amqp_pool_t *pool, size_t pagesize);
extern void recycle_amqp_pool(amqp_pool_t *pool);
extern void empty_amqp_pool(amqp_pool_t *pool);

extern void *amqp_pool_alloc(amqp_pool_t *pool, size_t amount);

extern amqp_connection_state_t amqp_new_connection(void);
extern int amqp_tune_connection(amqp_connection_state_t state,
				int frame_max);
extern int amqp_destroy_connection(amqp_connection_state_t state);

extern int amqp_handle_input(amqp_connection_state_t state,
			     amqp_pool_t *pool,
			     amqp_bytes_t received_data,
			     amqp_frame_t *decoded_frame);

extern int amqp_send_frame(amqp_connection_state_t state,
			   amqp_pool_t *pool,
			   int (*writer)(int context, void const *buf, size_t len),
			   int context,
			   amqp_frame_t const *frame);

extern int amqp_table_entry_cmp(void const *entry1, void const *entry2);

#ifdef __cplusplus
}
#endif

#endif
