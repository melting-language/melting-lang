#pragma once

class Interpreter;

// Call from registerBuiltins() to add:
// sqliteOpen, sqliteExec, sqliteQuery, sqliteFetchRow, sqliteFetchAll, sqliteClose.
// No-op if USE_SQLITE is not defined.
void registerSqliteBuiltins(Interpreter* interp);
