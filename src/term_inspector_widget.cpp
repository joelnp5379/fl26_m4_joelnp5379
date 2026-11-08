//Tells the user the TF and DF
#include "aiws/gui/term_inspector_widget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QString>

namespace aiws::gui {
//TermInspectorWidget creation
TermInspectorWidget::TermInspectorWidget(QWidget* parent) : QWidget(parent) {
    term_edit_ = new QLineEdit(this);
    term_edit_->setPlaceholderText("Enter a term");
    
    inspect_button_ = new QPushButton("Inspect", this);
    inspect_button_->setEnabled(false);
    
    document_frequency_value_ = new QLabel("–", this);
    chunk_frequency_value_ = new QLabel("–", this);

    auto* top_layout = new QHBoxLayout();
    top_layout->addWidget(new QLabel("<b>Term Inspector:</b>", this));
    top_layout->addStretch();

    auto* mid_layout = new QHBoxLayout();
    mid_layout->addWidget(new QLabel("Term:", this));
    mid_layout->addWidget(term_edit_);
    mid_layout->addWidget(inspect_button_);

    auto* bottom_layout = new QHBoxLayout();
    bottom_layout->addWidget(new QLabel("Document Frequency:", this));
    bottom_layout->addWidget(document_frequency_value_);
    bottom_layout->addStretch();
    bottom_layout->addWidget(new QLabel("Selected Chunk TF:", this));
    bottom_layout->addWidget(chunk_frequency_value_);
    bottom_layout->addStretch();

    auto* main_layout = new QVBoxLayout(this);
    main_layout->addLayout(top_layout);
    main_layout->addLayout(mid_layout);
    main_layout->addLayout(bottom_layout);
    main_layout->addStretch();

    connect(inspect_button_, &QPushButton::clicked,
            this, &TermInspectorWidget::request_inspection);
            
    selected_chunk_id_ = "";
}

void TermInspectorWidget::set_inspection_enabled(bool enabled) {
    inspect_button_->setEnabled(enabled);
}
//Remember the selected chunk for term inspection
void TermInspectorWidget::set_selected_chunk(const QString& chunk_id) {
    selected_chunk_id_ = chunk_id;
}
//Convert the unsigned ints into text
void TermInspectorWidget::set_statistics(std::size_t document_frequency,
                                         bool has_chunk_frequency,
                                         std::size_t chunk_frequency) {

    document_frequency_value_->setText(QString::number(document_frequency));
    
    if (has_chunk_frequency) {
        chunk_frequency_value_->setText(QString::number(chunk_frequency));
    } else {
        chunk_frequency_value_->setText("–");
    }
}

void TermInspectorWidget::clear_statistics() {
    document_frequency_value_->setText("–");
    chunk_frequency_value_->setText("–");
}

void TermInspectorWidget::reset() {
    term_edit_->clear();
    selected_chunk_id_ = "";
    clear_statistics();
    set_inspection_enabled(false);
}
//When the Inspect button is clicked, package the word and the chunk id and send to mainwindow.
void TermInspectorWidget::request_inspection() {
    QString term = term_edit_->text();
    emit term_inspection_requested(term, selected_chunk_id_);
}

}  // namespace aiws::gui