bin_PROGRAMS = amqp_sendstring amqp_exchange_declare amqp_listen

AM_CFLAGS = -I../librabbitmq
AM_LDFLAGS = ../librabbitmq/librabbitmq.la

nodist_INCLUDES = example_utils.h

amqp_sendstring_SOURCES = amqp_sendstring.c example_utils.c
amqp_exchange_declare_SOURCES = amqp_exchange_declare.c example_utils.c
amqp_listen_SOURCES = amqp_listen.c example_utils.c
