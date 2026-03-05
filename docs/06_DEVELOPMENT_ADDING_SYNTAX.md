# Development: How to Add a Simple Syntax Feature

This guide walks through adding a new syntax feature to Melt **step by step**. The pipeline is:

**Source code** → **Lexer** (tokens) → **Parser** (AST) → **Interpreter** (execution)

All language-core code lives under **`src/core/`**: `lexer.cpp`/`lexer.hpp`, `parser.cpp`/`parser.hpp`, `ast.hpp`, `interpreter.cpp`/`interpreter.hpp`.

---

## Example: Add a `say expr;` statement

We will add a new statement `say expr;` that prints the value of `expr` to stdout (like `print`, but with a different keyword). You will touch **four places**: lexer, AST, parser, interpreter.

---

### Step 1: Lexer — add a token for the new keyword

The lexer turns source text into a list of **tokens**. Keywords are recognized in `identifier()` and returned as special token types.

**1.1** Open **`src/core/lexer.hpp`**. In the `TokenType` enum, add a new value (e.g. next to `Print`):

```cpp
Print, Let, If, Else, While, ...
Say,   // add this
```

**1.2** Open **`src/core/lexer.cpp`**. In `Lexer::identifier()`, after the existing keyword checks (e.g. `if (value == "print") ...`), add:

```cpp
if (value == "say") return Token(TokenType::Say, value, start);
```

Now the source `say "hello";` will produce the tokens: `Say`, `String("hello")`, `Semicolon`.

---

### Step 2: AST — define a node for the new statement

The **Abstract Syntax Tree (AST)** is defined in **`src/core/ast.hpp`**. Each statement type has a struct that holds its parts.

**2.1** Add a new statement type. For `say expr;`, we need an expression and nothing else. Add this near the other statement structs (e.g. after `PrintStmt`):

```cpp
struct SayStmt : Stmt {
    std::unique_ptr<Expr> expr;
    SayStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
};
```

No other AST changes are needed for this example.

---

### Step 3: Parser — parse the new statement and build the AST

The parser consumes tokens and builds AST nodes. Statement parsing is in **`src/core/parser.cpp`**.

**3.1** In `Parser::statement()`, add a branch so the new keyword is recognized (order can matter if one keyword is a prefix of another):

```cpp
if (match(TokenType::Print)) return printStatement();
if (match(TokenType::Say))   return sayStatement();   // add this
if (match(TokenType::Let)) return letStatement();
// ...
```

**3.2** Add the parsing function. Implement it like `printStatement()`: expect an expression, then a semicolon, and return a `SayStmt`:

```cpp
std::unique_ptr<Stmt> Parser::sayStatement() {
    auto expr = expression();
    match(TokenType::Semicolon);
    return std::make_unique<SayStmt>(std::move(expr));
}
```

**3.3** Declare the new function in **`src/core/parser.hpp`** in the `private` section with the other statement parsers:

```cpp
std::unique_ptr<Stmt> sayStatement();
```

---

### Step 4: Interpreter — execute the new statement

The interpreter walks the AST and runs each statement. Changes are in **`src/core/interpreter.cpp`** and **`src/core/interpreter.hpp`**.

**4.1** In **`interpreter.hpp`**, declare an execution helper for the new statement (with the other `execute*` declarations):

```cpp
void executeSay(const SayStmt& stmt);
```

**4.2** In **`interpreter.cpp`**, in `Interpreter::execute(Stmt& stmt)`, add a branch that dispatches to your new function:

```cpp
if (auto p = dynamic_cast<SayStmt*>(&stmt)) { executeSay(*p); return; }
```

**4.3** Implement the behavior. For example, make `say` behave like `print` (evaluate the expression, print it, then a newline):

```cpp
void Interpreter::executeSay(const SayStmt& stmt) {
    Value v = evaluate(*stmt.expr);
    printValue(v);
    std::cout << "\n";
}
```

---

### Step 5: Build and test

From the project root:

```bash
cmake --build build
echo 'say "Hello from say!"; say 1 + 2;' > test_say.melt
./build/melt test_say.melt
```

You should see:

```
Hello from say!
3
```

---

## Checklist summary

| Step | File(s) | What to do |
|------|---------|------------|
| 1 | `lexer.hpp`, `lexer.cpp` | Add `TokenType::X`, recognize keyword in `identifier()` |
| 2 | `ast.hpp` | Add a struct for the new statement (or expression) |
| 3 | `parser.hpp`, `parser.cpp` | In `statement()` (or expression chain) call new parser; implement parser; return new AST node |
| 4 | `interpreter.hpp`, `interpreter.cpp` | In `execute()` (or `evaluate()`) handle new AST type; implement `executeX()` (or `evaluateX()`) |

---

## Adding a new expression (e.g. unary operator)

To add a **new expression** (e.g. a unary operator like `#expr`):

1. **Lexer:** Add a token type and emit it for the new symbol (e.g. in the main `tokenize()` loop for `#`).
2. **AST:** Add a new expression struct (e.g. `UnaryExpr` already exists; or add something like `HashExpr` with one child).
3. **Parser:** In the expression chain, add a branch (often in `primary()` or a new level) that matches the token and builds the new expression node.
4. **Interpreter:** In `evaluate()`, add a branch for the new expression type and implement `evaluateX()`.

---

## Adding a statement with a block (e.g. `repeat n { ... }`)

To add a **statement that has a block** (e.g. `repeat 5 { print "hi"; }`):

1. **Lexer:** Add token type for the keyword (e.g. `Repeat`) and recognize it in `identifier()`.
2. **AST:** Add a struct with an expression (e.g. count) and a body, e.g. `std::unique_ptr<Expr> count; std::unique_ptr<BlockStmt> body;`.
3. **Parser:** In `statement()`, match the keyword; parse an expression (e.g. count); require `{`; parse statements until `}` (reuse block logic); return the new statement node.
4. **Interpreter:** In `execute()`, handle the new type: evaluate the count, then in a loop execute the block that many times (with proper scope if needed).

---

## Where things live

| Component | Path | Role |
|-----------|------|------|
| Lexer | `src/core/lexer.cpp`, `lexer.hpp` | Source → tokens |
| AST | `src/core/ast.hpp` | Node definitions for statements and expressions |
| Parser | `src/core/parser.cpp`, `parser.hpp` | Tokens → AST |
| Interpreter | `src/core/interpreter.cpp`, `interpreter.hpp` | AST → execution |

Optional modules (HTTP, MySQL, GUI) live in **`src/http/`**, **`src/mysql/`**, **`src/gui/`** and are wired from the interpreter; new **syntax** is implemented only in **`src/core/`**.

---

## See also

- [Language Reference](02_LANGUAGE_REFERENCE.md) — existing syntax and semantics  
- [Getting Started](01_GETTING_STARTED.md) — build and run  
- **`src/README.md`** — source tree layout
