#include "aiws/gui/main_window.hpp"
#include "aiws/gui/workspace_widget.hpp"
#include "aiws/gui/search_widget.hpp"
#include "aiws/gui/term_inspector_widget.hpp"
#include "aiws/gui/context_widget.hpp"
#include "aiws/ipc_server.hpp"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>

#include <utility>

namespace aiws::gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), state_(make_not_built_state()) {
    // The one build thread. Its callbacks run on the worker thread and only
    // post a queued call to the GUI thread; they touch no widget and no state.
    worker_ = std::make_unique<BuildWorker>(
        [this](std::uint64_t generation, std::shared_ptr<ProcessingCore> core) {
            QMetaObject::invokeMethod(this, [this, generation, core]() {
                handle_build_finished(generation, core);
            }, Qt::QueuedConnection);
        },
        [this](std::uint64_t generation, std::string message) {
            const QString text = QString::fromStdString(message);
            QMetaObject::invokeMethod(this, [this, generation, text]() {
                handle_build_failed(generation, text);
            }, Qt::QueuedConnection);
        });

    // Create Central Widget and Vertical Layout
    auto* central_widget = new QWidget(this);
    setCentralWidget(central_widget);
    auto* main_layout = new QVBoxLayout(central_widget);

    auto* header_label = new QLabel("<b>AI Workspace</b><br><i>Document Search and Context Explorer</i>", this);
    header_label->setStyleSheet("background-color: #7B1FA2; color: white; padding: 10px; font-size: 16px;");

    // Widget Creation
    workspace_widget_ = new WorkspaceWidget(this);
    search_widget_ = new SearchWidget(this);
    term_inspector_widget_ = new TermInspectorWidget(this);
    context_widget_ = new ContextWidget(this);

    // Bottom layout creation with status buttons
    auto* bottom_layout = new QHBoxLayout();
    status_label_ = new QLabel("Current status: Ready", this);
    reset_button_ = new QPushButton("Reset", this);
    exit_button_ = new QPushButton("Exit", this);

    reset_button_->setStyleSheet("background-color: #F57C00; color: white; font-weight: bold; padding: 5px;");
    exit_button_->setStyleSheet("background-color: #1976D2; color: white; font-weight: bold; padding: 5px;");

    bottom_layout->addWidget(status_label_);
    bottom_layout->addStretch();
    bottom_layout->addWidget(reset_button_);
    bottom_layout->addWidget(exit_button_);

    // Horizontal layout for term inspector and context widget
    auto* tools_layout = new QHBoxLayout();
    tools_layout->addWidget(term_inspector_widget_, 1);
    tools_layout->addWidget(context_widget_, 1);

    // Add widgets to the main layout
    main_layout->addWidget(header_label);
    main_layout->addWidget(workspace_widget_, 1);
    main_layout->addWidget(search_widget_, 2);
    main_layout->addLayout(tools_layout, 1);
    main_layout->addLayout(bottom_layout);

    // Connect signals and slots
    connect(workspace_widget_, &WorkspaceWidget::add_document_requested, this, &MainWindow::add_document);
    connect(workspace_widget_, &WorkspaceWidget::build_corpus_requested, this, &MainWindow::build_corpus);
    connect(workspace_widget_, &WorkspaceWidget::document_selected, this, &MainWindow::document_selected);

    connect(search_widget_, &SearchWidget::search_requested, this, &MainWindow::search_requested);
    connect(search_widget_, &SearchWidget::query_changed, this, &MainWindow::query_changed);
    connect(search_widget_, &SearchWidget::source_document_requested, this, &MainWindow::source_document_requested);

    connect(search_widget_, &SearchWidget::selected_chunk_changed, term_inspector_widget_, &TermInspectorWidget::set_selected_chunk);

    connect(context_widget_, &ContextWidget::build_context_requested, this, &MainWindow::build_context_requested);
    connect(term_inspector_widget_, &TermInspectorWidget::term_inspection_requested, this, &MainWindow::term_inspection_requested);

    connect(reset_button_, &QPushButton::clicked, this, &MainWindow::reset_application);
    connect(exit_button_, &QPushButton::clicked, this, &MainWindow::exit_application);

    resize(900, 900);
    reset_application();
}

MainWindow::~MainWindow() {
    state_->shutdown(*this);  // Building: tell the worker its build is not wanted
    worker_.reset();          // joins the worker; returns within one document's time
}

void MainWindow::set_exit_callback(ExitCallback callback) noexcept {
    exit_callback_ = callback;
}

void MainWindow::set_build_delay_ms(int ms_per_document) {
    build_delay_ms_ = ms_per_document;
}

QString MainWindow::corpus_state_name() const {
    return QString::fromLatin1(state_->name());
}

// Events from the widgets

void MainWindow::add_document() {
    QString file_path = QFileDialog::getOpenFileName(this, "Select Document", "", "Text Files (*.txt);;All Files (*)");
    if (!file_path.isEmpty()) {
        add_document_file(file_path);
    }
}

bool MainWindow::add_document_file(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;  // unreadable: change nothing
    }

    QTextStream in(&file);
    const QString content = in.readAll();
    const QString title = QFileInfo(path).fileName();

    workspace_.add_document(Document{path.toStdString(), title.toStdString(), content.toStdString()});
    workspace_widget_->add_document(title);

    state_->workspace_changed(*this);
    update_action_states();
    return true;
}

void MainWindow::build_corpus() {
    state_->build_requested(*this);
}

void MainWindow::reset_application() {
    state_->reset_requested(*this);
}

void MainWindow::handle_build_finished(std::uint64_t generation, std::shared_ptr<ProcessingCore> core) {
    state_->build_finished(*this, generation, std::move(core));
}

void MainWindow::handle_build_failed(std::uint64_t generation, const QString& error_message) {
    state_->build_failed(*this, generation, error_message);
}

void MainWindow::exit_application() {
    if (exit_callback_) {
        exit_callback_(this);
    }
}

// Presentation-only events 

void MainWindow::document_selected(int row) {
    if (row >= 0 && row < static_cast<int>(workspace_.documents().size())) {
        const auto& doc = workspace_.documents()[static_cast<std::size_t>(row)];
        workspace_widget_->show_selected_document(QString::fromStdString(doc.title()), QString::fromStdString(doc.text()));
    }
}

void MainWindow::query_changed(const QString& query) {
    (void)query;
    update_action_states();
}

void MainWindow::source_document_requested(const QString& document_id) {
    const std::string id_str = document_id.toStdString();
    const auto& docs = workspace_.documents();
    for (std::size_t i = 0; i < docs.size(); ++i) {
        if (docs[i].id() == id_str) {
            workspace_widget_->select_document(static_cast<int>(i));
            break;
        }
    }
}

// ---- Operations that need a current corpus (the state says whether it is) --

void MainWindow::search_requested(const QString& query, int k) {
    if (!state_->corpus_current()) return;
    auto results = processing_core_.search(query.toStdString(), k);
    search_widget_->set_results(std::move(results));

    context_widget_->clear_context();
    status_label_->setText("Current status: Search completed.");
}

void MainWindow::build_context_requested(int token_budget) {
    if (!state_->corpus_current()) return;
    auto items = processing_core_.build_context(search_widget_->query().toStdString(),
                                                search_widget_->result_count(), token_budget);
    context_widget_->set_context(items);
    status_label_->setText("Current status: Context built.");
}

void MainWindow::term_inspection_requested(const QString& term, const QString& chunk_id) {
    if (!state_->corpus_current() || term.trimmed().isEmpty()) return;

    const std::string term_str = term.toStdString();
    try {
        const std::size_t df = processing_core_.document_frequency(term_str);
        const bool has_chunk = !chunk_id.isEmpty();
        std::size_t tf = 0;
        if (has_chunk) {
            tf = processing_core_.term_frequency(term_str, chunk_id.toStdString());
        }
        term_inspector_widget_->set_statistics(df, has_chunk, tf);
    } catch (...) {
        term_inspector_widget_->clear_statistics();
    }
}

// ---- CorpusContext: the actions the states ask for --------------------------

void MainWindow::enter_state(std::unique_ptr<CorpusState> next) {
    state_ = std::move(next);  // destroys the state that called us
    update_action_states();
}

std::uint64_t MainWindow::start_build() {
    if (workspace_.documents().empty()) return 0;
    const std::uint64_t generation = ++next_generation_;
    status_label_->setText("Current status: Building corpus in background...");
    worker_->submit(BuildRequest{workspace_, generation, build_delay_ms_});  // copy; never waits
    return generation;
}

void MainWindow::cancel_build() {
    worker_->cancel();
}

void MainWindow::install_corpus(std::shared_ptr<ProcessingCore> core) {
    processing_core_ = std::move(*core);
    workspace_widget_->set_corpus_status(QString("Corpus: %1 chunks").arg(processing_core_.chunk_count()));
    clear_processing_outputs();
    status_label_->setText("Current status: Corpus built successfully.");
}

void MainWindow::discard_corpus() {
    workspace_widget_->set_corpus_status("Corpus: Not built");
    clear_processing_outputs();
    update_action_states();
}

void MainWindow::reset_application_view() {
    workspace_.clear();
    Resettable* components[] = {workspace_widget_, search_widget_, term_inspector_widget_, context_widget_};
    for (auto* comp : components) {
        comp->reset();
    }
    status_label_->setText("Current status: Application reset.");
}

void MainWindow::report_status(const QString& text) {
    status_label_->setText("Current status: " + text);
}

// ---- Helpers ----------------------------------------------------------------

void MainWindow::update_action_states() {
    workspace_widget_->set_build_corpus_enabled(!workspace_.documents().empty() && state_->can_request_build());

    const bool can_search = state_->corpus_current() && search_widget_->has_query();
    search_widget_->set_search_enabled(can_search);
    context_widget_->set_build_context_enabled(can_search);

    term_inspector_widget_->set_inspection_enabled(state_->corpus_current());
}

void MainWindow::clear_processing_outputs() {
    search_widget_->clear_results();
    term_inspector_widget_->clear_statistics();
    context_widget_->clear_context();
}

}  // namespace aiws::gui