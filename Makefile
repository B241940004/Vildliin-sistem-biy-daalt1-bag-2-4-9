CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -pthread

tpdemo: tpdemo.c threadpool.c threadpool.h
	$(CC) $(CFLAGS) -o $@ tpdemo.c threadpool.c

tsan:
	$(CC) -O1 -g -Wall -Wextra -pthread -fsanitize=thread -o tpdemo tpdemo.c threadpool.c

test: tpdemo
	sh tests/run_tests.sh

clean:
	rm -f tpdemo

.PHONY: tsan test clean
