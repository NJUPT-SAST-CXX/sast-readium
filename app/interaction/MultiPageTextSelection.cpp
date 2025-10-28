#include "MultiPageTextSelection.h"

MultiPageTextSelection::MultiPageTextSelection(QObject* parent)
    : QObject(parent), m_startPage(-1), m_endPage(-1), m_isSelecting(false) {}

void MultiPageTextSelection::startSelection(int pageNumber,
                                            const QPointF& point) {
    m_isSelecting = true;
    m_startPage = pageNumber;
    m_endPage = pageNumber;
    m_startPoint = point;
    m_endPoint = point;
    m_selectedRanges.clear();
}

void MultiPageTextSelection::updateSelection(int pageNumber,
                                             const QPointF& point) {
    if (!m_isSelecting) {
        return;
    }

    m_endPage = pageNumber;
    m_endPoint = point;

    // Rebuild selected ranges across pages
    m_selectedRanges.clear();

    int startPage = qMin(m_startPage, m_endPage);
    int endPage = qMax(m_startPage, m_endPage);

    for (int page = startPage; page <= endPage; ++page) {
        PageTextRange range;
        range.pageNumber = page;
        // Would populate with actual text extraction
        m_selectedRanges.append(range);
    }

    emit selectionChanged();
}

void MultiPageTextSelection::finishSelection() {
    if (!m_isSelecting) {
        return;
    }

    m_isSelecting = false;
    QString selectedText = getSelectedText();
    emit selectionFinished(selectedText);
}

void MultiPageTextSelection::clearSelection() {
    m_selectedRanges.clear();
    m_isSelecting = false;
    m_startPage = -1;
    m_endPage = -1;
    emit selectionChanged();
}

QString MultiPageTextSelection::getSelectedText() const {
    QString result;
    for (const PageTextRange& range : m_selectedRanges) {
        result += range.text;
        if (&range != &m_selectedRanges.last()) {
            result += "\n";
        }
    }
    return result;
}
