CXX = g++

# Include all source dirs so that #include "lexer.hpp" etc. resolve from subdirs
CXXFLAGS = -std=c++17 -Wall \
		   -I src \
		   -I src/core \
		   -I src/http \
		   -I src/mysql \
		   -I src/sqlite \
		   -I src/gui \
		   -I src/qr

SRC = src/main.cpp \
	  src/core/lexer.cpp \
	  src/core/parser.cpp \
	  src/core/interpreter.cpp \
	  src/core/module_loader.cpp \
	  src/http/http_server.cpp \
	  src/mysql/mysql_builtin.cpp \
	  src/sqlite/sqlite_builtin.cpp \
	  src/qr/qr_builtin.cpp

SRC_EMBEDDED = src/main.cpp \
			   src/core/lexer.cpp \
			   src/core/parser.cpp \
			   src/core/interpreter.cpp \
			   src/core/module_loader.cpp

BINDIR = bin

TARGET = $(BINDIR)/melt
TARGET_EMBEDDED = $(BINDIR)/melt-embedded
# Linux needs -ldl for dlopen; macOS has it in libSystem
UNAME_S := $(shell uname -s 2>/dev/null || echo unknown)
ifeq ($(UNAME_S),Linux)
LDFLAGS += -ldl -rdynamic
endif

# SQLite: default build includes it so admin_panel_sqlite and examples work out of the box
SQLITE_CFLAGS ?= $(shell pkg-config --cflags sqlite3 2>/dev/null)
SQLITE_LIBS   ?= $(shell pkg-config --libs sqlite3 2>/dev/null)
ifeq ($(SQLITE_LIBS),)
SQLITE_LIBS := -lsqlite3
endif

# QR code (optional): make with-qr to enable qrGenerate builtin; requires libqrencode
QR_CFLAGS ?= $(shell pkg-config --cflags libqrencode 2>/dev/null)
QR_LIBS   ?= $(shell pkg-config --libs libqrencode 2>/dev/null)
ifeq ($(QR_LIBS),)
QR_LIBS := -lqrencode
endif

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -DUSE_SQLITE $(SQLITE_CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS) $(SQLITE_LIBS)

# Build with MySQL support. Requires libmysqlclient-dev (Linux) or mysql (Homebrew).
# Adjust MYSQL_CFLAGS / MYSQL_LIBS if needed (e.g. -I/usr/local/mysql/include -L/usr/local/mysql/lib).
MYSQL_CFLAGS ?= $(shell pkg-config --cflags mysqlclient 2>/dev/null)
MYSQL_LIBS   ?= $(shell pkg-config --libs mysqlclient 2>/dev/null)
with-mysql: CXXFLAGS += -DUSE_MYSQL $(MYSQL_CFLAGS)
with-mysql: LDFLAGS += $(MYSQL_LIBS)
with-mysql: $(SRC)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Build with SQLite support. Requires sqlite3 development library.
SQLITE_CFLAGS ?= $(shell pkg-config --cflags sqlite3 2>/dev/null)
SQLITE_LIBS   ?= $(shell pkg-config --libs sqlite3 2>/dev/null)
ifeq ($(SQLITE_LIBS),)
SQLITE_LIBS := -lsqlite3
endif
with-sqlite: CXXFLAGS += -DUSE_SQLITE $(SQLITE_CFLAGS)
with-sqlite: LDFLAGS += $(SQLITE_LIBS)
with-sqlite: $(SRC)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Build with QR code support (qrGenerate). Requires libqrencode. macOS: brew install qrencode
with-qr: CXXFLAGS += -DUSE_QR $(QR_CFLAGS)
with-qr: LDFLAGS += $(QR_LIBS)
with-qr: $(SRC)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -DUSE_SQLITE -DUSE_QR $(SQLITE_CFLAGS) $(QR_CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS) $(SQLITE_LIBS) $(QR_LIBS)

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
	rm -f $(BINDIR)/modules/image_optimize.so $(BINDIR)/modules/image_optimize.dylib $(BINDIR)/modules/image_optimize.dll
	rm -f $(BINDIR)/modules/os.so $(BINDIR)/modules/os.dylib $(BINDIR)/modules/os.dll
	rm -f $(BINDIR)/modules/headless_browser.so $(BINDIR)/modules/headless_browser.dylib $(BINDIR)/modules/headless_browser.dll
	rm -f $(BINDIR)/modules/ffmpeg.so $(BINDIR)/modules/ffmpeg.dylib $(BINDIR)/modules/ffmpeg.dll

# Example loadable extension: build into bin/modules/ (enable with extension = example in melt.ini).
EXT_MODULES = $(BINDIR)/modules
EXT_SRC = extensions/example/example.cpp
IMG_OPT_SRC = extensions/image_optimize/image_optimize.cpp
OS_EXT_SRC = extensions/os/os.cpp
HEADLESS_SRC = extensions/headless_browser/headless_browser.cpp
FFMPEG_SRC = extensions/ffmpeg/ffmpeg.cpp
ifeq ($(UNAME_S),Darwin)
EXT_SUFFIX = .dylib
else ifeq ($(UNAME_S),Linux)
EXT_SUFFIX = .so
else
EXT_SUFFIX = .dll
endif
EXT_TARGET = $(EXT_MODULES)/example$(EXT_SUFFIX)
IMG_OPT_TARGET = $(EXT_MODULES)/image_optimize$(EXT_SUFFIX)
OS_EXT_TARGET = $(EXT_MODULES)/os$(EXT_SUFFIX)
HEADLESS_TARGET = $(EXT_MODULES)/headless_browser$(EXT_SUFFIX)
FFMPEG_TARGET = $(EXT_MODULES)/ffmpeg$(EXT_SUFFIX)

modules: $(EXT_TARGET) $(IMG_OPT_TARGET) $(OS_EXT_TARGET) $(HEADLESS_TARGET) $(FFMPEG_TARGET)

$(EXT_TARGET): $(EXT_SRC)
	@mkdir -p $(EXT_MODULES)
ifeq ($(UNAME_S),Darwin)
	$(CXX) $(CXXFLAGS) -fPIC -shared -undefined dynamic_lookup -o $(EXT_TARGET) $(EXT_SRC)
else
	$(CXX) $(CXXFLAGS) -fPIC -shared -o $(EXT_TARGET) $(EXT_SRC)
endif

$(IMG_OPT_TARGET): $(IMG_OPT_SRC)
	@mkdir -p $(EXT_MODULES)
ifeq ($(UNAME_S),Darwin)
	$(CXX) $(CXXFLAGS) -fPIC -shared -undefined dynamic_lookup -o $(IMG_OPT_TARGET) $(IMG_OPT_SRC)
else
	$(CXX) $(CXXFLAGS) -fPIC -shared -o $(IMG_OPT_TARGET) $(IMG_OPT_SRC)
endif

$(OS_EXT_TARGET): $(OS_EXT_SRC)
	@mkdir -p $(EXT_MODULES)
ifeq ($(UNAME_S),Darwin)
	$(CXX) $(CXXFLAGS) -fPIC -shared -undefined dynamic_lookup -o $(OS_EXT_TARGET) $(OS_EXT_SRC)
else
	$(CXX) $(CXXFLAGS) -fPIC -shared -o $(OS_EXT_TARGET) $(OS_EXT_SRC)
endif

$(HEADLESS_TARGET): $(HEADLESS_SRC)
	@mkdir -p $(EXT_MODULES)
ifeq ($(UNAME_S),Darwin)
	$(CXX) $(CXXFLAGS) -fPIC -shared -undefined dynamic_lookup -o $(HEADLESS_TARGET) $(HEADLESS_SRC)
else
	$(CXX) $(CXXFLAGS) -fPIC -shared -o $(HEADLESS_TARGET) $(HEADLESS_SRC)
endif

$(FFMPEG_TARGET): $(FFMPEG_SRC)
	@mkdir -p $(EXT_MODULES)
ifeq ($(UNAME_S),Darwin)
	$(CXX) $(CXXFLAGS) -fPIC -shared -undefined dynamic_lookup -o $(FFMPEG_TARGET) $(FFMPEG_SRC)
else
	$(CXX) $(CXXFLAGS) -fPIC -shared -o $(FFMPEG_TARGET) $(FFMPEG_SRC)
endif

# CI build: same as 'all' but ensure no MySQL/SDL linkage (avoids dyld load errors on runners).
ci: MYSQL_CFLAGS :=
ci: MYSQL_LIBS :=
ci: $(TARGET)

.PHONY: all run clean with-mysql with-sqlite with-qr with-gui install uninstall modules ci embedded
