#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <filesystem>

bool write_all(int fd, const void* buffer, size_t count) {
    const char* ptr = static_cast<const char*>(buffer);
    size_t bytes_left = count;
    while (bytes_left > 0) {
        ssize_t written = write(fd, ptr, bytes_left);
        if (written <= 0) return false;
        bytes_left -= written;
        ptr += written;
    }
    return true;
}

bool read_all(int fd, void* buffer, size_t count) {
    char* ptr = static_cast<char*>(buffer);
    size_t bytes_left = count;
    while (bytes_left > 0) {
        ssize_t bytes_read = read(fd, ptr, bytes_left);
        if (bytes_read <= 0) return false;
        bytes_left -= bytes_read;
        ptr += bytes_read;
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::string socket_path;
    std::string file_arg;

    const char* user = std::getenv("USER");
    if (user && user[0]) {
        socket_path = "/tmp/aiws-" + std::string(user) + ".sock";
    } else {
        socket_path = "/tmp/aiws-" + std::to_string(getuid()) + ".sock";
    }

    int arg_idx = 1;
    if (argc > 1 && std::string(argv[1]) == "--socket") {
        if (argc < 4) {
            std::cerr << "Usage: aiws_send [--socket PATH] add FILE\n";
            return 2;
        }
        socket_path = argv[2];
        arg_idx = 3;
    }

    if (argc != arg_idx + 2 || std::string(argv[arg_idx]) != "add") {
        std::cerr << "Usage: aiws_send [--socket PATH] add FILE\n";
        return 2;
    }

    file_arg = argv[arg_idx + 1];

    std::error_code ec;
    std::filesystem::path abs_path = std::filesystem::absolute(file_arg, ec);
    std::string file_path = ec ? file_arg : abs_path.string();

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) return 2;

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (socket_path.size() >= sizeof(addr.sun_path)) {
        close(sockfd);
        return 2;
    }
    std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);

    // Non-blocking connect with 2-second timeout
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    connect(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));

    struct pollfd pfd{sockfd, POLLOUT, 0};
    if (poll(&pfd, 1, 2000) <= 0) {
        close(sockfd);
        return 2;
    }

    // Restore blocking and set 5-second IO timeouts
    fcntl(sockfd, F_SETFL, flags);
    struct timeval tv{5, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    // Send payload length + path string
    uint32_t net_len = htonl(static_cast<uint32_t>(file_path.size()));
    if (!write_all(sockfd, &net_len, sizeof(net_len)) ||
        !write_all(sockfd, file_path.data(), file_path.size())) {
        close(sockfd);
        return 2;
    }

    // Read response status byte
    uint8_t status = 0;
    if (!read_all(sockfd, &status, sizeof(status))) {
        close(sockfd);
        return 2;
    }

    // Read response message length + text
    uint32_t net_msg_len = 0;
    if (!read_all(sockfd, &net_msg_len, sizeof(net_msg_len))) {
        close(sockfd);
        return 2;
    }
    uint32_t msg_len = ntohl(net_msg_len);

    std::string msg_buf(msg_len, '\0');
    if (msg_len > 0) {
        read_all(sockfd, msg_buf.data(), msg_len);
    }

    close(sockfd);

    if (status == 0) {
        return 0;
    } else {
        if (!msg_buf.empty()) {
            std::cerr << msg_buf << "\n";
        }
        return 1;
    }
}