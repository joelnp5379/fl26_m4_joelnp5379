#include "aiws/build_worker.hpp"

#include <chrono>
#include <cstddef>
#include <exception>
#include <utility>

namespace aiws {

BuildWorker::BuildWorker(DoneFn on_done, FailFn on_failed)
    : on_done_(std::move(on_done)),
      on_failed_(std::move(on_failed)),
      thread_(&BuildWorker::run, this) {}

BuildWorker::~BuildWorker() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
        pending_.reset();
        wanted_generation_.store(0);  // a running build sees this and stops
    }
#ifndef AIWS_FAULT_SILENT_SHUTDOWN
    wake_.notify_all();  // wakes an idle worker so it can see stopping_
#endif
    thread_.join();
}

void BuildWorker::submit(BuildRequest request) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        wanted_generation_.store(request.generation);  // older builds are unwanted now
        pending_ = std::move(request);                 // replaces an untaken request
    }
    wake_.notify_one();
}

void BuildWorker::cancel() {
    std::lock_guard<std::mutex> lock(mutex_);
    wanted_generation_.store(0);
    pending_.reset();
}

bool BuildWorker::idle() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return idle_;
}

bool BuildWorker::has_pending() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_.has_value();
}

bool BuildWorker::wanted(std::uint64_t generation) const {
#ifdef AIWS_FAULT_NO_CANCEL
    (void)generation;
    return true;  // fault: a build nobody wants any more runs to the end
#else
    return wanted_generation_.load() == generation;
#endif
}

void BuildWorker::run() {
    std::unique_lock<std::mutex> lock(mutex_);
    for (;;) {
        idle_ = true;
        wake_.wait(lock, [this] { return stopping_ || pending_.has_value(); });
        idle_ = false;
        if (stopping_) return;

        BuildRequest request = std::move(*pending_);
        pending_.reset();
        lock.unlock();  // never hold the lock while building

        if (wanted(request.generation)) {  // an outdated request is never built
            build(request);
        }
        lock.lock();
    }
}

void BuildWorker::build(const BuildRequest& request) {
    const std::uint64_t generation = request.generation;
    try {
        // Simulated slow build: one delay per document, with a check before each
        // one, so a cancelled build stops within about one document's time.
        const std::size_t documents = request.workspace.documents().size();
        for (std::size_t i = 0; i < documents; ++i) {
            if (!wanted(generation)) return;
            if (request.delay_ms_per_document > 0) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(request.delay_ms_per_document));
            }
        }
        if (!wanted(generation)) return;

        auto core = std::make_shared<ProcessingCore>();
        core->rebuild(request.workspace);

        if (!wanted(generation)) return;  // a cancelled build is never delivered
        on_done_(generation, std::move(core));
    } catch (const std::exception& e) {
        if (!wanted(generation)) return;
        on_failed_(generation, e.what());
    }
}

}  // namespace aiws