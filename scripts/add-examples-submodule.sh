#!/bin/sh
# Run this after creating melting-lang-examples on GitHub and pushing from scripts/examples-repo-push-me.
# Usage: ./scripts/add-examples-submodule.sh [repo-url]
# Default URL: git@github.com:melting-language/melting-lang-examples.git

set -e
REPO_URL="${1:-git@github.com:melting-language/melting-lang-examples.git}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
git submodule add "$REPO_URL" examples
git add .gitmodules examples
git status
echo "Done. Commit with: git commit -m 'Add examples as submodule'"
