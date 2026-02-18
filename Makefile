CXX = g++
# Include all source dirs so that #include "lexer.hpp" etc. resolve from subdirs
CXXFLAGS = -std=c++17 -Wall -I src -I src/core -I src/http -I src/mysql -I src/gui
SRC = src/main.cpp src/core/lexer.cpp src/core/parser.cpp src/core/interpreter.cpp src/core/module_loader.cpp src/http/http_server.cpp src/mysql/mysql_builtin.cpp
SRC_EMBEDDED = src/main.cpp src/core/lexer.cpp src/core/parser.cpp src/core/interpreter.cpp src/core/module_loader.cpp
BINDIR = bin
TARGET = $(BINDIR)/melt
TARGET_EMBEDDED = $(BINDIR)/melt-embedded
# Linux needs -ldl for dlopen; macOS has it in libSystem
UNAME_S := $(shell uname -s 2>/dev/null || echo unknown)
ifeq ($(UNAME_S),Linux)
LDFLAGS += -ldl -rdynamic
endif

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

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

# Embedded build: smaller binary without HTTP, MySQL, or GUI. Output: bin/melt-embedded
embedded: CXXFLAGS := -std=c++17 -Wall -DMELT_EMBEDDED -I src -I src/core
embedded: $(SRC_EMBEDDED)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET_EMBEDDED) $(SRC_EMBEDDED) $(LDFLAGS)

# Install melt binary. Default: /usr/local (use PREFIX=/usr for system-wide, or PREFIX=$(HOME)/.local for user).
# Examples: make install   or   make install PREFIX=$(HOME)/.local   or   sudo make install
PREFIX ?= /usr/local
install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/melt

clean:
	rm -f $(TARGET) $(TARGET_EMBEDDED)
	rm -f $(BINDIR)/modules/example.so $(BINDIR)/modules/example.dylib $(BINDIR)/modules/example.dll

# Example loadable extension: build into bin/modules/ (enable with extension = example in melt.ini).
EXT_MODULES = $(BINDIR)/modules
EXT_SRC = extensions/example/example.cpp
ifeq ($(UNAME_S),Darwin)
EXT_SUFFIX = .dylib
else ifeq ($(UNAME_S),Linux)
EXT_SUFFIX = .so
else
EXT_SUFFIX = .dll
endif
EXT_TARGET = $(EXT_MODULES)/example$(EXT_SUFFIX)

modules: $(EXT_TARGET)

$(EXT_TARGET): $(EXT_SRC)
	@mkdir -p $(EXT_MODULES)
ifeq ($(UNAME_S),Darwin)
	$(CXX) $(CXXFLAGS) -fPIC -shared -undefined dynamic_lookup -o $(EXT_TARGET) $(EXT_SRC)
else
	$(CXX) $(CXXFLAGS) -fPIC -shared -o $(EXT_TARGET) $(EXT_SRC)
endif

# CI build: same as 'all' but ensure no MySQL/SDL linkage (avoids dyld load errors on runners).
ci: MYSQL_CFLAGS :=
ci: MYSQL_LIBS :=
ci: $(TARGET)

.PHONY: all run clean with-mysql with-gui install uninstall modules ci embedded
