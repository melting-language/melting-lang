# Add `examples` as a Git submodule

The `examples` folder has been removed from this repo. To use it as a **submodule**, do the following once.

## 1. Create the examples repo on GitHub

- Go to **https://github.com/new**
- Repository name: **melting-lang-examples** (or any name you prefer)
- Owner: **melting-language** (or your user/org)
- Create the repo **empty** (no README, no .gitignore).

## 2. Push the examples content

From the project root:

```bash
cd scripts/examples-repo-push-me
git remote add origin git@github.com:melting-language/melting-lang-examples.git
git push -u origin main
cd ../..
```

(If you used another repo name or org, change the URL accordingly.)

## 3. Add the submodule to this repo

From the project root:

```bash
git submodule add git@github.com:melting-language/melting-lang-examples.git examples
git add .gitmodules examples
git commit -m "Add examples as submodule"
git push origin main
```

## After that

- **Clone with examples:**  
  `git clone --recurse-submodules https://github.com/melting-language/melting-lang.git`
- **Existing clone:**  
  `git submodule update --init --recursive`
- **Update examples to latest:**  
  `git submodule update --remote examples`
