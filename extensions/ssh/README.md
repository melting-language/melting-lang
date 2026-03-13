# SSH extension

Connect to remote hosts over SSH and run commands from Melt.

## Build

- **With libssh:** Install `libssh` (e.g. `libssh-dev` on Debian/Ubuntu, `libssh` on macOS). The extension will be built with full support.
- **Without libssh:** The extension still builds as a stub; `sshConnect` returns false and `sshExec` returns empty string.

## Enable

In `melt.config` (project root or next to your script):

```
extension_enabled = 1
extension_dir = modules
extension = headless_browser,ffmpeg,datetime,zip,ssh
```

Or only SSH: `extension = ssh`

## Built-ins

| Function | Description |
|----------|-------------|
| `sshVersion()` | Returns extension version string (`"1.0"` with libssh, `"0 (stub)"` without). |
| `sshConnect(host, user, password [, port])` | Connect to host. Port defaults to 22. Returns true on success, false otherwise. |
| `sshExec(cmd)` | Run a command on the connected host. Returns stdout as string, or empty on error. |
| `sshClose()` | Disconnect and free the session. |

## Example

```melt
// Connect and run a command
if (sshConnect("myserver.example.com", "deploy", "secret")) {
    print sshExec("uptime");
    print sshExec("whoami");
    sshClose();
} else {
    print "Connection failed";
}
```

## Notes

- One session per process; `sshConnect` again closes the previous connection.
- Password auth only in this version; key-based auth could be added later.
- Requires **libssh** (not libssh2). Install e.g. `libssh-dev` (Debian/Ubuntu) or `libssh` (Homebrew).
