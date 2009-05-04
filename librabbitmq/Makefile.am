lib_LTLIBRARIES = librabbitmq.la

librabbitmq_la_SOURCES = amqp_mem.c amqp_table.c amqp_connection.c amqp_socket.c amqp_debug.c amqp_api.c
nodist_librabbitmq_la_SOURCES = amqp_framing.c
include_HEADERS = amqp_framing.h amqp.h
noinst_librabbitmq_la_INCLUDES = amqp_private.h
BUILT_SOURCES = amqp_framing.h amqp_framing.c
CLEANFILES = amqp_framing.h amqp_framing.c

PYTHON = python2.5

amqp_framing.h: $(AMQP_SPEC_JSON_PATH) codegen.py
	PYTHONPATH=$(AMQP_CODEGEN_DIR) $(PYTHON) codegen.py header $< $@

amqp_framing.c: $(AMQP_SPEC_JSON_PATH) codegen.py
	PYTHONPATH=$(AMQP_CODEGEN_DIR) $(PYTHON) codegen.py body $< $@
