CC?=gcc
AR?=ar

CPPFLAGS += -I.
CPPFLAGS_DEBUG = $(CPPFLAGS) -DHTTP_PARSER_STRICT=1 -DHTTP_PARSER_DEBUG=1
CPPFLAGS_DEBUG += $(CPPFLAGS_DEBUG_EXTRA)
CPPFLAGS_FAST = $(CPPFLAGS) -DHTTP_PARSER_STRICT=0 -DHTTP_PARSER_DEBUG=0
CPPFLAGS_FAST += $(CPPFLAGS_FAST_EXTRA)

CFLAGS += -Fno-strict-aliasing -Wno-strict-aliasing -Wall -Wextra -Werror -std=c99
CFLAGS_DEBUG = $(CFLAGS) -O0 -g $(CFLAGS_DEBUG_EXTRA)
CFLAGS_FAST = $(CFLAGS) -O2 $(CFLAGS_FAST_EXTRA)

LDFLAGS += -lev

run: server2
	./server2

server2: server2.o http_parser.o http_connection.o rope.o
	$(CC) $(CFLAGS_DEBUG) $^ $(LDFLAGS) -o $@
  
.c.o:
	$(CC) $(CPPFLAGS_DEBUG) $(CFLAGS_DEBUG) -c $< -o $@

clean:
	rm *.o server2
