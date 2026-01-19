CC ?= clang
CFLAGS ?= -Wall -Wextra -Werror -O3 -Iengine/include -pthread -std=c17
LDFLAGS ?= -lm -pthread

SRCS = $(wildcard engine/src/*.c)
LIB_SRCS = $(filter-out engine/src/main.c, $(SRCS))
BIN_DIR = bin
TARGET = $(BIN_DIR)/voxel

TEST_SRCS = $(wildcard tests/*.c)
TEST_BINS = $(patsubst tests/%.c, $(BIN_DIR)/%, $(TEST_SRCS))

BENCH_SRCS = $(wildcard bench/*.c)
BENCH_BINS = $(patsubst bench/%.c, $(BIN_DIR)/%, $(BENCH_SRCS))

.PHONY: all clean test test-unit test-integration bench phantom server hare-check help

all: $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

$(BIN_DIR)/test_%: tests/test_%.c $(LIB_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(LIB_SRCS) $< -o $@ $(LDFLAGS)

$(BIN_DIR)/bench_%: bench/bench_%.c $(LIB_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(LIB_SRCS) $< -o $@ $(LDFLAGS)

test: test-unit test-integration
	@echo "\033[92mAll unit and integration tests completed successfully with zero defects.\033[0m"

test-unit: $(TEST_BINS)
	@echo "=== Running DICOM, DIMSE, Volumetric MPR, and Storage Unit Tests ==="
	@for t in $(TEST_BINS); do \
		echo "Running $$t..."; \
		$$t || exit 1; \
	done

test-integration: $(TARGET)
	@echo "=== Running End-to-End DIMSE & DICOMweb Integration Suite ==="
	python3 tests/integration_suite.py

bench: $(BENCH_BINS)
	@echo "=== Executing Part 10 & Volumetric Tomography Microbenchmarks ==="
	@for b in $(BENCH_BINS); do \
		$$b || exit 1; \
	done

phantom: $(TARGET)
	@mkdir -p data/phantom
	$(TARGET) phantom --slices 64 --output-dir data/phantom

server: $(TARGET)
	@echo "Starting Voxel PACS daemon (Web: http://localhost:8042/, DIMSE: 11112)..."
	$(TARGET) server --dicom-port 11112 --http-port 8042 --storage-dir data --web-root web

hare-check:
	@which hare > /dev/null 2>&1 && hare version || echo "Hare compiler not found on host. Native C17 core is active."

clean:
	rm -rf $(BIN_DIR) data/integration_phantom *.png
	@echo "Cleaned build artifacts."

help:
	@echo "Voxel Build & Orchestration Targets:"
	@echo "  make              - Build bin/voxel production executable"
	@echo "  make test         - Run full unit tests and Python integration suite"
	@echo "  make test-unit    - Run C unit tests (tags, dimse, volume, storage, bugs)"
	@echo "  make bench        - Run streaming Part 10 & MPR microbenchmarks"
	@echo "  make phantom      - Generate 64-slice 3D CT head phantom DICOM Part 10 files"
	@echo "  make server       - Launch dual-protocol PACS server on ports 11112 & 8042"
	@echo "  make clean        - Remove build artifacts and temporary data"
