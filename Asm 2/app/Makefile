# SMS Sensor Monitoring System
# Author: Tran Nguyen Hai Dang
# Student ID: SE202009

CROSS_COMPILE ?= arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc

CPPFLAGS += -I../driver
CFLAGS += -Wall -Wextra -Werror -O2 -pthread
LDFLAGS += -pthread
LDLIBS += -lrt -lm

TARGET := sms_app

SOURCES := \
	sms_app.c \
	sensor_thread.c \
	processor_thread.c \
	alert_thread.c \
	logger_thread.c \
	circular_buffer.c

OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean native

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

native:
	$(MAKE) clean
	$(MAKE) CROSS_COMPILE= CC=gcc

clean:
	rm -f $(OBJECTS) $(TARGET)
