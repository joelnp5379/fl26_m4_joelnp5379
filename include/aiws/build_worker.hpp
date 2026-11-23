#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "aiws/processing_core.hpp"
#include "aiws/workspace.hpp"

namespace aiws {

struct BuildRequest {
    Workspace workspace;
    std::uint64_t generation{0};
    int delay_ms_per_document{0};  
};

/**
 * @brief The single build thread. Created once, joined in the destructor.
 *
 * Qt-free on purpose: results leave through the two callbacks, which the owner
 * runs on the worker thread (MainWindow posts them to the GUI thread).
 *
 * Shared between the GUI thread and the worker thread (all of it):
 *   - pending_, stopping_, idle_ : guarded by mutex_
 *   - wanted_generation_         : std::atomic, read by the worker between
 *                                  documents without taking the lock
 *   - on_done_, on_failed_       : const after construction
 */
class BuildWorker {
public:
    using DoneFn = std::function<void(std::uint64_t, std::shared_ptr<ProcessingCore>)>;
    using FailFn = std::function<void(std::uint64_t, std::string)>;

    BuildWorker(DoneFn on_done, FailFn on_failed);
    ~BuildWorker();  

    BuildWorker(const BuildWorker&) = delete;
    BuildWorker& operator=(const BuildWorker&) = delete;

    void submit(BuildRequest request);

    void cancel();

    bool idle() const;
    bool has_pending() const;

private:
    void run();
    void build(const BuildRequest& request);
    bool wanted(std::uint64_t generation) const;

    const DoneFn on_done_;
    const FailFn on_failed_;

    mutable std::mutex mutex_;
    std::condition_variable wake_;
    std::optional<BuildRequest> pending_;
    bool stopping_{false};
    bool idle_{false};
    std::atomic<std::uint64_t> wanted_generation_{0};

    std::thread thread_;  
};

}  // namespace aiws