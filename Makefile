# Thin wrapper: build and install use CMake only.
# Run: make (build) | make install [PREFIX=/usr/local] | make clean

BUILD_DIR ?= build
CMAKE_ARGS ?= -DUSE_MYSQL=OFF

all:
	cmake -B $(BUILD_DIR) $(CMAKE_ARGS)
	cmake --build $(BUILD_DIR)

install: all
	DESTDIR="$(DESTDIR)" cmake --install $(BUILD_DIR) --prefix $(or $(PREFIX),/usr/local)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all install clean
