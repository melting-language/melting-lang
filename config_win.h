#ifndef CONFIG_H
#define CONFIG_H

/* Makefile.win minimal build: no GUI, MySQL, SQLite, QR (no extra deps) */
#undef USE_GUI
#undef USE_MYSQL
#undef USE_SQLITE
#undef USE_QR

#define PROJECT_VERSION_MAJOR 1
#define PROJECT_VERSION_MINOR 3
#define PROJECT_VERSION_PATCH 2

#endif
