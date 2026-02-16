CXX = g++
CXXFLAGS = -std=c++17 -Wall -I src
SRC = src/main.cpp src/lexer.cpp src/parser.cpp src/interpreter.cpp src/http_server.cpp src/mysql_builtin.cpp
BINDIR = bin
TARGET = $(BINDIR)/melt

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

# Build with MySQL support. Requires libmysqlclient-dev (Linux) or mysql (Homebrew).
# Adjust MYSQL_CFLAGS / MYSQL_LIBS if needed (e.g. -I/usr/local/mysql/include -L/usr/local/mysql/lib).
MYSQL_CFLAGS ?= $(shell pkg-config --cflags mysqlclient 2>/dev/null)
MYSQL_LIBS   ?= $(shell pkg-config --libs mysqlclient 2>/dev/null)
with-mysql: CXXFLAGS += -DUSE_MYSQL $(MYSQL_CFLAGS)
with-mysql: LDFLAGS += $(MYSQL_LIBS)
with-mysql: $(SRC)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET) example.melt

# Install melt binary. Default: /usr/local (use PREFIX=/usr for system-wide, or PREFIX=$(HOME)/.local for user).
# Examples: make install   or   make install PREFIX=$(HOME)/.local   or   sudo make install
PREFIX ?= /usr/local
install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/melt

clean:
	rm -f $(TARGET)

.PHONY: all run clean with-mysql install uninstall
