CC = gcc
AR = ar

DEPS = json-c x11

CFLAGS   = -Iinclude -Wall -O2 $(shell pkg-config --cflags $(DEPS))
LDFLAGS  = $(shell pkg-config --libs $(DEPS))

LIB_SRC  = src/ukb.c $(wildcard src/backends/*.c)
LIB_OBJ  = $(LIB_SRC:.c=.o)

BIN_SRC  = src/main.c
BIN_OBJ  = $(BIN_SRC:.c=.o)

PREFIX       ?= /usr/local
BIN_DIR      = $(PREFIX)/bin
LIB_DIR      = $(PREFIX)/lib
INCLUDE_DIR  = $(PREFIX)/include/libukb

all: libukb.a libukb.so ukb

$(LIB_OBJ): %.o: %.c
	$(CC) $(CFLAGS) -fPIC -DUKB_BACKENDS_INTERNAL -c $< -o $@

$(BIN_OBJ): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

libukb.so: $(LIB_OBJ)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

libukb.a: $(LIB_OBJ)
	$(AR) rcs $@ $^

ukb: $(BIN_OBJ) libukb.a
	$(CC) $(BIN_OBJ) -L. -l:libukb.a -o $@ $(LDFLAGS)

.PHONY: install
install: all
	install -d $(BIN_DIR) $(LIB_DIR) $(INCLUDE_DIR)
	install -m 0755 ukb $(BIN_DIR)/
	install -m 0644 libukb.a libukb.so $(LIB_DIR)/
	install -m 0644 include/*.h $(INCLUDE_DIR)/

.PHONY: uninstall
uninstall:
	rm -f $(BIN_DIR)/ukb
	rm -f $(LIB_DIR)/libukb.a $(LIB_DIR)/libukb.so
	rm -rf $(INCLUDE_DIR)

.PHONY: clean
clean:
	rm -f $(LIB_OBJ) $(BIN_OBJ) ukb libukb.so libukb.a
