# ==============================================================================
# Linux Memory-Mapped File Management System - Makefile
# ==============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -g
TARGET = memory_manager
SRC = main.c

.PHONY: all run clean help test

all: $(TARGET)

$(TARGET): $(SRC)
	@echo "[*] Compiling Linux Memory-Mapped File Management System..."
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)
	@echo "[+] Build complete: ./"$(TARGET)

run: all
	@echo "[*] Running Memory Manager..."
	./$(TARGET)

clean:
	@echo "[*] Cleaning up build artifacts and temporary files..."
	rm -f $(TARGET) sample_files/demo_mmap.txt sample_files/test*.txt
	@echo "[+] Cleanup complete."

help:
	@echo "Available commands:"
	@echo "  make        - Compile the project with GCC (-Wall -Wextra -std=c11)"
	@echo "  make run    - Compile and launch the interactive program"
	@echo "  make clean  - Remove executable and test output files"
