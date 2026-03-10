#include "sqlite_builtin.hpp"
#include "interpreter.hpp"
#include <sstream>
#include <string>

#if defined(USE_SQLITE)
#include <sqlite3.h>
#endif

#if defined(USE_SQLITE)
static std::string str(Value v) {
    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
    if (std::holds_alternative<double>(v)) return std::to_string((int)std::get<double>(v));
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? "1" : "0";
    return "";
}
#endif

void registerSqliteBuiltins(Interpreter* interp) {
#if defined(USE_SQLITE)
    interp->registerBuiltin("sqliteOpen", [](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.empty()) return false;
        std::string path = str(args[0]);
        if (path.empty()) return false;

        std::string resolved = i->getResolvedPath(path);

        if (i->getSqliteStmt()) {
            sqlite3_finalize((sqlite3_stmt*)i->getSqliteStmt());
            i->setSqliteStmt(nullptr);
        }
        if (i->getSqliteDb()) {
            sqlite3_close((sqlite3*)i->getSqliteDb());
            i->setSqliteDb(nullptr);
        }

        sqlite3* db = nullptr;
        if (sqlite3_open(resolved.c_str(), &db) != SQLITE_OK) {
            if (db) sqlite3_close(db);
            return false;
        }
        i->setSqliteDb(db);
        return true;
    });

    interp->registerBuiltin("sqliteExec", [](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
        sqlite3* db = (sqlite3*)i->getSqliteDb();
        if (!db) return false;

        if (i->getSqliteStmt()) {
            sqlite3_finalize((sqlite3_stmt*)i->getSqliteStmt());
            i->setSqliteStmt(nullptr);
        }

        char* err = nullptr;
        std::string sql = std::get<std::string>(args[0]);
        int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
        if (err) sqlite3_free(err);
        return rc == SQLITE_OK;
    });

    interp->registerBuiltin("sqliteQuery", [](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
        sqlite3* db = (sqlite3*)i->getSqliteDb();
        if (!db) return false;

        if (i->getSqliteStmt()) {
            sqlite3_finalize((sqlite3_stmt*)i->getSqliteStmt());
            i->setSqliteStmt(nullptr);
        }

        sqlite3_stmt* stmt = nullptr;
        std::string sql = std::get<std::string>(args[0]);
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            if (stmt) sqlite3_finalize(stmt);
            return false;
        }
        i->setSqliteStmt(stmt);
        return true;
    });

    interp->registerBuiltin("sqliteFetchRow", [](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        sqlite3_stmt* stmt = (sqlite3_stmt*)i->getSqliteStmt();
        if (!stmt) return std::string("");

        int rc = sqlite3_step(stmt);
        if (rc == SQLITE_ROW) {
            int n = sqlite3_column_count(stmt);
            std::ostringstream out;
            static const char colSep = '\x01';
            for (int c = 0; c < n; ++c) {
                if (c) out << colSep;
                const unsigned char* txt = sqlite3_column_text(stmt, c);
                out << (txt ? (const char*)txt : "");
            }
            return out.str();
        }

        sqlite3_finalize(stmt);
        i->setSqliteStmt(nullptr);
        return std::string("");
    });

    interp->registerBuiltin("sqliteFetchAll", [](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        auto rows = std::make_shared<MeltArray>();
        sqlite3_stmt* stmt = (sqlite3_stmt*)i->getSqliteStmt();
        if (!stmt) return rows;

        for (;;) {
            int rc = sqlite3_step(stmt);
            if (rc == SQLITE_ROW) {
                auto row = std::make_shared<MeltArray>();
                int n = sqlite3_column_count(stmt);
                for (int c = 0; c < n; ++c) {
                    const unsigned char* txt = sqlite3_column_text(stmt, c);
                    row->data.push_back(std::string(txt ? (const char*)txt : ""));
                }
                rows->data.push_back(row);
                continue;
            }
            break;
        }

        sqlite3_finalize(stmt);
        i->setSqliteStmt(nullptr);
        return rows;
    });

    interp->registerBuiltin("sqliteClose", [](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        if (i->getSqliteStmt()) {
            sqlite3_finalize((sqlite3_stmt*)i->getSqliteStmt());
            i->setSqliteStmt(nullptr);
        }
        if (i->getSqliteDb()) {
            sqlite3_close((sqlite3*)i->getSqliteDb());
            i->setSqliteDb(nullptr);
        }
        return false;
    });
#else
    auto sqliteUnavailable = [](Interpreter* i, std::vector<Value>) -> Value {
        (void)i;
        i->runtimeError("SQLite is not available: this build was compiled without SQLite. Rebuild with SQLite (e.g. cmake with sqlite3, or use a build that has USE_SQLITE).");
        return std::monostate{};
    };
    interp->registerBuiltin("sqliteOpen", sqliteUnavailable);
    interp->registerBuiltin("sqliteExec", sqliteUnavailable);
    interp->registerBuiltin("sqliteQuery", sqliteUnavailable);
    interp->registerBuiltin("sqliteFetchRow", sqliteUnavailable);
    interp->registerBuiltin("sqliteFetchAll", sqliteUnavailable);
    interp->registerBuiltin("sqliteClose", sqliteUnavailable);
#endif
}
