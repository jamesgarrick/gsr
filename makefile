CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
TARGET = gsr
SRC = gsr.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

PREFIX = $(HOME)/.local

install: $(TARGET)
	mkdir -p $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: clean install uninstall
