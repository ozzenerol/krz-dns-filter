TARGET      := krz-dns-filter

INCLUDE_DIR := include
SRC_DIR     := src
BIN_DIR     := bin
BUILD_DIR   := build

CC          := cc
CSTD        := -std=c11
WARNFLAGS   := -Wall -Wextra
CPPFLAGS    := -I$(INCLUDE_DIR) -I$(INCLUDE_DIR)/vendor -MMD -MP
CFLAGS      := $(CSTD) $(WARNFLAGS) -O2
LDFLAGS     :=
LDLIBS      := -pthread

ifeq ($(DEBUG),1)
    CFLAGS := $(CSTD) $(WARNFLAGS) -g -O0 -DDEBUG -fsanitize=address,undefined
    LDFLAGS += -fsanitize=address,undefined
endif

SRCS        := $(wildcard $(SRC_DIR)/*.c)
OBJS        := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS        := $(OBJS:.o=.d)

BINARY      := $(BIN_DIR)/$(TARGET)

.PHONY: all debug run clean distclean dirs

all: dirs $(BINARY)

debug:
	$(MAKE) DEBUG=1

$(BINARY): $(OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

dirs: $(BIN_DIR) $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: all
	./$(BINARY)

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d

distclean: clean
	rm -f $(BINARY)

-include $(DEPS)
