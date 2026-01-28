VERSION := $(shell git describe --tags --always --dirty 2>/dev/null || echo "unknown")
AUTHOR = "James Garrick"

CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -DVERSION=\"$(VERSION)\" -DAUTHOR=\"$(AUTHOR)\"

TARGET = gsr
SRC = gsr.cpp

@$(TARGET): $(SRC)
	@$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)
	@strip $(TARGET)


PREFIX = $(HOME)/.local

install: $(TARGET)
	mkdir -p $(PREFIX)/bin
	install -s -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: clean install uninstall
