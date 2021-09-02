NAME := pcombine

SRCS := main.c

CFLAGS := \
	-std=gnu99 \
	-Werror \
	-Wall \
	-Wpedantic

LDFLAGS :=

CC := gcc

BUILD_PREFIX := build/

BIN := $(NAME)
OBJS := $(addprefix $(BUILD_PREFIX),$(addsuffix .o,$(SRCS)))

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $(BIN) $(LDFLAGS) $^

$(OBJS): $(BUILD_PREFIX)%.o : % | $(BUILD_PREFIX)
	$(CC) -o $@ -c $(CFLAGS) $^

$(BUILD_PREFIX):
	mkdir -p $@

.PHONY: clean
clean:
	rm -f $(OBJS)
	if [ -d $(BUILD_PREFIX) ]; then rmdir $(BUILD_PREFIX); fi

.PHONY: distclean
distclean: clean
	rm $(BIN)
