#pragma once

#include <QString>

#include <cstdint>
#include <memory>

#include "aiws/processing_core.hpp"

namespace aiws::gui {

class CorpusState;

/**
 * @brief What a state may ask the application to do (implemented by MainWindow).
 *
 * Rule for state methods: enter_state() is always the LAST thing a state does,
 * because it destroys the calling state object.
 */
class CorpusContext {
public:
    virtual ~CorpusContext() = default;

    virtual void enter_state(std::unique_ptr<CorpusState> next) = 0;
    virtual std::uint64_t start_build() = 0;
    virtual void cancel_build() = 0;
    virtual void install_corpus(std::shared_ptr<ProcessingCore> core) = 0;
    virtual void discard_corpus() = 0;
    virtual void reset_application_view() = 0;
    virtual void report_status(const QString& text) = 0;
};

/**
 * @brief State pattern: the current state object decides what each event does.
 *
 * All states live, are called and are destroyed on the GUI thread only.
 */
class CorpusState {
public:
    virtual ~CorpusState() = default;

    // Events
    virtual void build_requested(CorpusContext& ctx) = 0;
    virtual void workspace_changed(CorpusContext& ctx) = 0;
    virtual void reset_requested(CorpusContext& ctx) = 0;
    virtual void build_finished(CorpusContext& ctx, std::uint64_t generation,
                                std::shared_ptr<ProcessingCore> core) = 0;
    virtual void build_failed(CorpusContext& ctx, std::uint64_t generation,
                              const QString& message) = 0;
    virtual void shutdown(CorpusContext& ctx) = 0;

    // What the state allows
    virtual bool corpus_current() const = 0;
    virtual bool can_request_build() const = 0;
    virtual const char* name() const = 0;
};

std::unique_ptr<CorpusState> make_not_built_state();

}  // namespace aiws::gui