CC ?= clang
CLANG_FORMAT ?= clang-format
CLANG_TIDY ?= clang-tidy
TIDY_CC ?= clang
BEAR ?= bear
BUILD_DIR := build
GAME := $(BUILD_DIR)/game
TEST := $(BUILD_DIR)/test_logic

WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion
CFLAGS ?= -std=c11 -O2 -g
CFLAGS += $(WARNINGS) -Iinclude
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS := $(shell pkg-config --libs raylib 2>/dev/null)

GAME_SRCS := src/main.c src/game.c src/logic.c src/audio.c src/stage.c
TEST_SRCS := tests/test_logic.c src/logic.c
FORMAT_FILES := include/audio.h include/game.h include/logic.h include/stage.h src/audio.c src/main.c src/game.c src/logic.c src/stage.c tests/test_logic.c
LINT_SRCS := src/audio.c src/main.c src/game.c src/logic.c src/stage.c tests/test_logic.c

.PHONY: all game test check fmt fmt-check compile-commands lint run clean

all: game test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

game: $(GAME)

$(GAME): $(GAME_SRCS) include/audio.h include/game.h include/logic.h include/stage.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) $(GAME_SRCS) -o $@ $(RAYLIB_LIBS) -lm

test: $(TEST)

$(TEST): $(TEST_SRCS) include/logic.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_SRCS) -o $@ -lm

check: $(TEST)
	./$(TEST)

fmt:
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

fmt-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)

compile-commands: | $(BUILD_DIR)
	rm -f compile_commands.json $(BUILD_DIR)/compile_commands.json
	$(BEAR) --output compile_commands.json -- $(MAKE) clean all CC="$(TIDY_CC)"
	mkdir -p $(BUILD_DIR)
	cp compile_commands.json $(BUILD_DIR)/compile_commands.json

lint: compile-commands
	EXTRA_ARGS="$$(sh scripts/clang-tidy-extra-args.sh "$(TIDY_CC)")"; \
	$(CLANG_TIDY) -p $(BUILD_DIR) $$EXTRA_ARGS $(LINT_SRCS)

run: $(GAME)
	./$(GAME)

clean:
	rm -rf $(BUILD_DIR)
