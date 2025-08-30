# SPDX-FileCopyrightText: 2025 Mohamed Hamdi <haamdi@outlook.com>
#
# SPDX-License-Identifier: MPL-2.0

CC = gcc
CFLAGS = -std=c23 -Wall -Wextra -O2 -D_GNU_SOURCE -pedantic
TARGET = netspeed
SOURCE = main.c
INSTALL_DIR = /usr/local/bin

.PHONY: all clean install uninstall debug help

all: $(TARGET)

$(TARGET): $(SOURCE)
	@$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

install: $(TARGET)
	@echo "Installing $(TARGET) to $(INSTALL_DIR)..."
	@mkdir -p $(INSTALL_DIR)
	@cp $(TARGET) $(INSTALL_DIR)/$(TARGET)
	@chmod 755 $(INSTALL_DIR)/$(TARGET)
	@echo "Installation complete. You can now run '$(TARGET)'"

uninstall:
	@echo "Removing $(TARGET) from $(INSTALL_DIR)..."
	@rm -f $(INSTALL_DIR)/$(TARGET)
	@echo "Uninstall complete."


help:
	@echo "Available targets:"
	@echo "  all       - Build the program (default)"
	@echo "  clean     - Remove built files"
	@echo "  install   - Install to $(INSTALL_DIR)"
	@echo "  uninstall - Remove from $(INSTALL_DIR)"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - GCC 14+ or Clang 18+ for C23 support"
