#include "aiws/gui/context_widget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QString>

namespace aiws::gui {
//Sets up widget and slider
ContextWidget::ContextWidget(QWidget* parent) : QWidget(parent) {
    context_budget_slider_ = new QSlider(Qt::Horizontal, this);
    context_budget_slider_->setRange(50, 1000);
    context_budget_slider_->setSingleStep(50);
    context_budget_slider_->setValue(300);
    
    budget_value_label_ = new QLabel("300", this);
    
    build_context_button_ = new QPushButton("Build Context", this);
    build_context_button_->setEnabled(false);
    
    context_text_ = new QPlainTextEdit(this);
    context_text_->setReadOnly(true);
    context_text_->setPlaceholderText("Built context will appear here.");

    auto* top_layout = new QHBoxLayout();
    top_layout->addWidget(new QLabel("<b>Context:</b>", this));
    top_layout->addStretch();
    
    auto* mid_layout = new QHBoxLayout();
    mid_layout->addWidget(new QLabel("Token budget:", this));
    mid_layout->addWidget(context_budget_slider_);
    mid_layout->addWidget(budget_value_label_);
    mid_layout->addWidget(build_context_button_);

    auto* main_layout = new QVBoxLayout(this);
    main_layout->addLayout(top_layout);
    main_layout->addLayout(mid_layout);
    main_layout->addWidget(context_text_);
    
    connect(context_budget_slider_, &QSlider::valueChanged,
            this, &ContextWidget::budget_changed);
            
    connect(build_context_button_, &QPushButton::clicked,
            this, &ContextWidget::request_context);
}

int ContextWidget::token_budget() const {
    return context_budget_slider_->value();
}

void ContextWidget::set_build_context_enabled(bool enabled) {
    build_context_button_->setEnabled(enabled);
}
//Sets the context text area with the provided context items
void ContextWidget::set_context(const std::vector<ContextItem>& items) {
    QString output;
    if (items.empty()) {
        output = "No context generated.";
    } else {
        for (const auto& item : items) {
            output += QString("Document: %1\nChunk: %2\nTokens: %3\nScore: %4\nTruncated: %5\n\n%6\n\n-----------------\n\n")
                        .arg(QString::fromStdString(item.document_id))
                        .arg(item.chunk_sequence)
                        .arg(item.token_count)
                        .arg(item.score)
                        .arg(item.truncated ? "Yes" : "No")
                        .arg(QString::fromStdString(item.text));
        }
    }
    context_text_->setPlainText(output);
}

void ContextWidget::clear_context() {
    context_text_->clear();
}

void ContextWidget::reset() {
    context_budget_slider_->setValue(300);
    budget_value_label_->setText("300");
    clear_context();
    set_build_context_enabled(false);
}
//When the Build Context button is clicked, emit the signal to request context building
void ContextWidget::request_context() {
    emit build_context_requested(token_budget());
}
//Updates the budget label when the slider changes
void ContextWidget::budget_changed(int value) {

    budget_value_label_->setText(QString::number(value));
}

}  // namespace aiws::gui
