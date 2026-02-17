#pragma once

class Interpreter;

// Call from registerBuiltins() to add mysqlConnect, mysqlQuery, mysqlFetchRow, mysqlClose.
// No-op if USE_MYSQL is not defined.
void registerMysqlBuiltins(Interpreter* interp);
