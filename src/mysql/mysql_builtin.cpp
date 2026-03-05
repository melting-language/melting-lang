#include "mysql_builtin.hpp"
#include "interpreter.hpp"
#include <cstring>
#include <sstream>
#include <stdexcept>

#ifdef USE_MYSQL
#include <mysql/mysql.h>
#endif

#if defined(USE_MYSQL)
static std::string str(Value v) {
    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
    if (std::holds_alternative<double>(v)) return std::to_string((int)std::get<double>(v));
    return "";
}
#endif

void registerMysqlBuiltins(Interpreter* interp) {
#if defined(USE_MYSQL)
    interp->registerBuiltin("mysqlConnect", [](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.size() < 4) return false;
        std::string host = str(args[0]);
        std::string user = str(args[1]);
        std::string pass = str(args[2]);
        std::string db   = str(args[3]);
        MYSQL* mysql = mysql_init(nullptr);
        if (!mysql) return false;
        const char* ph = host.empty() ? nullptr : host.c_str();
        const char* pu = user.empty() ? nullptr : user.c_str();
        const char* pp = pass.empty() ? nullptr : pass.c_str();
        const char* pd = db.empty() ? nullptr : db.c_str();
        if (!mysql_real_connect(mysql, ph, pu, pp, pd, 0, nullptr, 0)) {
            mysql_close(mysql);
            return false;
        }
        if (i->getMysqlConn()) {
            mysql_close((MYSQL*)i->getMysqlConn());
        }
        i->setMysqlConn(mysql);
        i->setMysqlRes(nullptr);
        return true;
    });
    interp->registerBuiltin("mysqlQuery", [](Interpreter* i, std::vector<Value> args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) return false;
        MYSQL* mysql = (MYSQL*)i->getMysqlConn();
        if (!mysql) return false;
        if (i->getMysqlRes()) {
            mysql_free_result((MYSQL_RES*)i->getMysqlRes());
            i->setMysqlRes(nullptr);
        }
        std::string sql = std::get<std::string>(args[0]);
        if (mysql_real_query(mysql, sql.c_str(), (unsigned long)sql.size()) != 0) return false;
        MYSQL_RES* res = mysql_store_result(mysql);
        i->setMysqlRes(res);
        return true;
    });
    interp->registerBuiltin("mysqlFetchRow", [](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        MYSQL_RES* res = (MYSQL_RES*)i->getMysqlRes();
        if (!res) return std::string("");
        MYSQL_ROW row = mysql_fetch_row(res);
        if (!row) return std::string("");
        unsigned int n = mysql_num_fields(res);
        std::ostringstream out;
        static const char colSep = '\x01';  // safe delimiter (body may contain tab)
        for (unsigned int c = 0; c < n; ++c) {
            if (c) out << colSep;
            out << (row[c] ? row[c] : "");
        }
        return out.str();
    });
    interp->registerBuiltin("mysqlFetchAll", [](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        auto rows = std::make_shared<MeltArray>();
        MYSQL_RES* res = (MYSQL_RES*)i->getMysqlRes();
        if (!res) return rows;
        unsigned int n = mysql_num_fields(res);
        MYSQL_ROW row = nullptr;
        while ((row = mysql_fetch_row(res)) != nullptr) {
            auto rowArr = std::make_shared<MeltArray>();
            for (unsigned int c = 0; c < n; ++c)
                rowArr->data.push_back(std::string(row[c] ? row[c] : ""));
            rows->data.push_back(rowArr);
        }
        mysql_free_result(res);
        i->setMysqlRes(nullptr);
        return rows;
    });
    interp->registerBuiltin("mysqlClose", [](Interpreter* i, std::vector<Value> args) -> Value {
        (void)args;
        if (i->getMysqlRes()) {
            mysql_free_result((MYSQL_RES*)i->getMysqlRes());
            i->setMysqlRes(nullptr);
        }
        if (i->getMysqlConn()) {
            mysql_close((MYSQL*)i->getMysqlConn());
            i->setMysqlConn(nullptr);
        }
        return false;
    });
#else
    (void)interp;
#endif
}
