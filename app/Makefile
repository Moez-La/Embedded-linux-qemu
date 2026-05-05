CC := /tmp/buildroot-2023.11/output/host/bin/arm-buildroot-linux-gnueabi-gcc
CFLAGS := -Wall -Wextra -O2
LDFLAGS := -lpthread

TARGET := app
SRC := app.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
