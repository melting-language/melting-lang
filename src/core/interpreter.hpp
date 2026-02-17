#pragma once

#include "ast.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <set>
#include <string>
#include <variant>
#include <vector>

struct MeltObject;  // forward

struct MeltMethod {
    std::vector<std::string> params;
    std::unique_ptr<BlockStmt> body;
};

struct MeltClass {
    std::string name;
    std::unordered_map<std::string, MeltMethod> methods;
};

struct BoundMethod {
    std::shared_ptr<MeltObject> receiver;
    std::string methodName;
};

struct NativeFunc {
    size_t index = 0;  // index into Interpreter::nativeFunctions_
};

struct MeltArray;  // forward
struct MeltVec;   // forward

using Value = std::variant<std::monostate, double, std::string, bool,
    std::shared_ptr<MeltClass>, std::shared_ptr<MeltObject>, std::shared_ptr<MeltArray>,
    std::shared_ptr<MeltVec>,
    BoundMethod, NativeFunc>;

struct MeltArray {
    std::vector<Value> data;
};

// Math vector: dim 2 (x,y) or 3 (x,y,z). z is 0 when dim==2.
struct MeltVec {
    double x = 0, y = 0, z = 0;
    int dim = 2;  // 2 or 3
};

struct MeltObject {
    std::shared_ptr<MeltClass> klass;
    std::unordered_map<std::string, Value> fields;
};

class Interpreter {
public:
    void interpret(const std::vector<std::unique_ptr<Stmt>>& statements,
                   const std::string& currentFilePath = "");

    // HTTP server: called from native built-ins
    void setRequestData(const std::string& path, const std::string& method, const std::string& body, const std::string& headers = "");
    void callHandler();
    std::string getResponseBody() const { return responseBody_; }
    int getResponseStatus() const { return responseStatus_; }
    std::string getResponseContentType() const { return responseContentType_; }
    const std::vector<std::pair<std::string, std::string>>& getResponseHeaders() const { return responseHeaders_; }
    void setResponseBodyInternal(const std::string& s) { responseBody_ = s; }
    void setResponseStatusInternal(int n) { responseStatus_ = n; }
    void setResponseContentTypeInternal(const std::string& s) { responseContentType_ = s; }
    const std::string& getCurrentRequestPath() const { return currentRequestPath_; }
    const std::string& getCurrentRequestMethod() const { return currentRequestMethod_; }
    const std::string& getCurrentRequestBody() const { return currentRequestBody_; }
    const std::string& getCurrentRequestHeaders() const { return currentRequestHeaders_; }
    void addResponseHeader(const std::string& name, const std::string& value) { responseHeaders_.push_back({name, value}); }

    // Stdio / MCP transport: request/response strings and handler
    void setMcpRequest(const std::string& s);
    std::string getMcpRequest() const;
    void setMcpResponse(const std::string& s);
    std::string getMcpResponse() const;
    void setMcpHandlerClassName(const std::string& s);
    void callMcpHandler();

    // Serve a file from public/ (e.g. /js/x.js, /css/x.css, /images/x.png). Returns true if served.
    bool servePublic(const std::string& path);
    // Resolve path relative to script directory (for readFile in scripts).
    std::string getResolvedPath(const std::string& path) const;
    // Module path: add a directory to search for imports (used by addModulePath built-in).
    void addModulePath(const std::string& dir);
    // Config: get value from melt.config (used by getConfig built-in).
    std::string getConfig(const std::string& key) const;
    // Set directory containing the melt binary (for melt.ini and extension loading).
    void setBinDir(const std::string& binDir);

    using NativeFn = std::function<Value(Interpreter*, std::vector<Value>)>;
    // Register a native built-in (for extensions like MySQL)
    void registerBuiltin(const std::string& name, NativeFn fn);
    // MySQL storage (opaque; used by mysql_builtin when USE_MYSQL is defined)
    void* getMysqlConn() const { return mysql_conn_; }
    void setMysqlConn(void* p) { mysql_conn_ = p; }
    void* getMysqlRes() const { return mysql_res_; }
    void setMysqlRes(void* p) { mysql_res_ = p; }

    std::shared_ptr<MeltClass> getJsonObjectClass();

private:
    std::unordered_map<std::string, Value> variables_;
    Value this_;  // current 'this' for method execution
    std::string currentDir_;  // directory of the file being executed (for import resolution)
    std::set<std::string> importedPaths_;  // avoid circular/repeated imports
    std::string binDir_;  // directory containing the melt binary (for melt.ini, extensions)
    std::vector<std::string> modulePath_;  // additional directories to search for imports (from melt.config or addModulePath)
    std::unordered_map<std::string, std::string> config_;  // key-value from melt.config / melt.ini (last value per key)

    std::vector<NativeFn> nativeFunctions_;
    void registerBuiltins();

    void* mysql_conn_ = nullptr;
    void* mysql_res_ = nullptr;

    std::shared_ptr<MeltClass> jsonObjectClass_;

    // HTTP request/response state (for server)
    std::string currentRequestPath_;
    std::string currentRequestMethod_;
    std::string currentRequestBody_;
    std::string currentRequestHeaders_;
    std::string responseBody_;
    int responseStatus_ = 200;
    std::string responseContentType_ = "text/html; charset=utf-8";
    std::vector<std::pair<std::string, std::string>> responseHeaders_;
    std::string handlerClassName_;

    // MCP / stdio transport state
    std::string mcpRequest_;
    std::string mcpResponse_;
    std::string mcpHandlerClassName_;

    // GUI render: image buffer (RGB, row-major)
    int imageWidth_ = 0;
    int imageHeight_ = 0;
    std::vector<uint8_t> imageData_;

    void loadGlobalConfig();  // load binDir_/melt.ini, clear then set config_ and modulePath_ (paths relative to binDir_)
    void loadConfig(const std::string& entryDir);  // load melt.config from entryDir, merge into config_ and modulePath_
    std::string resolvePath(const std::string& path);
    std::string resolveImportPath(const std::string& path);  // resolve import path (currentDir + modulePath), add .melt if needed
    std::string readFile(const std::string& path);
    std::string renderViewTemplate(const std::string& path, std::shared_ptr<MeltObject> obj);
    bool saveImagePpm(const std::string& path) const;

    void execute(Stmt& stmt);
    Value evaluate(const Expr& expr);

    void executePrint(const PrintStmt& stmt);
    void executeExprStmt(const ExprStmt& stmt);
    void executeImport(const ImportStmt& stmt);
    void executeLet(const LetStmt& stmt);
    void executeAssign(const AssignStmt& stmt);
    void executeClass(ClassDeclStmt& stmt);
    void executeSetProperty(const SetPropertyStmt& stmt);
    void executeSetIndex(const SetIndexStmt& stmt);
    void executeBlock(const BlockStmt& stmt);
    void executeIf(const IfStmt& stmt);
    void executeWhile(const WhileStmt& stmt);
    void executeReturn(const ReturnStmt& stmt);
    void executeTryCatch(const TryCatchStmt& stmt);
    void executeThrow(const ThrowStmt& stmt);

    Value evaluateNumber(const NumberExpr& expr);
    Value evaluateString(const StringExpr& expr);
    Value evaluateBool(const BoolExpr& expr);
    Value evaluateVar(const VarExpr& expr);
    Value evaluateThis(const ThisExpr& expr);
    Value evaluateGet(const GetExpr& expr);
    Value evaluateCall(const CallExpr& expr);
    Value evaluateArray(const ArrayExpr& expr);
    Value evaluateIndex(const IndexExpr& expr);
    Value evaluateUnary(const UnaryExpr& expr);
    Value evaluateBinary(const BinaryExpr& expr);

    bool isTruthy(const Value& v);
    void printValue(const Value& v);
    Value getField(std::shared_ptr<MeltObject> obj, const std::string& name);
    void setField(MeltObject& obj, const std::string& name, Value v);
};
