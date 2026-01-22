# Compiler and flags (Linux only)
CXX = g++
CXXFLAGS = -std=c++17 -I include -Wall -pthread
LDFLAGS =

# Directories
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests

# Source files
SOURCES = $(SRC_DIR)/server_async.cpp \
          $(SRC_DIR)/resp_parser.cpp \
          $(SRC_DIR)/resp_encoder.cpp \
          $(SRC_DIR)/storage.cpp \
          $(SRC_DIR)/command_handler.cpp \
          $(SRC_DIR)/aof.cpp

# Test source files
TEST_SOURCES = $(TEST_DIR)/test_expiration.cpp \
               $(SRC_DIR)/resp_parser.cpp \
               $(SRC_DIR)/resp_encoder.cpp \
               $(SRC_DIR)/storage.cpp \
               $(SRC_DIR)/command_handler.cpp

# Output executables (Linux)
SERVER = server_async
TEST_EXE = $(TEST_DIR)/test_expiration

# Default target
all: $(SERVER)

# Build server
$(SERVER): $(SOURCES)
	@echo Building Redis server...
	$(CXX) $(CXXFLAGS) -o $(SERVER) $(SOURCES) $(LDFLAGS)
	@echo ✓ Build complete: $(SERVER)

# Build and run tests
test: $(TEST_EXE)
	@echo Running tests...
	$(TEST_EXE)

# Build test executable
$(TEST_EXE): $(TEST_SOURCES)
	@echo Building tests...
	$(CXX) $(CXXFLAGS) -o $(TEST_EXE) $(TEST_SOURCES) $(LDFLAGS)
	@echo ✓ Test build complete: $(TEST_EXE)

# Clean build artifacts
clean:
	@echo Cleaning build artifacts...
	rm -f $(SERVER) $(TEST_EXE)
	@echo ✓ Clean complete

# Rebuild from scratch
rebuild: clean all

# Run server
run: $(SERVER)
	@echo Starting Redis server on port 7379...
	$(SERVER)

# Show help
help:
	@echo Available targets:
	@echo   make          - Build the server (default)
	@echo   make test     - Build and run tests
	@echo   make clean    - Remove build artifacts
	@echo   make rebuild  - Clean and rebuild
	@echo   make run      - Build and run the server
	@echo   make help     - Show this help message

.PHONY: all test clean rebuild run help
