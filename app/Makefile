CC := /tmp/buildroot-2023.11/output/host/bin/arm-buildroot-linux-gnueabi-gcc
CFLAGS := -Wall -Wextra -O2
LDFLAGS := -lpthread

all: basic_app benchmark

basic_app: basic_app.c
	$(CC) $(CFLAGS) -o basic_app basic_app.c

benchmark: benchmark.c
	$(CC) $(CFLAGS) -o benchmark benchmark.c $(LDFLAGS)

clean:
	rm -f basic_app benchmark
