#pragma once

#include "config.h"

class Interpreter;

// Call from registerBuiltins() to add mysqlConnect, mysqlQuery, mysqlFetchRow, mysqlFetchAll, mysqlClose.
// No-op if USE_MYSQL is not defined.
void registerMysqlBuiltins(Interpreter* interp);
