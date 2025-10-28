#pragma once

#include <QList>
#include <QObject>
#include <QPair>
#include <QPointF>
#include <QRectF>
#include <QString>

/**
 * @brief Handles multi-page text selection (Feature 13)
 */
class MultiPageTextSelection : public QObject {
    Q_OBJECT

public:
    struct PageTextRange {
        int pageNumber;
        int startIndex;
        int endIndex;
        QList<QRectF> rects;
        QString text;
    };

    explicit MultiPageTextSelection(QObject* parent = nullptr);
    ~MultiPageTextSelection() = default;

    void startSelection(int pageNumber, const QPointF& point);
    void updateSelection(int pageNumber, const QPointF& point);
    void finishSelection();
    void clearSelection();

    bool hasSelection() const { return !m_selectedRanges.isEmpty(); }
    QString getSelectedText() const;
    QList<PageTextRange> getSelectedRanges() const { return m_selectedRanges; }

signals:
    void selectionChanged();
    void selectionFinished(const QString& text);

private:
    int m_startPage;
    int m_endPage;
    QPointF m_startPoint;
    QPointF m_endPoint;
    QList<PageTextRange> m_selectedRanges;
    bool m_isSelecting;
};
