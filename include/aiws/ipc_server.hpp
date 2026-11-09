#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace aiws::gui {

class IpcServer {
public:
    using AddCallback = std::function<void(const std::string&)>;

    explicit IpcServer(std::string socket_path, AddCallback callback);
    ~IpcServer();

    bool start();
    void stop();

private:
    void listen_loop();
    void process_client(int client_fd);

    std::string socket_path_;
    AddCallback callback_;
    std::thread server_thread_;
    std::atomic<bool> active_{false};
    int server_socket_{-1};
};

} // namespace aiws::gui