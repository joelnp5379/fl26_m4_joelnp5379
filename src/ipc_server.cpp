#include "aiws/ipc_server.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

namespace {

// Helper to safely write all bytes across a socket stream
bool write_data(int fd, const void* data, size_t length) {
    const char* ptr = static_cast<const char*>(data);
    size_t remaining = length;
    while (remaining > 0) {
        ssize_t written = send(fd, ptr, remaining, MSG_NOSIGNAL);
        if (written < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            return false;
        }
        if (written == 0) return false;
        ptr += written;
        remaining -= static_cast<size_t>(written);
    }
    return true;
}

// Reads exactly `length` bytes. Gives up after timeout_ms for this call, and
// also as soon as `active` turns false so a stalled client can never keep the
// server from stopping. Polls in short slices so the flag is re-checked often.
bool read_data(int fd, void* buffer, size_t length, int timeout_ms,
               const std::atomic<bool>& active) {
    char* ptr = static_cast<char*>(buffer);
    size_t remaining = length;

    const auto start_time = std::chrono::steady_clock::now();

    while (remaining > 0) {
        if (!active.load()) return false;  // server is stopping

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - start_time)
                           .count();
        int time_left = timeout_ms - static_cast<int>(elapsed);
        if (time_left <= 0) return false;

        pollfd pfd{fd, POLLIN, 0};
        int ret = poll(&pfd, 1, std::min(time_left, 100));
        if (ret < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (ret == 0) continue;  // slice expired; re-check flag and deadline

        ssize_t bytes_read = recv(fd, ptr, remaining, 0);
        if (bytes_read < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            return false;
        }
        if (bytes_read == 0) return false;

        ptr += bytes_read;
        remaining -= static_cast<size_t>(bytes_read);
    }
    return true;
}

// Validates if a file path points to a readable regular file
bool is_file_readable(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return false;

    struct stat st{};
    bool ok = (fstat(fd, &st) == 0) && S_ISREG(st.st_mode);
    close(fd);
    return ok;
}

}  // namespace

namespace aiws::gui {

IpcServer::IpcServer(std::string socket_path, AddCallback callback)
    : socket_path_(std::move(socket_path)), callback_(std::move(callback)) {}

IpcServer::~IpcServer() {
    stop();
}

bool IpcServer::start() {
    if (server_socket_ != -1) return false;

    server_socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        server_socket_ = -1;
        return false;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (socket_path_.empty() || socket_path_.size() >= sizeof(addr.sun_path)) {
        close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    std::strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

    // Remove any stale socket file left from previous runs
    unlink(socket_path_.c_str());

    if (bind(server_socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(server_socket_);
        server_socket_ = -1;
        return false;
    }

    // Only the owning user may connect and ask the app to open files.
    chmod(socket_path_.c_str(), 0600);

    if (listen(server_socket_, 10) < 0) {
        close(server_socket_);
        server_socket_ = -1;
        unlink(socket_path_.c_str());
        return false;
    }

    active_.store(true);
    try {
        server_thread_ = std::thread(&IpcServer::listen_loop, this);
    } catch (...) {
        active_.store(false);
        close(server_socket_);
        server_socket_ = -1;
        unlink(socket_path_.c_str());
        return false;
    }

    return true;
}

void IpcServer::stop() {
    // 1. Tell the server thread to finish. It re-checks this flag at least
    //    every ~100 ms, both while idle and while reading from a client.
    active_.store(false);

    // 2. Join first: after this the server thread no longer touches
    //    server_socket_, so there is no data race and no fd-reuse hazard.
    if (server_thread_.joinable()) {
        server_thread_.join();
    }

    // 3. Only now close the socket. Unlink only if this object created it, so
    //    a server that never started cannot delete someone else's socket.
    if (server_socket_ != -1) {
        close(server_socket_);
        server_socket_ = -1;
        unlink(socket_path_.c_str());
    }
}

void IpcServer::listen_loop() {
    while (active_.load()) {
        pollfd pfd{server_socket_, POLLIN, 0};
        int ret = poll(&pfd, 1, 100);

        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ret == 0) continue;

        if (pfd.revents & POLLIN) {
            int client_fd = accept(server_socket_, nullptr, nullptr);
            if (client_fd >= 0) {
                process_client(client_fd);
                close(client_fd);
            }
        }
    }
}

void IpcServer::process_client(int client_fd) {
    // Each read of the request gets up to 5 seconds before it is abandoned.
    const int timeout_ms = 5000;

    // Read 4-byte network-order length header
    uint32_t net_length = 0;
    if (!read_data(client_fd, &net_length, sizeof(net_length), timeout_ms, active_)) return;

    uint32_t path_length = ntohl(net_length);
    if (path_length == 0 || path_length > 1024 * 1024) return;

    // Read the file path payload
    std::string path_buffer(path_length, '\0');
    if (!read_data(client_fd, path_buffer.data(), path_length, timeout_ms, active_)) return;

    uint8_t status = 0;
    std::string response_msg;

    if (path_buffer.find('\0') != std::string::npos || !is_file_readable(path_buffer)) {
        status = 1;
        response_msg = "Error: File is unreadable or does not exist.";
    } else {
        try {
            if (callback_) {
                callback_(path_buffer);
            } else {
                status = 1;
                response_msg = "Error: No application handler registered.";
            }
        } catch (...) {
            status = 1;
            response_msg = "Error: Failed to queue document addition.";
        }
    }

    uint32_t msg_net_len = htonl(static_cast<uint32_t>(response_msg.size()));

    if (!write_data(client_fd, &status, sizeof(status))) return;
    if (!write_data(client_fd, &msg_net_len, sizeof(msg_net_len))) return;
    if (!response_msg.empty()) {
        write_data(client_fd, response_msg.data(), response_msg.size());
    }
}

}  // namespace aiws::gui