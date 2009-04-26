#ifndef librabbitmq_examples_example_utils_h
#define librabbitmq_examples_example_utils_h

extern void die_on_error(int x, char const *context);
extern void die_on_amqp_error(amqp_rpc_reply_t x, char const *context);

#endif
