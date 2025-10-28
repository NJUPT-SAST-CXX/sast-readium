#include "TextSelectionManager.h"
#include <QApplication>
#include <QClipboard>
#include <algorithm>

TextSelectionManager::TextSelectionManager(QObject* parent)
    : QObject(parent),
      m_currentPage(nullptr),
      m_pageNumber(-1),
      m_scaleFactor(1.0),
      m_isSelecting(false),
      m_selectionColor(QColor(0, 120, 215, 100)),
      m_textBoxesExtracted(false),
      m_layoutAnalyzed(false) {}

TextSelectionManager::~TextSelectionManager() { clearPage(); }

void TextSelectionManager::setPage(Poppler::Page* page, int pageNumber) {
    if (m_currentPage == page && m_pageNumber == pageNumber)
        return;
    clearPage();
    m_currentPage = page;
    m_pageNumber = pageNumber;
    m_textBoxesExtracted = false;
    m_layoutAnalyzed = false;
}

void TextSelectionManager::clearPage() {
    clearSelection();
    m_textBoxes.clear();
    m_charRects.clear();
    m_lineBreaks.clear();
    m_wordBreaks.clear();
    m_pageText.clear();
    m_currentPage = nullptr;
    m_pageNumber = -1;
    m_textBoxesExtracted = false;
    m_layoutAnalyzed = false;
}

bool TextSelectionManager::extractTextBoxes() {
    if (!m_currentPage) {
        emit selectionError("No page set for text extraction");
        return false;
    }
    if (m_textBoxesExtracted)
        return true;

    m_textBoxes.clear();
    m_charRects.clear();
    m_pageText.clear();

    QList<Poppler::TextBox*> popplerTextBoxes = m_currentPage->textList();
    int charIndex = 0;
    for (Poppler::TextBox* box : popplerTextBoxes) {
        if (box) {
            QString text = box->text();
            QRectF rect = box->boundingBox();
            for (int i = 0; i < text.length(); ++i) {
                TextBox textBox(rect, QString(text[i]), charIndex,
                                m_currentPage);
                m_textBoxes.append(textBox);
                m_charRects.append(rect);
                m_pageText.append(text[i]);
                charIndex++;
            }
            delete box;
        }
    }

    m_textBoxesExtracted = true;
    analyzeTextLayout();
    return true;
}

void TextSelectionManager::analyzeTextLayout() {
    if (m_layoutAnalyzed || m_textBoxes.isEmpty())
        return;

    m_lineBreaks.resize(m_textBoxes.size());
    m_wordBreaks.resize(m_textBoxes.size());
    m_lineBreaks.fill(false);
    m_wordBreaks.fill(false);

    for (int i = 1; i < m_textBoxes.size(); ++i) {
        const QRectF& prevRect = m_textBoxes[i - 1].rect;
        const QRectF& currRect = m_textBoxes[i].rect;
        double yDiff = qAbs(currRect.top() - prevRect.top());
        double lineHeight = qMax(prevRect.height(), currRect.height());
        if (yDiff > lineHeight * 0.5)
            m_lineBreaks[i] = true;

        QChar prevChar = m_pageText[i - 1];
        QChar currChar = m_pageText[i];
        if (prevChar.isSpace() || currChar.isSpace() || prevChar.isPunct() ||
            currChar.isPunct()) {
            m_wordBreaks[i] = true;
        }
    }
    m_layoutAnalyzed = true;
}

void TextSelectionManager::startSelection(const QPointF& point) {
    if (!m_currentPage)
        return;
    if (!m_textBoxesExtracted)
        extractTextBoxes();

    m_isSelecting = true;
    m_selectionStartPoint = point;
    m_selection.clear();
    m_selection.startPoint = point;
    m_selection.pageNumber = m_pageNumber;
    int charIdx = findCharacterAtPoint(point);
    if (charIdx >= 0)
        m_selection.startCharIndex = charIdx;
}

void TextSelectionManager::updateSelection(const QPointF& point) {
    if (!m_isSelecting || !m_currentPage)
        return;

    m_selection.endPoint = point;
    int charIdx = findCharacterAtPoint(point);
    if (charIdx >= 0) {
        m_selection.endCharIndex = charIdx;
        calculateSelectionRects();
        extractSelectedText();
        emit selectionChanged();
    }
}

void TextSelectionManager::endSelection() {
    m_isSelecting = false;
    if (!m_selection.isEmpty()) {
        m_selection.normalize();
        calculateSelectionRects();
        extractSelectedText();
        emit selectionChanged();
    }
}

void TextSelectionManager::clearSelection() {
    bool hadSelection = !m_selection.isEmpty();
    m_selection.clear();
    m_isSelecting = false;
    if (hadSelection)
        emit selectionCleared();
}

int TextSelectionManager::findCharacterAtPoint(const QPointF& point) const {
    for (int i = 0; i < m_textBoxes.size(); ++i) {
        if (m_textBoxes[i].contains(point))
            return i;
    }
    return -1;
}

TextBox TextSelectionManager::findTextBoxAtPoint(const QPointF& point) const {
    int idx = findCharacterAtPoint(point);
    if (idx >= 0 && idx < m_textBoxes.size())
        return m_textBoxes[idx];
    return TextBox();
}

void TextSelectionManager::calculateSelectionRects() {
    m_selection.rects.clear();
    if (m_selection.isEmpty() || m_textBoxes.isEmpty())
        return;

    int start = qMin(m_selection.startCharIndex, m_selection.endCharIndex);
    int end = qMax(m_selection.startCharIndex, m_selection.endCharIndex);
    if (start < 0 || end >= m_textBoxes.size())
        return;

    QRectF currentLineRect;
    for (int i = start; i <= end; ++i) {
        const QRectF& rect = m_textBoxes[i].rect;
        bool isNewLine = (i > start) && m_lineBreaks[i];
        if (isNewLine || currentLineRect.isNull()) {
            if (!currentLineRect.isNull())
                m_selection.rects.append(currentLineRect);
            currentLineRect = rect;
        } else {
            currentLineRect = currentLineRect.united(rect);
        }
    }
    if (!currentLineRect.isNull())
        m_selection.rects.append(currentLineRect);
}

void TextSelectionManager::extractSelectedText() {
    m_selection.text.clear();
    if (m_selection.isEmpty())
        return;

    int start = qMin(m_selection.startCharIndex, m_selection.endCharIndex);
    int end = qMax(m_selection.startCharIndex, m_selection.endCharIndex);
    if (start < 0 || end >= m_pageText.length())
        return;

    for (int i = start; i <= end; ++i) {
        m_selection.text.append(m_pageText[i]);
        if (i < end && i + 1 < m_lineBreaks.size() && m_lineBreaks[i + 1]) {
            m_selection.text.append(QChar::LineFeed);
        }
    }
}

void TextSelectionManager::copySelectionToClipboard() {
    if (!canCopy())
        return;
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setText(m_selection.text);
        emit textCopied(m_selection.text);
    }
}

void TextSelectionManager::renderSelection(QPainter& painter,
                                           double scaleFactor) {
    if (m_selection.isEmpty())
        return;

    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_selectionColor);
    for (const QRectF& rect : m_selection.rects) {
        QRectF scaledRect(rect.x() * scaleFactor, rect.y() * scaleFactor,
                          rect.width() * scaleFactor,
                          rect.height() * scaleFactor);
        painter.drawRect(scaledRect);
    }
    painter.restore();
}

void TextSelectionManager::selectWordAt(const QPointF& point) {
    if (!m_currentPage)
        return;
    if (!m_textBoxesExtracted)
        extractTextBoxes();
    int charIdx = findCharacterAtPoint(point);
    if (charIdx < 0)
        return;
    int start, end;
    findWordBoundaries(charIdx, start, end);
    m_selection.clear();
    m_selection.startCharIndex = start;
    m_selection.endCharIndex = end;
    m_selection.pageNumber = m_pageNumber;
    m_selection.startPoint = m_textBoxes[start].rect.topLeft();
    m_selection.endPoint = m_textBoxes[end].rect.bottomRight();
    calculateSelectionRects();
    extractSelectedText();
    emit selectionChanged();
}

void TextSelectionManager::selectLineAt(const QPointF& point) {
    if (!m_currentPage)
        return;
    if (!m_textBoxesExtracted)
        extractTextBoxes();
    int charIdx = findCharacterAtPoint(point);
    if (charIdx < 0)
        return;
    int start, end;
    findLineBoundaries(charIdx, start, end);
    m_selection.clear();
    m_selection.startCharIndex = start;
    m_selection.endCharIndex = end;
    m_selection.pageNumber = m_pageNumber;
    m_selection.startPoint = m_textBoxes[start].rect.topLeft();
    m_selection.endPoint = m_textBoxes[end].rect.bottomRight();
    calculateSelectionRects();
    extractSelectedText();
    emit selectionChanged();
}

void TextSelectionManager::selectAll() {
    if (!m_currentPage || m_textBoxes.isEmpty())
        return;
    if (!m_textBoxesExtracted)
        extractTextBoxes();
    m_selection.clear();
    m_selection.startCharIndex = 0;
    m_selection.endCharIndex = m_textBoxes.size() - 1;
    m_selection.pageNumber = m_pageNumber;
    m_selection.startPoint = m_textBoxes[0].rect.topLeft();
    m_selection.endPoint = m_textBoxes.last().rect.bottomRight();
    calculateSelectionRects();
    extractSelectedText();
    emit selectionChanged();
}

void TextSelectionManager::findWordBoundaries(int charIndex, int& start,
                                              int& end) {
    if (charIndex < 0 || charIndex >= m_textBoxes.size()) {
        start = end = -1;
        return;
    }
    start = charIndex;
    while (start > 0 && !m_wordBreaks[start])
        start--;
    if (start < m_pageText.length() &&
        (m_pageText[start].isSpace() || m_pageText[start].isPunct()))
        start++;
    end = charIndex;
    while (end < m_textBoxes.size() - 1 && !m_wordBreaks[end + 1])
        end++;
    if (end < m_pageText.length() &&
        (m_pageText[end].isSpace() || m_pageText[end].isPunct()))
        end--;
}

void TextSelectionManager::findLineBoundaries(int charIndex, int& start,
                                              int& end) {
    if (charIndex < 0 || charIndex >= m_textBoxes.size()) {
        start = end = -1;
        return;
    }
    start = charIndex;
    while (start > 0 && !m_lineBreaks[start])
        start--;
    end = charIndex;
    while (end < m_textBoxes.size() - 1 && !m_lineBreaks[end + 1])
        end++;
}

bool TextSelectionManager::isNewLine(int charIndex) const {
    return charIndex > 0 && charIndex < m_lineBreaks.size() &&
           m_lineBreaks[charIndex];
}

bool TextSelectionManager::isWordBoundary(int charIndex) const {
    return charIndex > 0 && charIndex < m_wordBreaks.size() &&
           m_wordBreaks[charIndex];
}

QList<QRectF> TextSelectionManager::getCharacterRects(int startIdx,
                                                      int endIdx) {
    QList<QRectF> rects;
    if (startIdx < 0 || endIdx >= m_charRects.size() || startIdx > endIdx)
        return rects;
    for (int i = startIdx; i <= endIdx; ++i)
        rects.append(m_charRects[i]);
    return rects;
}

bool TextSelectionManager::isPointInTextBox(const QPointF& point,
                                            const TextBox& box) const {
    return box.rect.contains(point);
}
