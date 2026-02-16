# Melt Language - VS Code Extension

Syntax highlighting and basic editor support for the **Melt** programming language.

## Features

- **Syntax highlighting** for:
  - Keywords: `let`, `if`, `else`, `while`, `class`, `method`, `this`, `print`, `import`
  - Strings (double-quoted)
  - Numbers
  - Operators and punctuation
  - Comments: `//` and `/* */`
- **Language configuration**: bracket matching, comment toggling, auto-closing quotes/brackets, indentation rules
- **File association**: `.melt` files are recognized as Melt source

## Installation

### From folder (development)

1. Open VS Code.
2. Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on Mac).
3. Run **Extensions: Install from VSIX...** if you have a `.vsix`, or:
4. Run **Developer: Install Extension from Location...** and select this folder (`vs_code_extension`).

### Package as VSIX (optional)

```bash
npm install -g @vscode/vsce
cd vs_code_extension
vsce package
```

Then install the generated `.vsix` via **Extensions: Install from VSIX...**.

## Usage

Open any `.melt` file; syntax highlighting and bracket/comment behavior will apply automatically.

## Structure

```
vs_code_extension/
├── package.json                 # Extension manifest
├── language-configuration.json  # Brackets, comments, indentation
├── syntaxes/
│   └── melt.tmLanguage.json    # TextMate grammar
└── README.md
```

## Melt language

Melt is a simple object-oriented language. See the project root `README.md` for the language spec and interpreter.
