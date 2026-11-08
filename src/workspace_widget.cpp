#include "aiws/gui/workspace_widget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace aiws::gui {
//WorkspaceWidget creation
WorkspaceWidget::WorkspaceWidget(QWidget* parent) : QWidget(parent) {
    document_list_ = new QListWidget(this);
    add_document_button_ = new QPushButton("Add Document", this);
    build_corpus_button_ = new QPushButton("Build Corpus", this);
    corpus_status_label_ = new QLabel("Corpus: Not built", this);
    
    document_title_label_ = new QLabel("Select a document to view its contents.", this);
    document_text_ = new QPlainTextEdit(this);
    document_text_->setReadOnly(true);

    auto* left_layout = new QVBoxLayout();
    auto* left_header = new QHBoxLayout();
    left_header->addWidget(new QLabel("<b>Workspace</b>", this));
    left_header->addWidget(add_document_button_);
    left_header->addWidget(build_corpus_button_);
    left_header->addStretch();

    left_layout->addLayout(left_header);
    left_layout->addWidget(document_list_);
    left_layout->addWidget(corpus_status_label_);

    auto* right_layout = new QVBoxLayout();
    auto* right_header = new QHBoxLayout();
    right_header->addWidget(new QLabel("<b>Selected Document -</b>", this));
    right_header->addWidget(document_title_label_);
    right_header->addStretch();

    right_layout->addLayout(right_header);
    right_layout->addWidget(document_text_);

    auto* main_layout = new QHBoxLayout(this);
    main_layout->addLayout(left_layout, 1);
    main_layout->addLayout(right_layout, 2);

    //Connect signals to slots for user interactions
    connect(add_document_button_, &QPushButton::clicked,
            this, &WorkspaceWidget::add_document_requested);
            
    connect(build_corpus_button_, &QPushButton::clicked,
            this, &WorkspaceWidget::build_corpus_requested);
            
    connect(document_list_, &QListWidget::currentRowChanged,
            this, &WorkspaceWidget::document_selected);

    build_corpus_button_->setEnabled(false);
}
//Setters and getters for document list, selected document, corpus status, and build corpus button
void WorkspaceWidget::add_document(const QString& title) {
    document_list_->addItem(title);
}

void WorkspaceWidget::select_document(int row) {
    document_list_->setCurrentRow(row);
}

void WorkspaceWidget::show_selected_document(const QString& title, const QString& text) {
    document_title_label_->setText(title);
    document_text_->setPlainText(text);
}

void WorkspaceWidget::clear_selected_document() {
    document_title_label_->setText("Select a document to view its contents.");
    document_text_->clear();
}

void WorkspaceWidget::set_corpus_status(const QString& text) {
    corpus_status_label_->setText(text);
}

void WorkspaceWidget::set_build_corpus_enabled(bool enabled) {
    build_corpus_button_->setEnabled(enabled);
}
//Reset implementation to restore the workspace to its initial state
void WorkspaceWidget::reset() {
    document_list_->clear();
    clear_selected_document();
    set_corpus_status("Corpus: Not built");
    set_build_corpus_enabled(false);
}

}  // namespace aiws::gui