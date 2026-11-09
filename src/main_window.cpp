#include "aiws/gui/main_window.hpp"
#include "aiws/gui/workspace_widget.hpp"
#include "aiws/gui/search_widget.hpp"
#include "aiws/gui/term_inspector_widget.hpp"
#include "aiws/gui/context_widget.hpp"
#include "aiws/ipc_server.hpp"
#include <unistd.h>

#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

namespace aiws::gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    //Create Central Widget and Vertical Layout
    auto* central_widget = new QWidget(this);
    setCentralWidget(central_widget);
    auto* main_layout = new QVBoxLayout(central_widget);

    auto* header_label = new QLabel("<b>AI Workspace</b><br><i>Document Search and Context Explorer</i>", this);
    header_label->setStyleSheet("background-color: #7B1FA2; color: white; padding: 10px; font-size: 16px;");
    
    //Widget Creation
    workspace_widget_ = new WorkspaceWidget(this);
    search_widget_ = new SearchWidget(this);
    term_inspector_widget_ = new TermInspectorWidget(this);
    context_widget_ = new ContextWidget(this);

    //Bottom layout creation with status buttons
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

    //Horizontal layout for term inspector and context widget
    auto* tools_layout = new QHBoxLayout();
    tools_layout->addWidget(term_inspector_widget_, 1);
    tools_layout->addWidget(context_widget_, 1);

    //Add widgets to the main layout
    main_layout->addWidget(header_label);
    main_layout->addWidget(workspace_widget_, 1);
    main_layout->addWidget(search_widget_, 2);
    main_layout->addLayout(tools_layout, 1);
    main_layout->addLayout(bottom_layout);

    //Connect signals and slots
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

void MainWindow::set_exit_callback(ExitCallback callback) noexcept {
    exit_callback_ = callback;
}
//Add document implementation
void MainWindow::add_document() {
    QString file_path = QFileDialog::getOpenFileName(this, "Select Document", "", "Text Files (*.txt);;All Files (*)");
    if (!file_path.isEmpty()) {
        add_document_file(file_path);
    }
}
//Build corpus implementation bridge between GUI and ProcessingCore
void MainWindow::build_corpus() {
    status_label_->setText("Current status: Building corpus in background...");
    
    int captured_version = workspace_version_.load();
    
    Workspace workspace_copy = workspace_;
    int delay_per_doc = build_delay_ms_;
    
    if (build_thread_.joinable()) {
        build_thread_.join();
    }
    
    build_thread_ = std::thread([this, workspace_copy, captured_version, delay_per_doc]() {
        try {
            // Build the corpus locally inside the thread
            auto local_core = std::make_shared<ProcessingCore>();
            local_core->rebuild(workspace_copy);
            
            // Simulate the slow build required by M4 (N ms per document)
            if (delay_per_doc > 0) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(delay_per_doc * workspace_copy.documents().size())
                );
            }
            
            QMetaObject::invokeMethod(this, [this, local_core, captured_version]() {
                handle_build_finished(local_core, captured_version);
            }, Qt::QueuedConnection);
            
        } catch (const std::exception& e) {
            // Send failure back to GUI thread
            QString error_msg = QString::fromStdString(e.what());
            QMetaObject::invokeMethod(this, [this, error_msg]() {
                handle_build_failed(error_msg);
            }, Qt::QueuedConnection);
        }
    });
}
//Document selected implementation
void MainWindow::document_selected(int row) {
    if (row >= 0 && row < static_cast<int>(workspace_.documents().size())) {
        const auto& doc = workspace_.documents()[row];
        workspace_widget_->show_selected_document(QString::fromStdString(doc.title()), QString::fromStdString(doc.text()));
    }
}
//Search requested implementation
void MainWindow::search_requested(const QString& query, int k) {
    if (!corpus_current_) return;
    //gives results to search widget and clears context widget
    auto results = processing_core_.search(query.toStdString(), k);
    search_widget_->set_results(std::move(results));
    
    context_widget_->clear_context();
    status_label_->setText("Current status: Search completed.");
}
//Query changed implementation
void MainWindow::query_changed(const QString& query) {
    (void)query;
    update_action_states();
}
//Build context requested implementation
void MainWindow::build_context_requested(int token_budget) {
    if (!corpus_current_) return;
    //Ask processing core to build the context
    auto items = processing_core_.build_context(search_widget_->query().toStdString(), search_widget_->result_count(), token_budget);
    context_widget_->set_context(items);
    status_label_->setText("Current status: Context built.");
}
//Source document requested implementation
void MainWindow::source_document_requested(const QString& document_id) {
    std::string id_str = document_id.toStdString(); //Get the doc ID and find the index in the workspace.
    const auto& docs = workspace_.documents();
    for (std::size_t i = 0; i < docs.size(); ++i) {
        if (docs[i].id() == id_str) {
            workspace_widget_->select_document(static_cast<int>(i));
            break;
        }
    }
}
//Term inspection requested implementation
void MainWindow::term_inspection_requested(const QString& term, const QString& chunk_id) {
    if (!corpus_current_ || term.trimmed().isEmpty()) return;
    
    std::string term_str = term.toStdString();
    try {
        std::size_t df = processing_core_.document_frequency(term_str);
        bool has_chunk = !chunk_id.isEmpty();
        std::size_t tf = 0;
        
        if (has_chunk) {
            tf = processing_core_.term_frequency(term_str, chunk_id.toStdString());
        }
        
        term_inspector_widget_->set_statistics(df, has_chunk, tf);
    } catch (...) {
        term_inspector_widget_->clear_statistics();
    }
}
//Reset application implementation
void MainWindow::reset_application() {
    workspace_.clear();
    corpus_current_ = false;
    
    Resettable* components[] = {workspace_widget_, search_widget_, term_inspector_widget_, context_widget_};
    for (auto* comp : components) {
        comp->reset();
    }
    
    status_label_->setText("Current status: Application reset.");
    update_action_states();
}
//Exit application implementation
void MainWindow::exit_application() {
    if (exit_callback_) {
        exit_callback_(this);
    }
}
//Update action states implementation turn the UI buttons on or off based on current state
void MainWindow::update_action_states() {
    workspace_widget_->set_build_corpus_enabled(!workspace_.documents().empty());
    
    bool can_search = corpus_current_ && search_widget_->has_query();
    search_widget_->set_search_enabled(can_search);
    context_widget_->set_build_context_enabled(can_search);
    
    term_inspector_widget_->set_inspection_enabled(corpus_current_);
}
//Invalidate corpus implementation
void MainWindow::invalidate_corpus() {
    corpus_current_ = false;
    workspace_version_++; // Increment version on every change
    workspace_widget_->set_corpus_status("Corpus: Not built");
    clear_processing_outputs();
}
//Clear processing outputs implementation
void MainWindow::clear_processing_outputs() { //Clear search results, term statistics, and context display
    search_widget_->clear_results();
    term_inspector_widget_->clear_statistics();
    context_widget_->clear_context();
}

void MainWindow::set_build_delay_ms(int ms_per_document) {
    build_delay_ms_ = ms_per_document;
}

bool MainWindow::add_document_file(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false; // Return false if unreadable
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    
    QFileInfo file_info(path);
    QString title = file_info.fileName();
    
    workspace_.add_document(Document{path.toStdString(), title.toStdString(), content.toStdString()});
    
    workspace_widget_->add_document(title);
    invalidate_corpus();
    update_action_states();
    
    return true;
}

MainWindow::~MainWindow() {
    //if (ipc_server_) ipc_server_->stop();

    cancel_build_ = true; // Signal any active background work to stop
    if (build_thread_.joinable()) {
        build_thread_.join();
    }
}

void MainWindow::handle_build_finished(std::shared_ptr<ProcessingCore> new_core, int completed_version) {
#ifndef AIWS_FAULT_ACCEPT_STALE
    if (completed_version != workspace_version_.load()) {
        status_label_->setText("Current status: Build finished, but workspace changed. Corpus discarded.");
        return;
    }
#endif

    processing_core_ = std::move(*new_core);
    corpus_current_ = true;
    workspace_widget_->set_corpus_status(QString("Corpus: %1 chunks").arg(processing_core_.chunk_count()));
    clear_processing_outputs();
    status_label_->setText("Current status: Corpus built successfully.");
    update_action_states();
}

void MainWindow::handle_build_failed(const QString& error_message) {
#ifndef AIWS_FAULT_LOSE_FAILURE
    status_label_->setText(QString("Current status: Rebuild failed - %1").arg(error_message));
    corpus_current_ = false;
    update_action_states();
#endif
}

}  // namespace aiws::gui