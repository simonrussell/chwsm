CC?=gcc
AR?=ar

CPPFLAGS += -I.
CPPFLAGS_DEBUG = $(CPPFLAGS) -DHTTP_PARSER_STRICT=1 -DHTTP_PARSER_DEBUG=1
CPPFLAGS_DEBUG += $(CPPFLAGS_DEBUG_EXTRA)
CPPFLAGS_FAST = $(CPPFLAGS) -DHTTP_PARSER_STRICT=0 -DHTTP_PARSER_DEBUG=0
CPPFLAGS_FAST += $(CPPFLAGS_FAST_EXTRA)

CFLAGS += -Wall -Wextra -Werror -std=c99
CFLAGS_DEBUG = $(CFLAGS) -O0 -g $(CFLAGS_DEBUG_EXTRA)
CFLAGS_FAST = $(CFLAGS) -O3 $(CFLAGS_FAST_EXTRA)

LDFLAGS += -lev

run: server2
	./server2

server2: server2.o http_parser.o
	$(CC) $(CFLAGS_DEBUG) server2.o http_parser.o $(LDFLAGS) -o $@
  
server2.o: server2.c
	$(CC) $(CPPFLAGS_DEBUG) $(CFLAGS_DEBUG) -c server2.c -o $@
	
http_parser.o: http_parser.c
	$(CC) $(CPPFLAGS_DEBUG) $(CFLAGS_DEBUG) -c http_parser.c -o $@
