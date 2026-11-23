#pragma once

#include <QMainWindow>
#include <QString>

#include <cstdint>
#include <memory>

#include "aiws/build_worker.hpp"
#include "aiws/gui/corpus_state.hpp"
#include "aiws/ipc_server.hpp"
#include "aiws/processing_core.hpp"
#include "aiws/workspace.hpp"

class QLabel;
class QPushButton;
class QWidget;

namespace aiws::gui {

class ContextWidget;
class SearchWidget;
class TermInspectorWidget;
class WorkspaceWidget;

/**
 * @brief Main application window and coordinator for the AI Workspace GUI.
 *
 * MainWindow owns the Workspace, the ProcessingCore, the single BuildWorker
 * and the current CorpusState. It is the CorpusContext the states act on.
 * Everything here except the worker runs on the GUI thread.
 */
class MainWindow final : public QMainWindow, private CorpusContext {
    Q_OBJECT

public:
    using ExitCallback = void (*)(QWidget* parent);

    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void set_exit_callback(ExitCallback callback) noexcept;

    bool add_document_file(const QString& path);
    void set_build_delay_ms(int ms_per_document);

    /// Name of the current corpus state ("NotBuilt", "Building", "Current").
    QString corpus_state_name() const;

private slots:
    void add_document();
    void build_corpus();
    void document_selected(int row);
    void search_requested(const QString& query, int k);
    void query_changed(const QString& query);
    void build_context_requested(int token_budget);
    void source_document_requested(const QString& document_id);
    void term_inspection_requested(const QString& term, const QString& chunk_id);
    void reset_application();
    void exit_application();

    // Worker results, delivered to the GUI thread by queued calls.
    void handle_build_finished(std::uint64_t generation, std::shared_ptr<ProcessingCore> core);
    void handle_build_failed(std::uint64_t generation, const QString& error_message);

private:
    // CorpusContext: what the states may ask for.
    void enter_state(std::unique_ptr<CorpusState> next) override;
    std::uint64_t start_build() override;
    void cancel_build() override;
    void install_corpus(std::shared_ptr<ProcessingCore> core) override;
    void discard_corpus() override;
    void reset_application_view() override;
    void report_status(const QString& text) override;

    void update_action_states();
    void clear_processing_outputs();

    Workspace workspace_;
    ProcessingCore processing_core_;
    ExitCallback exit_callback_{nullptr};
    int build_delay_ms_{0};
    std::uint64_t next_generation_{0};

    WorkspaceWidget* workspace_widget_{nullptr};
    SearchWidget* search_widget_{nullptr};
    TermInspectorWidget* term_inspector_widget_{nullptr};
    ContextWidget* context_widget_{nullptr};
    QLabel* status_label_{nullptr};
    QPushButton* reset_button_{nullptr};
    QPushButton* exit_button_{nullptr};

    std::unique_ptr<CorpusState> state_;
    std::unique_ptr<BuildWorker> worker_;  // last: destroyed (joined) first
};

}  // namespace aiws::gui