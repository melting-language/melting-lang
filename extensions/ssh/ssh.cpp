// SSH extension for Melt — connect, exec, close.
// Enable in melt.config: extension = ssh
// Requires: libssh (pkg-config libssh)
// Built-ins: sshConnect(host, user, password [, port])  sshExec(cmd)  sshClose()  sshVersion()

#include "interpreter.hpp"
#include <string>
#include <cstring>

#if defined(USE_LIBSSH) && USE_LIBSSH
#include <libssh/libssh.h>

namespace {
static ssh_session g_session = nullptr;

static std::string asString(const Value& v) {
    if (auto* s = std::get_if<std::string>(&v)) return *s;
    if (auto* d = std::get_if<double>(&v)) return std::to_string(static_cast<int>(*d));
    return "";
}
} // namespace

static Value sshVersion(Interpreter*, std::vector<Value>) {
    return Value(std::string("1.0"));
}

static Value sshConnect(Interpreter*, std::vector<Value> args) {
    if (args.size() < 3) return Value(false);
    std::string host = asString(args[0]);
    std::string user = asString(args[1]);
    std::string password = asString(args[2]);
    int port = 22;
    if (args.size() >= 4 && std::holds_alternative<double>(args[3]))
        port = static_cast<int>(std::get<double>(args[3]));
    if (host.empty() || user.empty()) return Value(false);

    if (g_session) {
        ssh_disconnect(g_session);
        ssh_free(g_session);
        g_session = nullptr;
    }

    g_session = ssh_new();
    if (!g_session) return Value(false);

    ssh_options_set(g_session, SSH_OPTIONS_HOST, host.c_str());
    ssh_options_set(g_session, SSH_OPTIONS_USER, user.c_str());
    ssh_options_set(g_session, SSH_OPTIONS_PORT, &port);

    if (ssh_connect(g_session) != SSH_OK) {
        ssh_free(g_session);
        g_session = nullptr;
        return Value(false);
    }

    if (ssh_userauth_password(g_session, nullptr, password.c_str()) != SSH_AUTH_SUCCESS) {
        ssh_disconnect(g_session);
        ssh_free(g_session);
        g_session = nullptr;
        return Value(false);
    }
    return Value(true);
}

static Value sshExec(Interpreter*, std::vector<Value> args) {
    if (!g_session || args.empty()) return Value(std::string(""));
    std::string cmd = asString(args[0]);
    if (cmd.empty()) return Value(std::string(""));

    ssh_channel channel = ssh_channel_new(g_session);
    if (!channel) return Value(std::string(""));

    if (ssh_channel_open_session(channel) != SSH_OK) {
        ssh_channel_free(channel);
        return Value(std::string(""));
    }

    if (ssh_channel_request_exec(channel, cmd.c_str()) != SSH_OK) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return Value(std::string(""));
    }

    std::string out;
    char buf[4096];
    int n;
    while ((n = ssh_channel_read(channel, buf, sizeof(buf), 0)) > 0)
        out.append(buf, static_cast<size_t>(n));
    if (n < 0) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return Value(std::string(""));
    }

    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return Value(out);
}

static Value sshClose(Interpreter*, std::vector<Value>) {
    if (g_session) {
        ssh_disconnect(g_session);
        ssh_free(g_session);
        g_session = nullptr;
    }
    return Value(false);
}

extern "C" void melt_register(Interpreter* interp) {
    interp->registerBuiltin("sshVersion", sshVersion);
    interp->registerBuiltin("sshConnect", sshConnect);
    interp->registerBuiltin("sshExec", sshExec);
    interp->registerBuiltin("sshClose", sshClose);
}

#else
// Stub when libssh not available: functions return false/empty; check sshVersion() for "0 (stub)".
extern "C" void melt_register(Interpreter* interp) {
    interp->registerBuiltin("sshVersion", [](Interpreter*, std::vector<Value>) { return Value(std::string("0 (stub)")); });
    interp->registerBuiltin("sshConnect", [](Interpreter*, std::vector<Value>) { return Value(false); });
    interp->registerBuiltin("sshExec", [](Interpreter*, std::vector<Value>) { return Value(std::string("")); });
    interp->registerBuiltin("sshClose", [](Interpreter*, std::vector<Value>) { return Value(false); });
}
#endif
