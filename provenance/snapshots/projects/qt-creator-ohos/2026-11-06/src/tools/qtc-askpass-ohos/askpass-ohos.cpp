// askpass-ohos.cpp
// OHOS variant of qtc-askpass (no UI, uses IPC).
//
// On desktop, qtc-askpass is a Qt Widgets program that pops a QInputDialog.
// OHOS forbids running UI-bearing executables directly (they must be shipped
// as HAPs through the app market), so this variant has NO Qt / NO UI at all.
// Instead, it connects to the main Qt Creator app over a LocalSocket and
// asks the main app (which already has a UI) to show the password dialog.
//
// Protocol (see docs/design/ohos-askpass-ipc-design.md, section 5.1):
//   - Request  (askpass  -> main app): <prompt text> + '\n'
//   - Response (main app -> askpass ): <password text> + '\n'
//       * An empty response (just '\n') means the user cancelled.
//
// Environment variable:
//   - QTC_ASKPASS_SOCK : path to the LocalSocket file. The main Qt Creator
//     app injects this when it forks ssh, and ssh in turn passes it to
//     this helper (ssh forks+execs $SSH_ASKPASS and reads the password
//     from this binary's stdout).
//
// Logging: uses OHOS hilog (visible via `hdc shell hilog`).
// Only errors and abnormal conditions are logged; normal success path is
// silent to avoid noise. No sensitive data (socket path, password, prompt
// text) is ever logged.

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>
#include <string>

// hilog logging (OHOS system log, visible via `hdc shell hilog`)
#define LOG_DOMAIN 0
#define LOG_TAG "qtc-askpass"
#include <hilog/log.h>

static bool sendAll(int fd, const std::string& data) {
    const char* p = data.data();
    size_t left = data.size();
    while (left > 0) {
        ssize_t n = write(fd, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        p += n;
        left -= n;
    }
    return true;
}

static std::string recvLine(int fd, size_t maxLen = 64 * 1024) {
    std::string data;
    char ch;
    while (data.size() < maxLen) {
        ssize_t n = read(fd, &ch, 1);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (n == 0) break;  // peer closed
        if (ch == '\n') break;
        data.push_back(ch);
    }
    return data;
}

int main(int argc, char* argv[]) {
    // argv[0] = path to this askpass binary
    // argv[1] = prompt text (passed by ssh, e.g. "user@host's password:")
    OH_LOG_DEBUG(LOG_APP, "QTC_ASKPASS_OHOS Enter.");
    const char* prompt = (argc > 1) ? argv[1] : "Password required.";

    // 1. Read the LocalSocket path from the environment.
    const char* sockPath = getenv("QTC_ASKPASS_SOCK");
    if (!sockPath || *sockPath == '\0') {
        OH_LOG_ERROR(LOG_APP, "QTC_ASKPASS_SOCK not set, cannot reach server");
        return 1;
    }

    // 2. connect() to the LocalSocket (AF_UNIX).
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        OH_LOG_ERROR(LOG_APP, "socket() failed errno=%{public}d", errno);
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (strlen(sockPath) >= sizeof(addr.sun_path)) {
        OH_LOG_ERROR(LOG_APP, "socket path too long");
        close(fd);
        return 1;
    }
    strncpy(addr.sun_path, sockPath, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        OH_LOG_ERROR(LOG_APP, "connect() failed errno=%{public}d", errno);
        close(fd);
        return 1;
    }

    // 3. Send the request (prompt + newline).
    std::string request = std::string(prompt) + "\n";
    if (!sendAll(fd, request)) {
        OH_LOG_ERROR(LOG_APP, "send request failed errno=%{public}d", errno);
        close(fd);
        return 1;
    }

    // 4. Receive the response (password + newline; empty line = cancel).
    std::string password = recvLine(fd);
    close(fd);

    if (password.empty()) {
        // User cancelled (or peer sent an empty line) — not an error.
        OH_LOG_INFO(LOG_APP, "empty response, user cancelled");
        return 1;
    }

    // 5. Write the password to stdout (ssh reads it via pipe).
    std::cout << password << std::endl;
    OH_LOG_DEBUG(LOG_APP, "password delivered, exit 0");
    return 0;
}
