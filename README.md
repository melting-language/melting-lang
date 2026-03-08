# Melt Programming Language

[![Latest Release](https://img.shields.io/github/v/release/melting-language/melting-lang)](https://github.com/melting-language/melting-lang/releases)
[![License](https://img.shields.io/github/license/melting-language/melting-lang)](LICENSE)

A lightweight, object-oriented programming language with integrated web server capabilities, designed for rapid prototyping, educational purposes, and backend development.

---

## Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Installation](#installation)
  - [Binary Installation](#binary-installation)
  - [Building from Source](#building-from-source)
  - [System Requirements](#system-requirements)
- [Getting Started](#getting-started)
  - [Running Your First Program](#running-your-first-program)
  - [Basic Usage](#basic-usage)
- [Language Reference](#language-reference)
  - [Syntax Overview](#syntax-overview)
  - [Variables and Data Types](#variables-and-data-types)
  - [Control Structures](#control-structures)
  - [Object-Oriented Programming](#object-oriented-programming)
  - [Module System](#module-system)
- [Web Development](#web-development)
  - [HTTP Server](#http-server)
  - [Request Handling](#request-handling)
  - [Template Engine](#template-engine)
- [Database Integration](#database-integration)
- [Built-in Functions](#built-in-functions)
- [Code Examples](#code-examples)
- [Documentation](#documentation)
- [Project Structure](#project-structure)
- [Contributing](#contributing)
- [License](#license)
- [Support and Community](#support-and-community)

---

## Overview

Melt is a minimal, interpreted object-oriented programming language implemented in C++. The language provides a clean syntax combined with powerful built-in functionality including HTTP server capabilities, database connectivity, file I/O operations, and JSON processing.

### Design Goals

- **Simplicity**: Clean, readable syntax suitable for learning programming concepts
- **Integration**: Built-in web server and database support without external dependencies
- **Portability**: Cross-platform support for Linux, macOS, and Windows
- **Performance**: Efficient C++ implementation with minimal overhead
- **Extensibility**: Modular architecture allowing easy integration of new features

### Use Cases

- Educational programming instruction
- Rapid web application prototyping
- Backend service development
- Scripting and automation tasks
- Learning language implementation concepts

---

## Key Features

### Core Language Features

**Data Types and Variables**
- Primitive types: numbers, strings
- Complex types: arrays, objects
- Dynamic typing with runtime type checking

**Object-Oriented Programming**
- Class-based object model
- Method definitions with parameter passing
- Constructor initialization via `init` method
- Property access and modification via `this` keyword

**Control Flow**
- Conditional statements: `if`, `else`
- Loop constructs: `while`
- Method return statements

**Code Organization**
- File-based module system with `import` statements
- Relative path resolution for imported modules
- Shared namespace for classes and variables

### Built-in Capabilities

**Web Server**
- Native HTTP server implementation
- Request routing and handler configuration
- Static file serving
- Response body, status, and header control

**Database Support**
- MySQL client integration (optional compile-time feature)
- Query execution and result set iteration
- Connection management

**File Operations**
- File reading and writing
- Path-based file access
- Error handling for I/O operations

**Data Processing**
- JSON encoding and decoding
- Array manipulation functions
- Base64 encoding/decoding
- XOR cipher for basic encryption

**Template Rendering**
- Blade-inspired template syntax
- Variable interpolation with HTML escaping
- Raw output support for trusted content

**Additional Features**
- SQLite embedded database support
- SDL2 graphics library integration
- QR code generation
- Comprehensive standard library functions

---

## Installation

### Binary Installation

#### Linux (Debian/Ubuntu)

Download the latest Debian package from the releases page:

```bash
wget https://github.com/melting-language/melting-lang/releases/latest/download/meltlang_amd64.deb
sudo dpkg -i meltlang_amd64.deb
```

Verify installation:

```bash
melt --version
```

The `melt` executable will be installed system-wide and available in your PATH.

### Building from Source

#### System Requirements

**Minimum Requirements:**
- C++17-compliant compiler (GCC 7.0+, Clang 5.0+, MSVC 2017+)
- CMake 3.10 or higher
- Build system (Make or Ninja)

**Optional Dependencies:**
- MySQL client library (libmysqlclient-dev) - For MySQL database support
- SDL2 development library (libsdl2-dev) - For GUI applications  
- QRencode library (libqrencode-dev) - For QR code generation
- SQLite3 library (libsqlite3-dev) - For embedded database support

#### Platform-Specific Dependency Installation

**Debian/Ubuntu:**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake \
                     libmysqlclient-dev libsdl2-dev \
                     libqrencode-dev libsqlite3-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install https://dev.mysql.com/get/mysql84-community-release-fc42-3.noarch.rpm
sudo dnf install gcc-c++ cmake mysql-community-devel \
                 qrencode-devel sqlite-devel SDL2-devel
```

**macOS (Homebrew):**
```bash
brew install cmake mysql sdl2 qrencode sqlite3
```

**Windows:**
- Install Visual Studio 2017 or later with C++ build tools
- Install CMake from https://cmake.org/download/
- Optional: Install vcpkg for dependency management

#### Build Process

1. Clone the repository:
```bash
git clone https://github.com/melting-language/melting-lang.git
cd melting-lang
```

2. Configure the build:
```bash
# Standard build
cmake -B build

# Build with MySQL support
cmake -B build -DUSE_MYSQL=ON

# Release build (optimized)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Custom installation prefix
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
```

3. Compile:
```bash
cmake --build build
```

4. Install (optional):
```bash
# System-wide installation (requires administrative privileges)
sudo cmake --install build

# User-local installation (ensure ~/.local/bin is in PATH)
cmake --install build --prefix ~/.local

# Custom location
cmake --install build --prefix /opt/melt
```

#### Build Configuration Options

| CMake Option | Description | Default Value |
|-------------|-------------|---------------|
| `USE_MYSQL` | Enable MySQL database integration | `OFF` |
| `CMAKE_BUILD_TYPE` | Build configuration (Debug/Release/RelWithDebInfo) | `Debug` |
| `CMAKE_INSTALL_PREFIX` | Installation directory path | `/usr/local` |
| `BUILD_SHARED_LIBS` | Build shared libraries instead of static | `OFF` |

---

## Getting Started

### Running Your First Program

Create a file named `hello.melt`:

```melt
print "Hello, World!";
```

Execute the program:

```bash
# Using installed binary
melt hello.melt

# Using build directory executable
./build/melt hello.melt
```

### Basic Usage

**Execute a script:**
```bash
melt path/to/script.melt
```

**Run built-in demonstration:**
```bash
melt
```

**Verify installation:**
```bash
melt --version
```

---

## Language Reference

### Syntax Overview

Melt uses a C-like syntax with the following characteristics:

- Statements are terminated with semicolons (`;`)
- Code blocks are delimited by braces (`{` and `}`)
- Comments: single-line (`//`) and multi-line (`/* */`)
- Case-sensitive identifiers
- Dynamic typing with runtime type inference

### Variables and Data Types

**Variable Declaration:**
```melt
let variableName = value;
```

**Supported Data Types:**

**Numbers:**
```melt
let integer = 42;
let decimal = 3.14159;
let negative = -100;
```

**Strings:**
```melt
let message = "Hello, World!";
let empty = "";
let multiWord = "Multiple words in string";
```

**Arrays:**
```melt
let numbers = [1, 2, 3, 4, 5];
let mixed = ["text", 42, 3.14];
let empty = [];
```

**Objects:**
```melt
// Objects are created through class instantiation
let point = Point(10, 20);
```

### Control Structures

**Conditional Statements:**

```melt
if (condition) {
    // Code block executed when condition is true
}

if (condition) {
    // Code for true condition
} else {
    // Code for false condition
}
```

**Loops:**

```melt
while (condition) {
    // Loop body
    // Executes while condition evaluates to true
}
```

**Operators:**

Arithmetic: `+`, `-`, `*`, `/`  
Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`  
Assignment: `=`

### Object-Oriented Programming

**Class Definition:**

```melt
class ClassName {
    method init(parameters) {
        // Constructor code
        // Initializes object properties
        this.property = value;
    }
    
    method methodName(parameters) {
        // Method implementation
        // Access object properties via 'this'
        return value;  // Optional return statement
    }
}
```

**Object Creation:**

```melt
let instance = ClassName(arguments);
```

The `init` method serves as the constructor and is automatically invoked when an object is instantiated.

**Method Invocation:**

```melt
instance.methodName(arguments);
let result = instance.methodName(arguments);
```

**Property Access:**

```melt
// Read property
let value = instance.propertyName;

// Write property
instance.propertyName = newValue;

// Within methods, use 'this'
this.propertyName = value;
```

**Complete Example:**

```melt
class BankAccount {
    method init(accountHolder, initialBalance) {
        this.holder = accountHolder;
        this.balance = initialBalance;
    }
    
    method deposit(amount) {
        this.balance = this.balance + amount;
        return this.balance;
    }
    
    method withdraw(amount) {
        if (amount <= this.balance) {
            this.balance = this.balance - amount;
            return this.balance;
        } else {
            print "Insufficient funds";
            return this.balance;
        }
    }
    
    method getBalance() {
        return this.balance;
    }
}

// Usage
let account = BankAccount("John Doe", 1000);
account.deposit(500);
account.withdraw(200);
let currentBalance = account.getBalance();
print currentBalance;  // Output: 1300
```

### Module System

**Import Statement:**

```melt
import "relative/path/to/module.melt";
```

- Paths are resolved relative to the importing file's directory
- Imported files share the same global namespace
- Classes and variables defined in imported files are accessible
- Multiple imports are supported
- Circular imports should be avoided

**Example Project Structure:**

```
project/
├── main.melt
├── models/
│   ├── user.melt
│   └── product.melt
└── utilities/
    └── helpers.melt
```

**main.melt:**
```melt
import "models/user.melt";
import "models/product.melt";
import "utilities/helpers.melt";

let user = User("admin", "admin@example.com");
let product = Product("Widget", 29.99);
```

---

## Web Development

### HTTP Server

Melt includes a built-in HTTP server for web application development without external dependencies.

**Basic Server Implementation:**

```melt
class RequestHandler {
    method handle() {
        let path = getRequestPath();
        let method = getRequestMethod();
        
        setResponseContentType("text/html");
        setResponseBody("<html><body><h1>Welcome</h1></body></html>");
        setResponseStatus(200);
    }
}

setHandler("RequestHandler");
print "Server listening on port 8080";
listen(8080);
```

### Request Handling

**Available Request Functions:**

| Function | Return Type | Description |
|----------|-------------|-------------|
| `getRequestPath()` | String | Returns the request URI path |
| `getRequestMethod()` | String | Returns HTTP method (GET, POST, etc.) |
| `getRequestBody()` | String | Returns request body content |
| `getQueryParam(name)` | String | Returns query parameter value |
| `getHeader(name)` | String | Returns request header value |

**Response Configuration:**

| Function | Parameters | Description |
|----------|-----------|-------------|
| `setResponseBody(content)` | String | Sets response body content |
| `setResponseStatus(code)` | Number | Sets HTTP status code |
| `setResponseContentType(type)` | String | Sets Content-Type header |
| `setResponseHeader(name, value)` | String, String | Sets custom response header |

**Static File Serving:**

```melt
// Serve files from public directory
servePublic("public");

// Files in public/css, public/js, public/images are accessible
// Example: http://localhost:8080/css/style.css
```

**Advanced Routing Example:**

```melt
class Router {
    method handle() {
        let path = getRequestPath();
        let method = getRequestMethod();
        
        if (path == "/" and method == "GET") {
            this.handleHome();
        } else if (path == "/api/users" and method == "GET") {
            this.handleGetUsers();
        } else if (path == "/api/users" and method == "POST") {
            this.handleCreateUser();
        } else {
            this.handle404();
        }
    }
    
    method handleHome() {
        setResponseContentType("text/html");
        setResponseBody("<h1>Home Page</h1>");
        setResponseStatus(200);
    }
    
    method handleGetUsers() {
        setResponseContentType("application/json");
        setResponseBody('[{"id": 1, "name": "User 1"}]');
        setResponseStatus(200);
    }
    
    method handleCreateUser() {
        let body = getRequestBody();
        // Process POST data
        setResponseContentType("application/json");
        setResponseBody('{"status": "created"}');
        setResponseStatus(201);
    }
    
    method handle404() {
        setResponseContentType("text/html");
        setResponseBody("<h1>404 Not Found</h1>");
        setResponseStatus(404);
    }
}

setHandler("Router");
listen(8080);
```

### Template Engine

The template engine provides Blade-inspired syntax for dynamic HTML generation.

**Template Syntax:**

- `{{ variable }}` - Escaped output (HTML entities encoded)
- `{!! variable !!}` - Raw output (no escaping)

**Example Template (template.html):**

```html
<!DOCTYPE html>
<html>
<head>
    <title>{{ pageTitle }}</title>
</head>
<body>
    <h1>{{ heading }}</h1>
    <div class="content">
        {!! htmlContent !!}
    </div>
    <p>User: {{ userName }}</p>
</body>
</html>
```

**Rendering in Melt:**

```melt
class TemplateHandler {
    method handle() {
        let data = jsonDecode('{"pageTitle": "Welcome", "heading": "Hello World", "htmlContent": "<p>This is <strong>HTML</strong></p>", "userName": "John"}');
        
        let rendered = renderView("template.html", data);
        
        setResponseContentType("text/html");
        setResponseBody(rendered);
        setResponseStatus(200);
    }
}

setHandler("TemplateHandler");
listen(8080);
```

---

## Database Integration

### MySQL Support

MySQL integration is available as an optional compile-time feature.

**Build Configuration:**

```bash
cmake -B build -DUSE_MYSQL=ON
cmake --build build
```

**Database Functions:**

| Function | Parameters | Return Type | Description |
|----------|-----------|-------------|-------------|
| `mysqlConnect(host, user, password, database)` | String × 4 | Boolean | Establishes database connection |
| `mysqlQuery(sql)` | String | Boolean | Executes SQL query |
| `mysqlFetchRow()` | None | String | Retrieves next result row (tab-separated) |
| `mysqlClose()` | None | Void | Closes database connection |

**Implementation Example:**

```melt
class DatabaseHandler {
    method connect() {
        let connected = mysqlConnect(
            "localhost",
            "username",
            "password",
            "database_name"
        );
        
        if (connected) {
            print "Database connection established";
            return true;
        } else {
            print "Database connection failed";
            return false;
        }
    }
    
    method queryUsers() {
        let success = mysqlQuery("SELECT id, name, email FROM users");
        
        if (success) {
            let row = mysqlFetchRow();
            while (row != "") {
                print row;  // Tab-separated: id\tname\temail
                row = mysqlFetchRow();
            }
        }
    }
    
    method insertUser(name, email) {
        let sql = "INSERT INTO users (name, email) VALUES ('" + name + "', '" + email + "')";
        let success = mysqlQuery(sql);
        return success;
    }
    
    method cleanup() {
        mysqlClose();
        print "Database connection closed";
    }
}

// Usage
let db = DatabaseHandler();
if (db.connect()) {
    db.insertUser("John Doe", "john@example.com");
    db.queryUsers();
    db.cleanup();
}
```

---

## Built-in Functions

### File Operations

| Function | Parameters | Return Type | Description |
|----------|-----------|-------------|-------------|
| `readFile(path)` | String | String | Reads file contents (empty string on error) |
| `writeFile(path, content)` | String, String | Boolean | Writes content to file |

**Example:**

```melt
let content = readFile("data.txt");
if (content != "") {
    print content;
}

let success = writeFile("output.txt", "File content here");
if (success) {
    print "File written successfully";
}
```

### Array Operations

| Function | Parameters | Return Type | Description |
|----------|-----------|-------------|-------------|
| `arrayCreate()` | None | Array | Creates empty array |
| `arrayPush(array, value)` | Array, Any | Void | Appends value to array |
| `arrayGet(array, index)` | Array, Number | Any | Retrieves element at index |
| `arraySet(array, index, value)` | Array, Number, Any | Void | Sets element at index |
| `arrayLength(array)` | Array | Number | Returns array length |

**Example:**

```melt
let arr = arrayCreate();
arrayPush(arr, "first");
arrayPush(arr, "second");
arrayPush(arr, "third");

let length = arrayLength(arr);  // Returns 3
let item = arrayGet(arr, 1);     // Returns "second"
arraySet(arr, 1, "modified");    // Changes second element
```

### JSON Processing

| Function | Parameters | Return Type | Description |
|----------|-----------|-------------|-------------|
| `jsonEncode(value)` | Any | String | Converts value to JSON string |
| `jsonDecode(string)` | String | Object/Array | Parses JSON to value |

**Example:**

```melt
// Encoding
let data = jsonEncode(["item1", "item2", "item3"]);
print data;  // Output: ["item1","item2","item3"]

// Decoding
let parsed = jsonDecode('{"name": "John", "age": 30}');
print parsed.name;  // Output: John
print parsed.age;   // Output: 30
```

### Encoding Functions

| Function | Parameters | Return Type | Description |
|----------|-----------|-------------|-------------|
| `base64Encode(string)` | String | String | Encodes string to Base64 |
| `base64Decode(string)` | String | String | Decodes Base64 to string |
| `xorCipher(string, key)` | String, String | String | XOR encryption/decryption |

**Example:**

```melt
let encoded = base64Encode("Hello, World!");
print encoded;  // Output: SGVsbG8sIFdvcmxkIQ==

let decoded = base64Decode(encoded);
print decoded;  // Output: Hello, World!

// XOR cipher (same function encrypts and decrypts)
let secret = xorCipher("sensitive data", "encryption-key");
let revealed = xorCipher(secret, "encryption-key");
```

### Output Functions

| Function | Parameters | Description |
|----------|-----------|-------------|
| `print(value)` | Any | Outputs value to console |

---

## Code Examples

### Example 1: Variables and Arithmetic

```melt
let width = 10;
let height = 20;
let area = width * height;

print "Width: ";
print width;
print "Height: ";
print height;
print "Area: ";
print area;
```

### Example 2: Control Flow

```melt
let age = 25;
let hasLicense = true;

if (age >= 18 and hasLicense) {
    print "Eligible to drive";
} else {
    print "Not eligible to drive";
}

let counter = 0;
while (counter < 10) {
    print counter;
    counter = counter + 1;
}
```

### Example 3: Classes and Objects

```melt
class Rectangle {
    method init(width, height) {
        this.width = width;
        this.height = height;
    }
    
    method area() {
        return this.width * this.height;
    }
    
    method perimeter() {
        return 2 * (this.width + this.height);
    }
    
    method scale(factor) {
        this.width = this.width * factor;
        this.height = this.height * factor;
    }
}

let rect = Rectangle(5, 10);
print "Area: ";
print rect.area();          // Output: 50

print "Perimeter: ";
print rect.perimeter();     // Output: 30

rect.scale(2);
print "Scaled Area: ";
print rect.area();          // Output: 200
```

### Example 4: Web API

```melt
class APIHandler {
    method handle() {
        let path = getRequestPath();
        let method = getRequestMethod();
        
        if (path == "/api/status" and method == "GET") {
            this.getStatus();
        } else if (path == "/api/data" and method == "POST") {
            this.postData();
        } else {
            this.notFound();
        }
    }
    
    method getStatus() {
        let response = jsonEncode({
            "status": "operational",
            "version": "1.0.0",
            "timestamp": 1234567890
        });
        
        setResponseContentType("application/json");
        setResponseBody(response);
        setResponseStatus(200);
    }
    
    method postData() {
        let body = getRequestBody();
        let data = jsonDecode(body);
        
        // Process data
        let response = jsonEncode({
            "received": true,
            "message": "Data processed successfully"
        });
        
        setResponseContentType("application/json");
        setResponseBody(response);
        setResponseStatus(201);
    }
    
    method notFound() {
        setResponseContentType("application/json");
        setResponseBody('{"error": "Endpoint not found"}');
        setResponseStatus(404);
    }
}

setHandler("APIHandler");
print "API server running on port 8080";
listen(8080);
```

### Example 5: File Processing

```melt
class FileProcessor {
    method processLog(filename) {
        let content = readFile(filename);
        
        if (content != "") {
            // Process file content
            let lines = this.countLines(content);
            
            let report = "Log Analysis Report\n";
            report = report + "File: " + filename + "\n";
            report = report + "Lines: " + lines + "\n";
            
            let success = writeFile("report.txt", report);
            if (success) {
                print "Report generated successfully";
            }
        } else {
            print "Error reading file";
        }
    }
    
    method countLines(content) {
        // Simplified line counting
        let count = 0;
        let i = 0;
        while (i < arrayLength(content)) {
            if (arrayGet(content, i) == "\n") {
                count = count + 1;
            }
            i = i + 1;
        }
        return count;
    }
}

let processor = FileProcessor();
processor.processLog("application.log");
```

---

## Documentation

Comprehensive documentation is available in the `docs` directory:

| Document | Description |
|----------|-------------|
| [Documentation Index](docs/README.md) | Complete documentation overview |
| [Getting Started Guide](docs/01_GETTING_STARTED.md) | Installation and initial setup |
| [Language Reference](docs/02_LANGUAGE_REFERENCE.md) | Complete syntax and semantics guide |
| [Built-in Functions Reference](docs/03_BUILTINS.md) | Comprehensive function documentation |
| [HTTP Server and MVC](docs/04_HTTP_AND_MVC.md) | Web development guide |
| [Example Programs](docs/05_EXAMPLES.md) | Annotated code examples |

**HTML Documentation:**

Formatted HTML documentation is available at `docs/html/index.html`. Open this file in a web browser for enhanced readability.

**Online Documentation:**

Visit [melting-language.github.io/melting-lang](https://melting-language.github.io/melting-lang/) for the latest online documentation.

---

## Project Structure

```
melting-lang/
├── .github/
│   └── workflows/          # CI/CD configuration
├── debian/                 # Debian packaging files
├── docs/                   # Documentation
│   ├── html/              # HTML documentation
│   ├── 01_GETTING_STARTED.md
│   ├── 02_LANGUAGE_REFERENCE.md
│   ├── 03_BUILTINS.md
│   ├── 04_HTTP_AND_MVC.md
│   └── 05_EXAMPLES.md
├── examples/              # Example programs
│   ├── example.melt
│   ├── example_oop.melt
│   ├── server.melt
│   ├── mysql_example.melt
│   └── multi_file/
├── extensions/            # Editor extensions
├── scripts/               # Build and utility scripts
├── src/                   # Source code
│   ├── lexer/
│   ├── parser/
│   ├── interpreter/
│   └── builtins/
├── CMakeLists.txt         # CMake build configuration
├── Makefile              # Make wrapper
├── README.md             # This file
└── LICENSE               # License information
```

---

## Contributing

Contributions to the Melt programming language are welcome. Please follow these guidelines:

### Development Process

1. Fork the repository on GitHub
2. Create a feature branch from `main`
3. Implement your changes with appropriate tests
4. Ensure code follows project style guidelines
5. Update documentation as needed
6. Submit a pull request with detailed description

### Code Style

- Follow existing C++ coding conventions
- Use meaningful variable and function names
- Add comments for complex logic
- Maintain consistent indentation (spaces preferred)

### Testing

- Add test cases for new features
- Ensure existing tests pass before submitting
- Include both positive and negative test scenarios

### Documentation

- Update relevant documentation files
- Add examples for new features
- Keep README.md current with new capabilities

### Issue Reporting

Report bugs and feature requests through [GitHub Issues](https://github.com/melting-language/melting-lang/issues).

Include:
- Detailed description of the issue
- Steps to reproduce (for bugs)
- Expected vs. actual behavior
- System information (OS, compiler version)
- Relevant code samples or error messages

---

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for complete terms.

---

## Support and Community

### Resources

- **Website**: [melting-language.github.io/melting-lang](https://melting-language.github.io/melting-lang/)
- **Source Code**: [github.com/melting-language/melting-lang](https://github.com/melting-language/melting-lang)
- **Issue Tracker**: [GitHub Issues](https://github.com/melting-language/melting-lang/issues)
- **Discussions**: [GitHub Discussions](https://github.com/melting-language/melting-lang/discussions)

### Getting Help

For questions and discussions:
1. Check existing documentation
2. Search closed issues for similar problems
3. Post in GitHub Discussions for general questions
4. Open an issue for bug reports or feature requests

### Release Information

Current version: See [releases page](https://github.com/melting-language/melting-lang/releases) for latest version and changelog.

Release history and version notes are maintained in the repository's release section.

---

**Melt Programming Language**  
Copyright © 2024-2026 Melt Language Contributors  
Distributed under the MIT License
