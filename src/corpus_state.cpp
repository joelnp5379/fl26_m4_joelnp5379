#include "aiws/gui/corpus_state.hpp"

#include <utility>

namespace aiws::gui {
namespace {

// No usable corpus (startup, after Reset, after a Workspace change, after a
// cancelled or failed build).
class NotBuilt final : public CorpusState {
public:
    void build_requested(CorpusContext& ctx) override;
    void workspace_changed(CorpusContext& ctx) override;
    void reset_requested(CorpusContext& ctx) override;
    void build_finished(CorpusContext&, std::uint64_t, std::shared_ptr<ProcessingCore>) override {}
    void build_failed(CorpusContext&, std::uint64_t, const QString&) override {}
    void shutdown(CorpusContext&) override {}
    bool corpus_current() const override { return false; }
    bool can_request_build() const override { return true; }
    const char* name() const override { return "NotBuilt"; }
};

// A build for the present Workspace is running on the worker. Remembers which one.
class Building final : public CorpusState {
public:
    explicit Building(std::uint64_t generation) : generation_(generation) {}
    void build_requested(CorpusContext&) override {}  // decision 1: ignored while building
    void workspace_changed(CorpusContext& ctx) override;
    void reset_requested(CorpusContext& ctx) override;
    void build_finished(CorpusContext& ctx, std::uint64_t generation,
                        std::shared_ptr<ProcessingCore> core) override;
    void build_failed(CorpusContext& ctx, std::uint64_t generation,
                      const QString& message) override;
    void shutdown(CorpusContext& ctx) override { ctx.cancel_build(); }
    bool corpus_current() const override { return false; }
    bool can_request_build() const override { return false; }
    const char* name() const override { return "Building"; }

private:
    std::uint64_t generation_;
};

// A corpus built from the present Workspace is installed and usable.
class Current final : public CorpusState {
public:
    void build_requested(CorpusContext& ctx) override;
    void workspace_changed(CorpusContext& ctx) override;
    void reset_requested(CorpusContext& ctx) override;
    void build_finished(CorpusContext&, std::uint64_t, std::shared_ptr<ProcessingCore>) override {}
    void build_failed(CorpusContext&, std::uint64_t, const QString&) override {}
    void shutdown(CorpusContext&) override {}
    bool corpus_current() const override { return true; }
    bool can_request_build() const override { return true; }
    const char* name() const override { return "Current"; }
};

// Not Built

void NotBuilt::build_requested(CorpusContext& ctx) {
    if (const std::uint64_t generation = ctx.start_build()) {
        ctx.enter_state(std::make_unique<Building>(generation));
    }
}

void NotBuilt::workspace_changed(CorpusContext& ctx) {
    ctx.discard_corpus();
}

void NotBuilt::reset_requested(CorpusContext& ctx) {
    ctx.reset_application_view();
    ctx.enter_state(std::make_unique<NotBuilt>());
}

// Building

void Building::workspace_changed(CorpusContext& ctx) {
    ctx.cancel_build();  // decision 2: the running build is useless now
    ctx.discard_corpus();
    ctx.report_status("Build cancelled: the workspace changed.");
    ctx.enter_state(std::make_unique<NotBuilt>());
}

void Building::reset_requested(CorpusContext& ctx) {
    ctx.cancel_build();
    ctx.reset_application_view();
    ctx.enter_state(std::make_unique<NotBuilt>());
}

void Building::build_finished(CorpusContext& ctx, std::uint64_t generation,
                              std::shared_ptr<ProcessingCore> core) {
    if (generation != generation_) return;  // late result of an earlier, cancelled build
    ctx.install_corpus(std::move(core));
    ctx.enter_state(std::make_unique<Current>());
}

void Building::build_failed(CorpusContext& ctx, std::uint64_t generation,
                            const QString& message) {
    if (generation != generation_) return;
    ctx.discard_corpus();
    ctx.report_status(QString("Rebuild failed - %1").arg(message));
    ctx.enter_state(std::make_unique<NotBuilt>());
}

// Current

void Current::build_requested(CorpusContext& ctx) {
    if (const std::uint64_t generation = ctx.start_build()) {
        ctx.discard_corpus();  // the old corpus is not usable while rebuilding
        ctx.enter_state(std::make_unique<Building>(generation));
    }
}

void Current::workspace_changed(CorpusContext& ctx) {
    ctx.discard_corpus();
    ctx.enter_state(std::make_unique<NotBuilt>());
}

void Current::reset_requested(CorpusContext& ctx) {
    ctx.reset_application_view();
    ctx.enter_state(std::make_unique<NotBuilt>());
}

}  // namespace

std::unique_ptr<CorpusState> make_not_built_state() {
    return std::make_unique<NotBuilt>();
}

}  // namespace aiws::gui