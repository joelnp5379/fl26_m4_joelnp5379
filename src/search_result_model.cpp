#include "aiws/gui/search_result_model.hpp"

namespace aiws::gui {
//Build the table
SearchResultModel::SearchResultModel(QObject* parent)
    : QAbstractTableModel(parent) {
}

int SearchResultModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(results_.size());
}

int SearchResultModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return 5;
}
//Provide data for each cell in the table
QVariant SearchResultModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(results_.size())) { //Check for valid index
        return QVariant();
    }

    if (role == Qt::DisplayRole) { //Role filtering for display data
        const auto& res = results_[index.row()]; //Grab search results
        switch (index.column()) { //Switch between the columns to display the appropriate data
            case 0: return index.row() + 1;
            case 1: return QString::fromStdString(res.document_id);
            case 2: return static_cast<qulonglong>(res.chunk_sequence);
            case 3: return res.score;
            case 4: return QString::fromStdString(res.text);
            default: return QVariant();
        }
    }
    return QVariant();
}
//Provide header data for the table
QVariant SearchResultModel::headerData(int section,
                                       Qt::Orientation orientation,
                                       int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) { //Use switch statement to return the appropriate header for each column
            case 0: return "Rank";
            case 1: return "Document";
            case 2: return "Chunk";
            case 3: return "Score";
            case 4: return "Text";
            default: return QVariant();
        }
    }
    return QVariant();
}
//Model updates
void SearchResultModel::set_results(std::vector<SearchResult> results) {
    beginResetModel();
    results_ = std::move(results);
    endResetModel();
}

void SearchResultModel::clear() {
    beginResetModel();
    results_.clear();
    endResetModel();
}
//Helper function to get the SearchResult at a specific row
const SearchResult* SearchResultModel::result_at(int row) const noexcept {
    if (row >= 0 && row < static_cast<int>(results_.size())) {
        return &results_[row];
    }
    return nullptr;
}

}  // namespace aiws::gui