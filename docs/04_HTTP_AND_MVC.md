# HTTP Server & MVC Framework

Melt includes a built-in HTTP server and an example MVC-style web project.

## Built-in HTTP server (no separate process)

- **No need for a separate web server or reverse proxy** for development.
- One handler class with **`method handle()`**, then **`setHandler("App")`** and **`listen(port)`**.
- Request data (path, method, body, headers) is available via built-ins; you build the response with the same language.

The **interpreter is built with the HTTP server included**: the `melt` binary compiles in `src/http/http_server.cpp`, and built-ins such as `listen(port)` and `setHandler("ClassName")` are part of the interpreter. The HTTP server runs **inside the same process** as your Melt script. You do **not** need to run a separate web server (e.g. nginx, Apache) or a separate backend process (e.g. Node, Python). One command starts everything:

```bash
./build/melt examples/server.melt
```

That single process parses your script, sets up the handler, and listens for HTTP connections. Each request is handled by calling your handler’s `handle()` method; the response is sent and the connection is closed. No extra daemon or reverse proxy is required for development or simple deployment.

Minimal example: **`examples/http_server_minimal.melt`** — run `./build/melt examples/http_server_minimal.melt` then open http://localhost:8765/

---

## HTTP server (any script)

### Minimal server

1. Define a class with a **`method handle()`**.
2. In `handle()`, use **`getRequestPath()`**, **`getRequestMethod()`**, **`getRequestBody()`** to read the request.
3. Use **`setResponseBody(str)`**, **`setResponseStatus(code)`**, and optionally **`setResponseContentType(str)`** to set the response.
4. Call **`setHandler("ClassName")`** and **`listen(port)`**.

Example:

```melt
class App {
    method handle() {
        let path = getRequestPath();
        if (path == "/") {
            setResponseBody("<h1>Hello</h1>");
            setResponseStatus(200);
        } else {
            setResponseBody("<h1>404</h1>");
            setResponseStatus(404);
        }
    }
}
setHandler("App");
listen(8080);
```

Run: `./build/melt examples/server.melt` then open http://localhost:8080/

### Request handling

**Request/response API**

- **Request:** `getRequestPath()`, `getRequestMethod()`, `getRequestBody()`, `getRequestHeader(name)`.
- **Response:** `setResponseBody()`, `setResponseStatus()`, `setResponseContentType()`, `setResponseHeader(name, value)`.
- **Streaming:** `streamChunk(str)` with chunked transfer for SSE or progressive output; `sleep(seconds)` for throttling.

So you get a full request/response model and streaming without extra libraries.

For each connection the server:

1. **Reads the request** — Accepts a client socket, then reads until the full request is received (headers plus body). Body length is determined by the `Content-Length` header when present.
2. **Parses it** — Extracts HTTP method, path (URI path only), body, and the raw header block from the request.
3. **Sets request data** — Calls `interp->setRequestData(path, method, body, headers)` so the interpreter has the current request for this handle.
4. **Calls the handler** — Invokes the handler class’s `handle()` method. Inside `handle()`, your script uses:
   - **`getRequestPath()`** — Request path (e.g. `/`, `/api/users`; may include query string, e.g. `/items?id=1`).
   - **`getRequestMethod()`** — HTTP method (e.g. `GET`, `POST`).
   - **`getRequestBody()`** — Raw request body (e.g. POST form data or JSON).
   - **`getRequestHeader(name)`** — Value of a request header (case-insensitive); empty string if missing.

Response is built in the interpreter via `setResponseBody`, `setResponseStatus`, `setResponseContentType`, `setResponseHeader`, and optionally `streamChunk`; the server then sends the response and closes the connection. See [Built-ins](03_BUILTINS.md) for the full list.

### Static file serving

**`servePublic(path)`** serves from a **`public/`** directory under `/js/`, `/css/`, `/images/` with correct `Content-Type`. Call it first in your router; if it returns true, the request is handled and you don’t need to route it yourself. Otherwise it returns false and you can handle the route as usual.

---

## MVC framework (web_project_mvc)

The **`examples/web_project_mvc/`** folder is a full example: config, routes, controllers, views, public assets, and database migrations.

### Running the MVC app

From the **project root**:

```bash
./build/melt examples/web_project_mvc/main.melt
```

Then open http://localhost:8080/ (or the port set in config).

### Project structure

| Path | Role |
|------|------|
| **main.melt** | Entry: imports config and routes, defines `App` with `handle()`, calls `Router().dispatch(path)`, then `listen(app.port)`. |
| **config/app.melt** | App config: `AppConfig` and global `app` (`name`, `port`, `tagline`, `debug`). Used by main and views. |
| **config/database.melt** | DB config: `DatabaseConfig` and global `db` (`host`, `user`, `password`, `database`, `enabled`). For MySQL. |
| **routes.melt** | Imports all controllers; defines `Router.dispatch(path)`: calls `servePublic(path)` first, then the right controller action. |
| **controllers/*.melt** | One class per controller (e.g. `HomeController`, `ItemsController`). Each action creates a `View()`, calls a render method, then `setResponseBody(view.output)` and `setResponseStatus(...)`. |
| **models/items.melt** | Model: e.g. `Item` class and `itemsList`. |
| **views/layout.melt** | View logic: `ViewData` and `View`; each `render*()` uses `renderView("views/...", data)` and wraps in a layout. |
| **views/*.html** | Blade-like templates: `{{ key }}` (escaped), `{!! key !!}` (raw). Layouts: `layout.html`, `layout_home.html`. Pages: `home.html`, `items.html`, `about.html`, `404.html`. |
| **public/js**, **public/css**, **public/images** | Static assets. Requests to `/js/...`, `/css/...`, `/images/...` are served from here via `servePublic`. |
| **migrations/*.sql** | SQL migration files (e.g. `001_create_items_table.sql`). |
| **run_migrations.melt** | Migration runner: connects with `config/database.melt`, ensures `_migrations` table exists, runs each listed migration once. |

### Configuration

- **App:** Edit **config/app.melt** for `app.name`, `app.port`, `app.tagline`, `app.debug`. Main and views use these.
- **Database:** Edit **config/database.melt** for `db.host`, `db.user`, `db.password`, `db.database`. Use in models/controllers that need MySQL; connect with `mysqlConnect(db.host, db.user, db.password, db.database)`.

### Blade-like templates

- **`{{ name }}`** — Replaced with the escaped value of `data.name` (HTML-safe).
- **`{!! name !!}`** — Replaced with the raw value (e.g. HTML from Melt).
- Data is a Melt object passed as the second argument to **`renderView(templatePath, data)`**; path is relative to the script directory.

### Adding a route

1. In **routes.melt**, add a branch: e.g. `else if (path == "/contact") { ContactController().index(); }`.
2. Create **controllers/contact_controller.melt** (import view, define `ContactController` with `index()` that sets response).
3. In **views/layout.melt**, add e.g. `renderContact()` that builds HTML and sets `this.output`.
4. Optionally add **views/contact.html** and use it inside `renderContact()` via `renderView`.

### Public assets

Put files in **public/css/**, **public/js/**, **public/images/**. In HTML templates use e.g.:

- `<link rel="stylesheet" href="/css/app.css">`
- `<script src="/js/app.js"></script>`
- `<img src="/images/logo.png" alt="Logo">`

Only paths under `/js/`, `/css/`, `/images/` are served from `public/`.

### Database migrations

- **Run migrations:** From project root, `./build/melt examples/web_project_mvc/run_migrations.melt`. Requires Melt built with **`-DUSE_MYSQL=ON`** and MySQL running.
- **Add a migration:** (1) Add **migrations/NNN_name.sql** with one SQL statement. (2) Append `"NNN_name.sql"` to the **migrationFiles** array in **run_migrations.melt**.
- The runner creates **`_migrations`** and records each run migration; already-run files are skipped.

---

## Official website (official_website_using_melt)

The **`examples/official_website_using_melt/`** folder is the **full official website** for the Melt language, built with Melt using the same MVC pattern. It includes a **dynamic blog** (MySQL): list posts, create new posts, and publish drafts.

### Running the official website

From the **project root**:

```bash
./build/melt examples/official_website_using_melt/main.melt
```

Then open **http://localhost:4000** in your browser.

Without MySQL, the site still runs: the blog shows “No posts yet” and the “New post” form is available (creating a post requires MySQL).

### Project structure

| Path | Role |
|------|------|
| **main.melt** | Entry: imports config and routes, defines `App` with `handle()`, calls `Router().dispatch(path)`, then `listen(app.port)`. |
| **config/app.melt** | App config: global `app` (`name`, `port` (4000), `tagline`). |
| **config/database.melt** | MySQL config: global `db` (`host`, `user`, `password`, `database`). Used by the blog. |
| **routes.melt** | Imports all controllers; defines `Router.dispatch(path)`: `servePublic(path)` first, then routes for `/`, `/documentation`, `/about`, `/resource`, `/support`, `/blog`, `/blog/new`, POST `/blog`, and 404. |
| **controllers/home_controller.melt** | Home page. |
| **controllers/documentation_controller.melt** | Documentation page (links to docs). |
| **controllers/about_controller.melt** | About page. |
| **controllers/resource_controller.melt** | Resources page. |
| **controllers/support_controller.melt** | Support page. |
| **controllers/blog_controller.melt** | Blog: `index()` (list all posts, drafts marked), `newPost()` (form), `createPost()` (POST handler). |
| **controllers/not_found_controller.melt** | 404 page. |
| **database/models/blog.melt** | `BlogPost` class (id, title, body, published, created_at). |
| **views/layout.melt** | `ViewData` and `View`; `renderHome()`, `renderDocumentation()`, `renderAbout()`, `renderResource()`, `renderSupport()`, `renderBlog(posts)`, `renderBlogNew()`, `renderBlogCreated()`, `renderNotFound(path)`. |
| **views/*.html** | Templates: `layout.html` (menu + content), `home.html`, `documentation.html`, `about.html`, `resource.html`, `support.html`, `blog.html` (`{!! postsHtml !!}`), `blog_new.html`, `blog_created.html`, `404.html`. |
| **public/css/site.css** | Styles for header, nav, main, footer, blog. |
| **database/migrations/001_create_blog_posts_table.sql** | Creates `blog_posts` table. |
| **database/seeders/** | Melt scripts to seed sample data (e.g. `001_seed_blog_posts.melt`). |
| **run_migrations.melt** | Runs `database/migrations/*.sql` once. **run_seeders.melt** runs `database/seeders/*.melt`. |

### Menu and routes

| Menu item | Path |
|-----------|------|
| Home | `/` |
| Documentation | `/documentation` |
| About | `/about` |
| Resource | `/resource` |
| Support | `/support` |
| Blog | `/blog` |
| New post | `/blog/new` |

Static assets under `public/` are served at `/css/`, `/js/`, `/images/` via `servePublic(path)`.

### Blog and database

- **Table:** `blog_posts` (id, title, body, published, created_at). All posts are shown on `/blog`; unpublished ones display a “Draft” badge.
- **Migrations:** Build Melt with MySQL (`-DUSE_MYSQL=ON`), ensure MySQL is running and the database exists, then from repo root:
  ```bash
  ./build/melt examples/official_website_using_melt/run_migrations.melt
  ```
- **Config:** Edit **config/database.melt** (host, user, password, database) to match your MySQL setup.
- **Create a post:** Open http://localhost:4000/blog/new, fill title and body, optionally check “Publish”, and submit. Posts are inserted into `blog_posts`. Use `chr(1)` when splitting `mysqlFetchRow()` output (see Built-ins).
