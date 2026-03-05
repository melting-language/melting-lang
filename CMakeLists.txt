cmake_minimum_required(VERSION 3.31)

project(melt VERSION 1.3.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_BUILD_TYPE Debug)
add_compile_options(-Wall -Wextra -Wpedantic)


if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(LINUX_OS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
    link_libraries(dl)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    add_compile_definitions(FREEBSD_OS)
endif()

# Melt specific configurations
set(USE_GUI ON)
set(USE_MYSQL ON)
set(USE_SQLITE ON)


configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)


find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
pkg_check_modules(MYSQL mysqlclient)

message(STATUS "SDL2 Libs: ${SDL2_LIBRARIES}")
message(STATUS "SDL2 Includes: ${SDL2_INCLUDE_DIRS}")
message(STATUS "MySQL Libraries: ${MYSQL_LIBRARIES}")
message(STATUS "MySQL Library Dir: ${MYSQL_LIBRARY_DIRS}")
message(STATUS "MySQL LDFLAGS: ${MYSQL_LDFLAGS}")


set(SOURCES
    src/main.cpp
    src/core/lexer.cpp
    src/core/parser.cpp
    src/core/interpreter.cpp
    src/core/module_loader.cpp
    src/http/http_server.cpp
    src/mysql/mysql_builtin.cpp
    src/sqlite/sqlite_builtin.cpp
    src/qr/qr_builtin.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE 
    src
    src/core
    src/http
    src/mysql
    src/sqlite
    src/gui
    src/qr
    ${SQLITE3_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${MYSQL_INCLUDE_DIRS}
)


target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    pthread
    ${MYSQL_LDFLAGS}
    ${SDL2_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    ${CMAKE_DL_LIBS}
)
