cmake_minimum_required(VERSION 3.16)

project(melt VERSION 1.3.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)


if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(LINUX_OS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
    link_libraries(dl)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    add_compile_definitions(FREEBSD_OS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_compile_definitions(MACOS_OS)
    # macOS uses -export_dynamic, though often symbols are exported by default
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-export_dynamic")
    # No need to link 'dl'; it's part of libSystem on macOS
endif()


# Melt specific configurations
set(USE_GUI ON)
set(USE_SQLITE ON)
set(USE_QR ON)
set(USE_MYSQL OFF)

find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(MYSQL mysqlclient)
pkg_check_modules(QRENCODE REQUIRED IMPORTED_TARGET libqrencode)
pkg_check_modules(LIBZIP libzip)


configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

message(STATUS "SDL2 Libs: ${SDL2_LIBRARIES}")
message(STATUS "SDL2 Includes: ${SDL2_INCLUDE_DIRS}")
message(STATUS "MySQL found: ${MYSQL_FOUND}")
message(STATUS "libzip found: ${LIBZIP_FOUND}")
message(STATUS "Built-in HTTP server: enabled (no separate process; listen/setHandler in interpreter)")

set(SOURCES
    src/main.cpp
    src/core/interpreter.cpp
    src/core/lexer.cpp
    src/core/module_loader.cpp
    src/core/parser.cpp
    src/gui/gui_window.cpp
    src/http/http_server.cpp
    src/mysql/mysql_builtin.cpp
    src/qr/qr_builtin.cpp
    src/sqlite/sqlite_builtin.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

set(MELT_INCLUDE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/http
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mysql
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sqlite
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qr
    ${SQLITE3_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
    ${SDL2_INCLUDE_DIRS}
)
target_include_directories(${PROJECT_NAME} PRIVATE ${MELT_INCLUDE_HEADERS})

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    pthread
    PkgConfig::QRENCODE
    ${SDL2_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${CMAKE_DL_LIBS}
)

# =================================================================================
#  Optional enable section
# =================================================================================

# MySQL: optional. Enable with -DUSE_MYSQL=ON if mysqlclient include path works.
if(MYSQL_FOUND AND USE_MYSQL)
    message(STATUS "Building with MySQL support")
    target_include_directories(${PROJECT_NAME} PRIVATE ${MYSQL_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${MYSQL_LDFLAGS})
else()
    if(MYSQL_FOUND)
        message(STATUS "MySQL found but disabled (set -DUSE_MYSQL=ON to enable)")
    else()
        message(STATUS "MySQL not found; building without MySQL support")
    endif()
endif()


add_subdirectory(extensions)

# Install (e.g. cmake --install build --prefix /usr/local)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
