# Tutorial: How to Implement New Syntax in Melt

This tutorial walks you through adding a new syntax feature to the Melt language **from start to finish**. You will add a `say expr;` statement that prints an expression to stdout (like `print`, but with a different keyword). By the end, you will know exactly which files to edit and in what order.

**Time:** about 15–20 minutes.  
**You need:** the Melt source tree and a successful build (`cmake -B build && cmake --build build`).

---

## How Melt runs your code

Melt turns source code into running program in four stages:

```
Source (text)  →  Lexer (tokens)  →  Parser (AST)  →  Interpreter (run)
```

- **Lexer** — Reads characters and produces **tokens** (keywords, identifiers, numbers, strings, punctuation).
- **Parser** — Consumes tokens and builds an **AST** (Abstract Syntax Tree): a tree of statement and expression nodes.
- **Interpreter** — Walks the AST and **executes** each node (evaluate expressions, run statements).

To add new syntax, you add a **token**, an **AST node**, **parsing** for it, and **execution** for it. We do that in four steps.

---

## What we’re adding

We’ll add a **statement**:

```melt
say "Hello!";
say 1 + 2;
```

Behavior: evaluate the expression and print it to stdout with a newline (same as `print`). The goal is to learn the pipeline; the behavior is intentionally simple.

---

## Step 1: Lexer — recognize the keyword

The lexer turns `say` in the source into a token. All keyword handling is in **`src/core/lexer.cpp`** inside `identifier()`.

### 1.1 Add the token type

**File:** `src/core/lexer.hpp`

In the `TokenType` enum, add `Say` next to `Print`:

```cpp
Print, Let, If, Else, While, ...
Say,   // add this
```

### 1.2 Emit the token for the keyword

**File:** `src/core/lexer.cpp`

In `Lexer::identifier()`, after the line that handles `"print"`, add:

```cpp
if (value == "print") return Token(TokenType::Print, value, start);
if (value == "say")   return Token(TokenType::Say, value, start);   // add this
if (value == "let") return Token(TokenType::Let, value, start);
```

Now the source `say "hello";` produces the token sequence: `Say`, `String("hello")`, `Semicolon`.

---

## Step 2: AST — define the statement node

The AST is the in-memory representation of the program. Each kind of statement has a struct in **`src/core/ast.hpp`**.

### 2.1 Add a struct for `say expr;`

**File:** `src/core/ast.hpp`

Add a new statement type after `PrintStmt` (or next to the other single-expression statements):

```cpp
struct PrintStmt : Stmt {
    std::unique_ptr<Expr> expr;
    PrintStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
};

struct SayStmt : Stmt {
    std::unique_ptr<Expr> expr;
    SayStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
};
```

So: one expression child, same shape as `PrintStmt`. No other AST changes are needed.

---

## Step 3: Parser — build the AST from tokens

The parser turns the token stream into AST nodes. We need to (a) call a new parser when we see `Say`, and (b) implement that parser.

### 3.1 Dispatch in `statement()`

**File:** `src/core/parser.cpp`

In `Parser::statement()`, add a branch for `Say` (order can matter; put it near `Print`):

```cpp
if (match(TokenType::Print)) return printStatement();
if (match(TokenType::Say))   return sayStatement();   // add this
if (match(TokenType::Let)) return letStatement();
```

### 3.2 Implement `sayStatement()`

**File:** `src/core/parser.cpp`

Add the function (e.g. right after `printStatement()`):

```cpp
std::unique_ptr<Stmt> Parser::sayStatement() {
    int line = peek().line;
    auto expr = expression();
    match(TokenType::Semicolon);
    auto s = std::make_unique<SayStmt>(std::move(expr));
    s->line = line;
    return s;
}
```

This: reads one expression, expects a semicolon, wraps the expression in a `SayStmt`, sets the line for error messages, and returns it.

### 3.3 Declare the function in the header

**File:** `src/core/parser.hpp`

In the `private` section with the other statement parsers, add:

```cpp
std::unique_ptr<Stmt> sayStatement();
```

---

## Step 4: Interpreter — execute the statement

The interpreter walks the AST. For each statement it calls `execute(stmt)`, which dispatches by type. We add a branch for `SayStmt` and implement the behavior.

### 4.1 Declare the executor

**File:** `src/core/interpreter.hpp`

With the other `execute*` declarations, add:

```cpp
void executeSay(const SayStmt& stmt);
```

### 4.2 Dispatch in `execute()`

**File:** `src/core/interpreter.cpp`

In `Interpreter::execute(Stmt& stmt)`, add (e.g. after `PrintStmt`):

```cpp
if (auto p = dynamic_cast<PrintStmt*>(&stmt)) { executePrint(*p); return; }
if (auto p = dynamic_cast<SayStmt*>(&stmt))   { executeSay(*p); return; }   // add this
if (auto p = dynamic_cast<ExprStmt*>(&stmt)) { executeExprStmt(*p); return; }
```

### 4.3 Implement `executeSay()`

**File:** `src/core/interpreter.cpp`

Add the implementation (e.g. after `executePrint()`):

```cpp
void Interpreter::executeSay(const SayStmt& stmt) {
    Value v = evaluate(*stmt.expr);
    printValue(v);
    std::cout << "\n";
}
```

This evaluates the expression, prints it with the existing `printValue()` helper, then a newline.

---

## Step 5: Build and test

From the **project root**:

```bash
cmake --build build
```

Create a test script:

```bash
echo 'say "Hello from say!"; say 1 + 2;' > test_say.melt
./build/melt test_say.melt
```

Expected output:

```
Hello from say!
3
```

If you see that, you’ve successfully added new syntax.

---

## Checklist (quick reference)

| Step | File(s) | What you did |
|------|---------|----------------|
| 1 | `lexer.hpp` | Added `Say` to `TokenType` enum. |
| 1 | `lexer.cpp` | In `identifier()`, return `Token(TokenType::Say, ...)` when `value == "say"`. |
| 2 | `ast.hpp` | Added `struct SayStmt` with `std::unique_ptr<Expr> expr`. |
| 3 | `parser.hpp` | Declared `std::unique_ptr<Stmt> sayStatement();` |
| 3 | `parser.cpp` | In `statement()`, `if (match(TokenType::Say)) return sayStatement();` and implemented `sayStatement()`. |
| 4 | `interpreter.hpp` | Declared `void executeSay(const SayStmt& stmt);` |
| 4 | `interpreter.cpp` | In `execute()`, dispatch to `executeSay` for `SayStmt`; implemented `executeSay()`. |

---

## Next: other kinds of syntax

- **New expression** (e.g. unary operator `#expr`): add a token, an expression struct in `ast.hpp`, parse it in the expression chain (e.g. in `primary()` or a new precedence level), and in `evaluate()` handle the new node with `evaluateX()`.
- **Statement with a block** (e.g. `repeat n { ... }`): add keyword token, AST node with expression + `BlockStmt` body, parse keyword + expr + `{` + statements + `}`, and in `execute()` loop and run the block.

The pattern is always: **Lexer → AST → Parser → Interpreter**. See [06_DEVELOPMENT_ADDING_SYNTAX.md](06_DEVELOPMENT_ADDING_SYNTAX.md) for more examples and the full checklist.

---

## See also

- [Getting Started](01_GETTING_STARTED.md) — build and run
- [Developer documentation](dev.html) — project structure and flow
- [Development: Adding syntax](06_DEVELOPMENT_ADDING_SYNTAX.md) — reference and extra patterns
