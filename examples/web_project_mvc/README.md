# Melt MVC Web Project

A small website built with **Melt** using a Model-View-Controller (MVC) style structure.

## Structure

- **`main.melt`** – Entry point: imports `config/app.melt` and `routes.melt`, defines the `App` handler, and calls `listen(app.port)`.
- **`config/app.melt`** – App configuration: defines `AppConfig` and a global `app` instance with `name`, `port`, `tagline`, `debug`. Used by `main.melt` (port, name) and by the view (tagline, name for the home page).
- **`config/database.melt`** – Database configuration: defines `DatabaseConfig` and a global `db` instance with `host`, `user`, `password`, `database`, `enabled`. For use with MySQL (build with `make with-mysql`). In any file that needs the DB, `import "../config/database.melt"` (or `import "config/database.melt"` from `main.melt`) and call `mysqlConnect(db.host, db.user, db.password, db.database)`.
- **`routes.melt`** – Routes: imports all controllers, defines `Router` with `dispatch(path)` that matches path and calls the right controller action (e.g. `HomeController().index()`).
- **`controllers/`** – One file per controller. Each imports the view (and model if needed) and defines one class with actions that create a `View`, call a render method, and set the response.
  - `home_controller.melt` – `HomeController.index()`
  - `items_controller.melt` – `ItemsController.index()` (imports `models/items.melt`)
  - `about_controller.melt` – `AboutController.index()`
  - `not_found_controller.melt` – `NotFoundController.show(path)` for 404
- **`models/items.melt`** – Model layer: `Item` class and `itemsList` (data).
- **`views/layout.melt`** – View logic: `ViewData` and `View` class; each `render*()` loads a template with `renderView()` and passes data.
- **`views/*.html`** – Blade-like templates: HTML files with `{{ name }}` (escaped) and `{!! name !!}` (raw). Layouts: `layout.html`, `layout_home.html`. Pages: `home.html`, `items.html`, `about.html`, `404.html`.
- **`public/`** – Public static assets. Only paths under `/js/`, `/css/`, and `/images/` are served (via `servePublic(path)` in the router). Put files in `public/js/`, `public/css/`, `public/images/` and reference them in views as `/js/...`, `/css/...`, `/images/...`. Sample files: `public/css/app.css`, `public/js/app.js`.
- **`migrations/`** – SQL migration files (e.g. `001_create_items_table.sql`). Run with `./bin/melt examples/web_project_mvc/run_migrations.melt` from the project root. The runner creates a `_migrations` table and runs each listed file once. Add new migrations by creating a `.sql` file and appending its name to the list in `run_migrations.melt`.
- **`run_migrations.melt`** – Migration runner: connects using `config/database.melt`, ensures `_migrations` table exists, then runs each migration file in order (skipping any already recorded).

## How to run

From the **project root** (so imports resolve correctly):

```bash
./bin/melt examples/web_project_mvc/main.melt
```

Then open in a browser:

- **http://localhost:8080/** – Home
- **http://localhost:8080/items** – Items list (from model)
- **http://localhost:8080/about** – About
- Any other path – 404 page

## How it works

1. **Entry** (`main.melt`): `App.handle()` gets `getRequestPath()` and calls `Router().dispatch(path)`.
2. **Routes** (`routes.melt`): `Router.dispatch(path)` matches the path and calls the right controller action (e.g. `HomeController().index()`, `ItemsController().index()`). Add or change routes here.
3. **Controllers**: Each controller imports `../views/layout.melt` (and `../models/...` if it needs data). Its action creates a `View()`, calls the right `view.render*()` method, then `setResponseBody(view.output)` and `setResponseStatus(...)`.
4. **View**: The `View` class in `views/layout.melt` calls the built-in `renderView(path, data)` to load a template. Data is a `ViewData` object whose fields are passed to the template.

## Blade-like templates

Templates are plain HTML files in `views/` with placeholders:

- **`{{ name }}`** – Escaped output (safe for text; `<` becomes `&lt;` etc.).
- **`{!! name !!}`** – Raw output (use for HTML you build in Melt, e.g. layout content or list markup).

Example template:

```html
<title>{{ title }}</title>
<div class="content">{!! content !!}</div>
```

In Melt you pass an object with those keys:

```melt
let data = ViewData();
data.title = "Page Title";
data.content = "<p>HTML body</p>";
this.output = renderView("views/layout.html", data);
```

- **Layouts**: `layout.html` (default) and `layout_home.html` (home) wrap page content in `{!! content !!}`.
- **Pages**: `home.html`, `items.html`, `about.html`, `404.html` are rendered first; their output is passed as `content` into a layout.

**Configuration** – Edit `config/app.melt` to change `app.name`, `app.port`, `app.tagline`, or `app.debug`. The view uses `app.tagline` and `app.name` for the home page; `main.melt` uses `app.port` and `app.name` for the server.

**Database** – Edit `config/database.melt` to set `db.host`, `db.user`, `db.password`, `db.database`. Set `db.enabled = 1` if you use MySQL. In models or controllers that need the DB, import `../config/database.melt` and connect with `mysqlConnect(db.host, db.user, db.password, db.database)`. The Melt binary must be built with `make with-mysql` for MySQL built-ins to be available.

**Migrations** – From the project root, run `./bin/melt examples/web_project_mvc/run_migrations.melt`. The runner uses `config/database.melt`, creates `_migrations` if needed, and runs each SQL file in `migrations/` that is listed in `run_migrations.melt`, in order. Already-run migrations are skipped. To add a migration: (1) add a new `.sql` file in `migrations/` (e.g. `002_add_users_table.sql`) with one or more SQL statements; (2) append the filename to the `migrationFiles` array in `run_migrations.melt`. Each migration file should contain a single SQL statement (or the runner can be extended to split by `;`).

**Public assets** – Put CSS in `public/css/`, JavaScript in `public/js/`, and images in `public/images/`. The router calls `servePublic(path)` first; requests to `/css/...`, `/js/...`, or `/images/...` are served from these folders with the correct `Content-Type`. In your HTML templates, use e.g. `<link rel="stylesheet" href="/css/app.css">` and `<script src="/js/app.js"></script>`.

To add a route: add a branch in `routes.melt`, a new controller file in `controllers/` (or a new action in an existing controller), a `render*()` in `views/layout.melt` if needed, and optionally a new `views/yourpage.html` template. Controllers in `controllers/` must use `../views/layout.melt` and `../models/...` for imports because paths are relative to the current file’s directory.
