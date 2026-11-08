//Manages query inputs, data table, and result inspection box.
#include "aiws/gui/search_widget.hpp"
#include "aiws/gui/search_result_model.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QVBoxLayout>
#include <QHeaderView>

namespace aiws::gui {

//Establish Model/View and highlight the entire row.
SearchWidget::SearchWidget(QWidget* parent) : QWidget(parent) {
    result_model_ = new SearchResultModel(this);
    
    query_edit_ = new QLineEdit(this);
    query_edit_->setPlaceholderText("Search the current corpus");
    
    result_count_spin_ = new QSpinBox(this);
    result_count_spin_->setRange(1, 100);
    result_count_spin_->setValue(3);
    
    search_button_ = new QPushButton("Search", this);
    search_button_->setEnabled(false);
    
    results_view_ = new QTableView(this);
    results_view_->setModel(result_model_);
    results_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    results_view_->setSelectionMode(QAbstractItemView::SingleSelection);
    results_view_->horizontalHeader()->setStretchLastSection(true);
    
    selected_result_text_ = new QPlainTextEdit(this);
    selected_result_text_->setReadOnly(true);
    
    view_source_button_ = new QPushButton("View Source", this);
    view_source_button_->setEnabled(false);

    auto* top_layout = new QHBoxLayout();
    top_layout->addWidget(new QLabel("<b>Query:</b>", this));
    top_layout->addWidget(query_edit_, 1);
    top_layout->addWidget(new QLabel("Results:", this));
    top_layout->addWidget(result_count_spin_);
    top_layout->addWidget(search_button_);

    auto* bottom_layout = new QHBoxLayout();
    bottom_layout->addWidget(new QLabel("<b>Selected Result:</b>", this));
    bottom_layout->addStretch();
    bottom_layout->addWidget(view_source_button_);

    auto* main_layout = new QVBoxLayout(this);
    main_layout->addLayout(top_layout);
    main_layout->addWidget(results_view_, 1);
    main_layout->addLayout(bottom_layout);
    main_layout->addWidget(selected_result_text_, 1);

    //Connect signals to slots
    connect(query_edit_, &QLineEdit::textChanged,
            this, &SearchWidget::query_changed);
            
    connect(search_button_, &QPushButton::clicked,
            this, &SearchWidget::request_search);
            
    connect(results_view_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &SearchWidget::show_selected_result);
            
    connect(view_source_button_, &QPushButton::clicked,
            this, &SearchWidget::request_source_document);
}

QString SearchWidget::query() const {
    return query_edit_->text();
}

int SearchWidget::result_count() const {
    return result_count_spin_->value();
}

bool SearchWidget::has_query() const {
    return !query_edit_->text().trimmed().isEmpty();
}

void SearchWidget::set_search_enabled(bool enabled) {
    search_button_->setEnabled(enabled);
}

void SearchWidget::set_results(std::vector<SearchResult> results) {
    result_model_->set_results(std::move(results));
    results_view_->clearSelection();
}
//Wipe the restults and reset the view
void SearchWidget::clear_results() {
    result_model_->clear();
    results_view_->clearSelection();
    selected_result_text_->clear();
    view_source_button_->setEnabled(false);
}

void SearchWidget::reset() {
    query_edit_->clear();
    result_count_spin_->setValue(3);
    clear_results();
    set_search_enabled(false);
}
//Custom signal for search request
void SearchWidget::request_search() {
    emit search_requested(query(), result_count());
}
//Custom signal for showing selected result
void SearchWidget::show_selected_result() {
    auto indexes = results_view_->selectionModel()->selectedRows();
    if (indexes.isEmpty()) {
        selected_result_text_->clear();
        view_source_button_->setEnabled(false);
        emit selected_chunk_changed(""); // Notify Term Inspector that selection cleared
        return;
    }
    //Asks for SearchResult at that selected index.
    int row = indexes.first().row();
    const SearchResult* res = result_model_->result_at(row);
    if (res) {
        QString details = QString("Document: %1\nChunk: %2\nScore: %3\n\n%4")
            .arg(QString::fromStdString(res->document_id))
            .arg(res->chunk_sequence)
            .arg(res->score)
            .arg(QString::fromStdString(res->text));
            
        selected_result_text_->setPlainText(details);
        view_source_button_->setEnabled(true);
        
        emit selected_chunk_changed(QString::fromStdString(res->chunk_id));
    }
}
//Custom signal for source document request
void SearchWidget::request_source_document() {
    auto indexes = results_view_->selectionModel()->selectedRows();
    if (!indexes.isEmpty()) {
        const SearchResult* res = result_model_->result_at(indexes.first().row());
        if (res) {
            emit source_document_requested(QString::fromStdString(res->document_id));
        }
    }
}

}  // namespace aiws::gui