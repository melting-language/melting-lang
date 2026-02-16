#!/bin/sh
# Run this after creating melting-lang-syntax-highlighter on GitHub and pushing from scripts/syntax-highlighter-repo-push-me.
# Usage: ./scripts/add-syntax-highlighter-submodule.sh [repo-url]
# Default URL: git@github.com:melting-language/melting-lang-syntax-highlighter.git

set -e
REPO_URL="${1:-git@github.com:melting-language/melting-lang-syntax-highlighter.git}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
git submodule add "$REPO_URL" syntax_highlighter_extension
git add .gitmodules syntax_highlighter_extension
git status
echo "Done. Commit with: git commit -m 'Add syntax_highlighter_extension as submodule'"
