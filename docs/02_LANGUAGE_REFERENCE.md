# Language Reference

## Comments

- **Line comment:** from `//` to the end of the line.
- **Block comment:** `/* ... */` (can span multiple lines).

Comments are ignored by the lexer.

```melt
// This is a line comment
let x = 10;  // inline comment
/*
  Block comment
  across lines
*/
```

## Statements and blocks

- Every **statement** ends with `;`.
- A **block** is a list of statements inside `{` and `}`.

## Variables

- **Declare and assign:** `let name = value;`
- **Reassign:** `name = value;` (variable must already exist).
- Variable names: letters, digits, underscore (identifier).

```melt
let x = 10;
let name = "hello";
x = 20;
```

## Types and literals

Melt is dynamically typed. Values can be:

| Type    | Literal / example        |
|---------|---------------------------|
| Number  | `42`, `3.14`              |
| String  | `"hello"`, `"line\n"`     |
| Boolean | `true`, `false` |
| Array   | `[1, 2, 3]`, `[item1, item2]` |
| Object  | Instance of a class       |
| Class   | Result of `class Name { ... }` |
| Nothing | Method returns nothing: `return;` |

- **Strings:** double-quoted; supports `\n`, `\r`, `\t`, `\\`, `\"`.
- **Numbers:** integer or decimal.
- **Arrays:** `[ expr, expr, ... ]`; elements can be any type.
- **Map (object) literal:** `[ "key" :=> value, ... ]` — PHP-style; produces a JSON-like object. Keys are strings (or numbers/booleans, coerced to string). Use for nested key-value data.
- **Booleans:** `true` and `false` are literals; comparisons and built-ins also produce boolean values.

## Expressions and operators

### Arithmetic

- `+` (addition; also string concatenation)
- `-` (subtraction)
- `-expr` (prefix numeric negation)
- `*` (multiplication)
- `/` (division)

Numeric and string `+`: if one operand is a string, the other is converted to string and concatenated.

```melt
print -1;
print -(1 + 2);
let x = 5;
print -x;
```

### Comparison

- `==` (equal)
- `!=` (not equal)
- `<`, `<=`, `>`, `>=`

Used in conditions; result is used as truthy/falsy.

### Logical

- `&&` (and) — short-circuit: right side is not evaluated if left is falsy.
- `||` (or) — short-circuit: right side is not evaluated if left is truthy.
- `!` (not) — unary; converts value to boolean and negates.

Precedence (low to high): `||`, then `&&`, then `!`, then comparison/arithmetic.

```melt
if (x > 0 && x < 10) { }
if (!flag) { }
if (a || b) { }
```

### Assignment

- `=` for variables: `x = 5;`
- `=` for object properties: `obj.field = value;`
- `=` for array index: `arr[i] = value;`

## Control flow

### if / else

```melt
if (condition) {
    // then
}
```

```melt
if (condition) {
    // then
} else {
    // else
}
```

Parentheses around the condition are required. Only one branch is executed.

### while

```melt
while (condition) {
    // body
}
```

Condition is evaluated before each iteration. Loop runs zero or more times.

### foreach

```melt
foreach (value in arrayExpr) {
    // value = each element
}
```

```melt
foreach (index, value in arrayExpr) {
    // index = 0..n-1, value = element
}
```

```melt
foreach (key, value in objectExpr) {
    // key = property name, value = property value
}
```

`foreach` works with arrays and objects. It throws a runtime error for other types.

### break / continue

```melt
while (true) {
    break;
}
```

```melt
for (let i = 0; i < 5; i = i + 1) {
    if (i == 2) continue;
    print i;
}
```

- **`break;`** — exits the nearest enclosing `while`, `for`, or `foreach`.
- **`continue;`** — skips the rest of the current iteration of the nearest enclosing loop and proceeds to the next iteration.
- Both are valid only inside loops. Using them outside a loop is a parser error.

### try / catch / throw

```melt
try {
    // statements that may throw
    throw "error message";
} catch (err) {
    // err holds the thrown value
    print(err);
}
```

- **`throw expr;`** — evaluates `expr` and throws it as an exception. Execution jumps to the nearest enclosing `catch` block. The thrown value can be any type (string, number, etc.).
- **`try { ... } catch (name) { ... }`** — runs the try block; if a `throw` happens, the thrown value is bound to `name` and the catch block runs. **Both** Melt’s `throw` and interpreter runtime errors (e.g. unknown variable, type errors like “Expected array for index”) are caught; the catch variable receives the thrown value or the error message string. Parser errors (syntax) still abort before execution. `return` inside try propagates out (it is not converted to a catchable error).

## Print

```melt
print expr;
```

Evaluates `expr` and prints its value to stdout, followed by a newline. No parentheses.

## Classes and objects

### Class definition

```melt
class ClassName {
    method init(param1, param2) {
        this.field1 = param1;
        this.field2 = param2;
    }
    method otherMethod(x) {
        // use this.field1, etc.
    }
}
```

- **`method name(params) { body }`** — defines a method. Parameters are comma-separated names.
- **`static name = expr;`** — class-level (global to the class) variable; shared by all instances. Optional; any number of static fields can appear before or after methods.
- **`init`** is the constructor: it is run when you call `ClassName(args)`.
- **`this`** refers to the current object inside a method.

Example with a static counter:

```melt
class Counter {
    static count = 0;
    method init() {
        Counter.count = Counter.count + 1;
    }
    method total() { return Counter.count; }
}
let a = Counter();
let b = Counter();
print a.total();
print Counter.count;
```

### Creating objects

```melt
let obj = ClassName(arg1, arg2);
```

- Evaluates arguments, creates a new object, runs `init` with those arguments, then returns the object.

### Property access and assignment

- **Get:** `obj.fieldName`
- **Set:** `obj.fieldName = value;`
- **Get from current object:** `this.fieldName`
- **Class (static) variables:** Declare inside a class with `static name = expr;`. They are shared by all instances. Read or write via `ClassName.fieldName` or inside methods via `ClassName.fieldName` and `this` still sees them as the class field (instance fields shadow class fields of the same name).

### Method calls

```melt
obj.methodName(arg1, arg2);
```

- The call is an expression; it returns the value returned by the method (see Return).
- If the method does not execute `return expr`, the call yields a “nothing” value.

### Return

- **Return a value:** `return expr;`
- **Return nothing:** `return;`

Only valid inside a method. The method call expression then evaluates to the returned value (or “nothing”). A method can return numbers, strings, objects, arrays, etc.

```melt
method add(a, b) {
    return a + b;
}
let sum = obj.add(3, 5);  // sum is 8
```

## Arrays

- **Literal:** `[ expr, expr, ... ]`
- **Index (read):** `arr[index]` — index is 0-based.
- **Index (write):** `arr[index] = value;`
- Built-ins: `arrayCreate()`, `arrayPush(arr, value)`, `arrayGet(arr, i)`, `arraySet(arr, i, value)`, `arrayLength(arr)` — see [Built-in Functions](03_BUILTINS.md).

## Import

```melt
import "path.melt";
import "path.melt" as M;
```

- **path** is relative to the **directory of the file that contains this import**.
- **Without `as`:** The imported file is executed once; its top-level variables and classes are available in the current file (shared global scope).
- **With `as M`:** The module is run in its own scope and its exports (top-level variables and classes) are bound to the single variable **M**. Use the class via **M.ClassName** (e.g. `M.MyClass(1, 2)`). One line: import and define the namespace variable.
- Circular or repeated imports of the same resolved path are skipped.

Example (from a file in `examples/web_project_mvc/`):

```melt
import "config/app.melt";
import "../views/layout.melt";
```

## Truthiness

Used in `if` and `while`:

- **Falsy:** `false`, `0`, empty string `""`, empty array, “nothing” (return with no value).
- **Truthy:** anything else (e.g. non-zero numbers, non-empty strings, non-empty arrays, objects).

## Reserved words and identifiers

- **Keywords:** `let`, `if`, `else`, `while`, `print`, `class`, `method`, `static`, `this`, `import`, `return`, `try`, `catch`, `throw`. **Literals:** `true`, `false`.
- **Identifier:** `method` is a keyword; do not use it as a variable name (e.g. use `reqMethod` for “request method”).
- Other names (variables, classes, methods, properties) use letters, digits, and underscore.

## File extension

Use **`.melt`** for Melt source files. The interpreter does not require the extension on the command line, but paths in `import` and `readFile` typically include it.
