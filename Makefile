# ---------------------------------------------------------------------------
#  make          bien dich ra ./calc
#  make test     chay unit test
#  make wasm     bien dich loi tinh toan sang WebAssembly cho ban demo web (can clang)
#  make clean    xoa file sinh ra
# ---------------------------------------------------------------------------
CC      ?= gcc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra -Wpedantic
SRC_DIR  = src

CORE     = $(SRC_DIR)/calc.c $(SRC_DIR)/mathx.c
APP      = $(CORE) $(SRC_DIR)/history.c $(SRC_DIR)/main.c
HEADERS  = $(wildcard $(SRC_DIR)/*.h)

TARGET   = calc
TEST_BIN = tests/test_calc
WASM_OUT = docs/calc.wasm

.PHONY: all test wasm clean

all: $(TARGET)

$(TARGET): $(APP) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(APP)

# Test so sanh voi <math.h> nen moi can -lm (ban chinh khong can).
$(TEST_BIN): tests/test_calc.c $(CORE) $(SRC_DIR)/history.c $(HEADERS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -o $@ tests/test_calc.c $(CORE) $(SRC_DIR)/history.c -lm

test: $(TEST_BIN)
	./$(TEST_BIN)

# Loi tinh toan khong dung thu vien chuan C -> bien dich thang sang wasm32,
# khong can Emscripten.
wasm: $(WASM_OUT)

$(WASM_OUT): $(CORE) $(SRC_DIR)/wasm_api.c $(HEADERS)
	clang --target=wasm32 -std=c99 -O2 -Wall -Wextra -nostdlib -fno-builtin \
		-Wl,--no-entry -Wl,--export-dynamic -Wl,--strip-all \
		-o $@ $(CORE) $(SRC_DIR)/wasm_api.c

clean:
	rm -f $(TARGET) $(TARGET).exe $(TEST_BIN) $(TEST_BIN).exe
