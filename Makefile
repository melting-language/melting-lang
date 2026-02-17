CXX = g++
# Include all source dirs so that #include "lexer.hpp" etc. resolve from subdirs
CXXFLAGS = -std=c++17 -Wall -I src -I src/core -I src/http -I src/mysql -I src/gui
SRC = src/main.cpp src/core/lexer.cpp src/core/parser.cpp src/core/interpreter.cpp src/http/http_server.cpp src/mysql/mysql_builtin.cpp
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

# Build with GUI preview window (imagePreview). Requires SDL2. Linux: libsdl2-dev; macOS: brew install sdl2.
SDL2_CFLAGS ?= $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL2_LIBS   ?= $(shell pkg-config --libs sdl2 2>/dev/null)
# macOS Homebrew fallback when pkg-config does not find SDL2
ifeq ($(SDL2_CFLAGS),)
SDL2_CFLAGS := -I/opt/homebrew/opt/sdl2/include
SDL2_LIBS   := -L/opt/homebrew/opt/sdl2/lib -lSDL2
endif
SRC_GUI = $(SRC) src/gui/gui_window.cpp
with-gui: CXXFLAGS += -DUSE_GUI $(SDL2_CFLAGS)
with-gui: LDFLAGS += $(SDL2_LIBS)
with-gui: $(SRC_GUI)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC_GUI) $(LDFLAGS)

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

.PHONY: all run clean with-mysql with-gui install uninstall
