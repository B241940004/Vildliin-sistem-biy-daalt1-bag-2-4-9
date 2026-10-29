CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread

tpdemo: src/tpdemo.c src/threadpool.c src/threadpool.h
	$(CC) $(CFLAGS) -o tpdemo src/tpdemo.c src/threadpool.c

test: tpdemo
	sh tests/run_tests.sh

clean:
	rm -f tpdemo
