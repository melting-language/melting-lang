# Melt Official Website

This folder contains the **official website** for the Melt language, built with Melt itself using the MVC pattern (same as `web_project_mvc`). The **blog** is dynamic: posts are stored in MySQL and can be created/published from the site.

## Run

From the **Melt repo root**:

```bash
./bin/melt examples/official_website_using_melt/main.melt
```

Then open **http://localhost:4000** in your browser.

Without MySQL, the blog still works: it shows “No published posts yet” and you can open the “New post” form (creating a post requires MySQL).

## Blog and database

- **Table:** `blog_posts` (id, title, body, published, created_at). All posts are shown on `/blog`; unpublished ones display a “Draft” badge.
- **Database folder:** `database/models/` (e.g. `blog.melt`), `database/migrations/` (SQL), `database/seeders/` (Melt scripts to insert sample data).
- **Migrations:** Build Melt with MySQL (`make with-mysql`), ensure MySQL is running and the database exists, then from repo root:
  ```bash
  ./bin/melt examples/official_website_using_melt/run_migrations.melt
  ```
- **Seeders (optional):** To insert sample blog posts, run after migrations:
  ```bash
  ./bin/melt examples/official_website_using_melt/run_seeders.melt
  ```
- **Config:** Edit `config/database.melt` (host, user, password, database) to match your MySQL setup.
- **Create/post:** Open **http://localhost:4000/blog/new**, fill title and body, check “Publish” to show the post on `/blog`, and submit. Posts are inserted into `blog_posts`.

## Documentation page

The **Documentation** link (/documentation) serves the full Melt docs from **docs/html/index.html** when that file is present (relative path `../../docs/html/index.html` from the app). If the file cannot be read, the site falls back to a short docs index page. Run the site from the repo so the path to docs resolves (e.g. `./bin/melt examples/official_website_using_melt/main.melt` from repo root).

## Structure

- **main.melt** — Entry point; sets handler and calls `Router().dispatch(path)`.
- **config/app.melt** — App name, port (4000), tagline.
- **config/database.melt** — MySQL config for blog (host, user, password, database).
- **config/admin.melt** — Admin login (username, password, sessionSecret for cookie).
- **routes.melt** — Router: `servePublic` then routes for `/`, `/documentation`, `/about`, `/resource`, `/support`, `/blog`, `/blog/new`, POST `/blog`, and 404.
- **controllers/** — One controller per section; `BlogController` (blog), `AdminController` (login, panel, logout).
- **database/models/** — `blog.melt` defines `BlogPost` (id, title, body, published, created_at).
- **database/migrations/** — SQL files (e.g. `001_create_blog_posts_table.sql`). **run_migrations.melt** runs them once.
- **database/seeders/** — Melt scripts to seed sample data. **run_seeders.melt** runs them (e.g. `001_seed_blog_posts.melt`).
- **views/layout.melt** — `View` with `renderBlog(posts)`, `renderBlogNew()`, `renderBlogCreated()`, etc.
- **views/*.html** — Page templates; `layout.html` has the top menu. Blog: `blog.html`, `blog_new.html`, `blog_created.html`.
- **run_migrations.melt** — Runs `database/migrations/*.sql`. **run_seeders.melt** — Runs `database/seeders/*.melt`.
- **public/css/site.css** — Styles for header, nav, main, footer, blog.

## Menu

| Item           | Path            |
|----------------|-----------------|
| Home           | `/`             |
| Documentation | `/documentation`|
| About          | `/about`        |
| Resource       | `/resource`     |
| Support        | `/support`      |
| Blog           | `/blog`         |
| New post       | `/blog/new`     |
| Admin          | `/admin` (login at `/admin/login`) |

Static assets under `public/` are served at `/css/`, `/js/`, `/images/` via `servePublic(path)`.

## Admin

- **Login:** `/admin/login` — username and password from **config/admin.melt** (default: `admin` / `melt2025`). Change `admin.username`, `admin.password`, and `admin.sessionSecret` for production.
- **Panel:** `/admin` — after login, lists blog posts (ID, title, status, created) and links to create a new post and view the blog. **Logout:** `/admin/logout`.
