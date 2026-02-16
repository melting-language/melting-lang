# Add `syntax_highlighter_extension` (and VS Code extension) as a Git submodule

The `syntax_highlighter_extension` folder (which includes `vs_code_extension`) has been removed from this repo. To use it as a **submodule**, do the following once.

## 1. Create the repo on GitHub

- Go to **https://github.com/new**
- Repository name: **melting-lang-syntax-highlighter** (or any name you prefer)
- Owner: **melting-language** (or your user/org)
- Create the repo **empty** (no README, no .gitignore).

## 2. Push the syntax highlighter content

From the project root:

```bash
cd scripts/syntax-highlighter-repo-push-me
git remote add origin git@github.com:melting-language/melting-lang-syntax-highlighter.git
git push -u origin main
cd ../..
```

(If you used another repo name or org, change the URL accordingly.)

## 3. Add the submodule to this repo

From the project root:

```bash
./scripts/add-syntax-highlighter-submodule.sh
git commit -m "Add syntax_highlighter_extension as submodule"
```

Or manually:

```bash
git submodule add git@github.com:melting-language/melting-lang-syntax-highlighter.git syntax_highlighter_extension
git add .gitmodules syntax_highlighter_extension
git commit -m "Add syntax_highlighter_extension as submodule"
```

## After that

- **Clone with submodules:**  
  `git clone --recurse-submodules https://github.com/melting-language/melting-lang.git`
- **Existing clone:**  
  `git submodule update --init --recursive`
- **Update to latest:**  
  `git submodule update --remote syntax_highlighter_extension`

The VS Code extension lives at `syntax_highlighter_extension/vs_code_extension/`.
