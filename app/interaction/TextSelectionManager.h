#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QClipboard>
#include <QColor>
#include <QGuiApplication>
#include <QList>
#include <QObject>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

struct TextBox {
    QRectF rect;
    QString text;
    int charIndex;
    Poppler::Page* page;

    TextBox() : charIndex(-1), page(nullptr) {}
    TextBox(const QRectF& r, const QString& t, int idx, Poppler::Page* p)
        : rect(r), text(t), charIndex(idx), page(p) {}

    bool contains(const QPointF& point) const { return rect.contains(point); }
};

struct TextSelection {
    QPointF startPoint;
    QPointF endPoint;
    int startCharIndex;
    int endCharIndex;
    QList<QRectF> rects;
    QString text;
    int pageNumber;

    TextSelection() : startCharIndex(-1), endCharIndex(-1), pageNumber(-1) {}
    bool isEmpty() const { return startCharIndex == -1 || endCharIndex == -1; }
    void clear() {
        startPoint = QPointF();
        endPoint = QPointF();
        startCharIndex = -1;
        endCharIndex = -1;
        rects.clear();
        text.clear();
        pageNumber = -1;
    }
    void normalize() {
        if (startCharIndex > endCharIndex) {
            std::swap(startCharIndex, endCharIndex);
            std::swap(startPoint, endPoint);
        }
    }
};

class TextSelectionManager : public QObject {
    Q_OBJECT

public:
    explicit TextSelectionManager(QObject* parent = nullptr);
    ~TextSelectionManager();

    void setPage(Poppler::Page* page, int pageNumber);
    void clearPage();
    bool hasPage() const { return m_currentPage != nullptr; }

    bool extractTextBoxes();
    QList<TextBox> getTextBoxes() const { return m_textBoxes; }
    QString getPageText() const { return m_pageText; }

    void startSelection(const QPointF& point);
    void updateSelection(const QPointF& point);
    void endSelection();
    void clearSelection();
    bool hasSelection() const { return !m_selection.isEmpty(); }

    TextSelection getSelection() const { return m_selection; }
    QString getSelectedText() const { return m_selection.text; }
    QList<QRectF> getSelectionRects() const { return m_selection.rects; }

    void copySelectionToClipboard();
    bool canCopy() const {
        return hasSelection() && !m_selection.text.isEmpty();
    }

    void renderSelection(QPainter& painter, double scaleFactor);
    void setSelectionColor(const QColor& color) { m_selectionColor = color; }
    QColor getSelectionColor() const { return m_selectionColor; }

    int findCharacterAtPoint(const QPointF& point) const;
    TextBox findTextBoxAtPoint(const QPointF& point) const;

    void selectWordAt(const QPointF& point);
    void selectLineAt(const QPointF& point);
    void selectAll();

    void setScaleFactor(double scale) { m_scaleFactor = scale; }
    double getScaleFactor() const { return m_scaleFactor; }

signals:
    void selectionChanged();
    void selectionCleared();
    void textCopied(const QString& text);
    void selectionError(const QString& error);

private:
    void calculateSelectionRects();
    void extractSelectedText();
    QList<QRectF> getCharacterRects(int startIdx, int endIdx);
    bool isPointInTextBox(const QPointF& point, const TextBox& box) const;
    void findWordBoundaries(int charIndex, int& start, int& end);
    void findLineBoundaries(int charIndex, int& start, int& end);
    void analyzeTextLayout();
    bool isNewLine(int charIndex) const;
    bool isWordBoundary(int charIndex) const;

    Poppler::Page* m_currentPage;
    int m_pageNumber;
    QList<TextBox> m_textBoxes;
    QString m_pageText;
    QVector<QRectF> m_charRects;
    double m_scaleFactor;

    TextSelection m_selection;
    bool m_isSelecting;
    QPointF m_selectionStartPoint;

    QColor m_selectionColor;

    QVector<bool> m_lineBreaks;
    QVector<bool> m_wordBreaks;

    bool m_textBoxesExtracted;
    bool m_layoutAnalyzed;
};
