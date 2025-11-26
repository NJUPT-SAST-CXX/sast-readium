#include "SearchExecutor.h"
#include <poppler/qt6/poppler-qt6.h>
#include <QDebug>
#include "SearchErrorRecovery.h"
#include "SearchValidator.h"
#include "TextExtractor.h"
#include "logging/LoggingMacros.h"

class SearchExecutor::Implementation {
public:
    Implementation(SearchExecutor* q)
        : q_ptr(q), textExtractor(nullptr), validator(new SearchValidator()) {}

    QString extractContext(const QString& text, int position,
                           int length) const {
        int contextStart = qMax(0, position - options.contextLength);
        int contextEnd =
            qMin(text.length(), position + length + options.contextLength);
        return text.mid(contextStart, contextEnd - contextStart);
    }

    QList<SearchResult> performSearch(const QString& text, const QString& query,
                                      int pageNumber) {
        QList<SearchResult> results;

        if (text.isEmpty() || query.isEmpty()) {
            return results;
        }

        QRegularExpression regex = q_ptr->createSearchPattern(query, options);
        if (!regex.isValid()) {
            emit q_ptr->searchError("Invalid search pattern: " +
                                    regex.errorString());
            return results;
        }

        QRegularExpressionMatchIterator iterator = regex.globalMatch(text);

        while (iterator.hasNext() && results.size() < options.maxResults) {
            QRegularExpressionMatch match = iterator.next();

            int position = match.capturedStart();
            int length = match.capturedLength();
            QString matchedText = match.captured();
            QString context = extractContext(text, position, length);

            // Calculate bounding rect for the matched text
            QRectF boundingRect =
                calculateBoundingRect(pageNumber, position, length);

            SearchResult result(pageNumber, matchedText, context, boundingRect,
                                position, length);
            results.append(result);

            emit q_ptr->resultFound(result);
        }

        return results;
    }

    QRectF calculateBoundingRect(int pageNumber, int textPosition,
                                 int textLength) {
        if (!textExtractor || textPosition < 0 || textLength <= 0) {
            return QRectF();
        }

        try {
            // Try to get actual PDF page for accurate coordinates
            auto* document = textExtractor->getDocument();
            if (!document) {
                return calculateEstimatedRect(textPosition, textLength);
            }

            // Get the specific page
            auto page = document->page(pageNumber);
            if (!page) {
                return calculateEstimatedRect(textPosition, textLength);
            }

            // Extract full text from page to find the match context
            QString pageText = textExtractor->extractPageText(pageNumber);
            if (pageText.isEmpty()) {
                return calculateEstimatedRect(textPosition, textLength);
            }

            // Find the actual match substring and get context
            int matchStart = textPosition;
            int matchEnd = textPosition + textLength;

            // Get some context around the match (100 characters before and
            // after)
            int contextStart = qMax(0, matchStart - 100);
            int contextEnd = qMin(pageText.length(), matchEnd + 100);
            QString contextText =
                pageText.mid(contextStart, contextEnd - contextStart);

            // Calculate relative position within context
            int relativeStart = matchStart - contextStart;
            int relativeEnd = matchEnd - contextStart;

            // Use Poppler's text boxes to find actual coordinates
            // This requires Poppler::TextBox which provides accurate position
            // info
            QList<QRectF> textBoxes = extractTextBoxes(page, contextText);

            if (textBoxes.isEmpty()) {
                return calculateEstimatedRect(textPosition, textLength);
            }

            // Combine rectangles for the matching text
            return combineTextBoxRects(textBoxes, relativeStart, relativeEnd);

        } catch (const std::exception& e) {
            LOG_WARNING("Error calculating precise bounding rectangle: {}",
                        e.what());
            return calculateEstimatedRect(textPosition, textLength);
        } catch (...) {
            LOG_WARNING("Unknown error calculating bounding rectangle");
            return calculateEstimatedRect(textPosition, textLength);
        }
    }

private:
    QList<QRectF> extractTextBoxes(const std::unique_ptr<Poppler::Page>& page,
                                   const QString& text) {
        QList<QRectF> textBoxes;

        if (!page || text.isEmpty()) {
            LOG_DEBUG(
                "SearchExecutor::extractTextBoxes: Invalid page or empty text");
            return textBoxes;
        }

        try {
            // Get text boxes from Poppler page
            // Poppler::Page::textList() returns a vector of
            // unique_ptr<Poppler::TextBox> Each TextBox contains text content
            // and its bounding rectangle
            auto popplerTextBoxes = page->textList();

            if (popplerTextBoxes.empty()) {
                LOG_DEBUG(
                    "SearchExecutor::extractTextBoxes: No text boxes found on "
                    "page");
                return textBoxes;
            }

            // Extract bounding rectangles from text boxes
            // We need to find text boxes that contain parts of our search
            // context
            for (const auto& textBox : popplerTextBoxes) {
                if (textBox) {
                    // Get the bounding box for this text element
                    QRectF boundingBox = textBox->boundingBox();

                    // Get the text content of this box
                    QString boxText = textBox->text();

                    // Check if this text box contains any part of our search
                    // text We use case-insensitive comparison to be more
                    // flexible
                    if (!boxText.isEmpty() &&
                        (text.contains(boxText, Qt::CaseInsensitive) ||
                         boxText.contains(text, Qt::CaseInsensitive))) {
                        textBoxes.append(boundingBox);

                        LOG_TRACE(
                            "SearchExecutor::extractTextBoxes: Found matching "
                            "text box at ({}, {}) size {}x{}",
                            boundingBox.x(), boundingBox.y(),
                            boundingBox.width(), boundingBox.height());
                    }
                }
            }

            // No manual cleanup needed - unique_ptr handles memory
            // automatically

            LOG_DEBUG(
                "SearchExecutor::extractTextBoxes: Extracted {} text boxes "
                "from {} total",
                textBoxes.size(), popplerTextBoxes.size());

        } catch (const std::exception& e) {
            LOG_ERROR(
                "SearchExecutor::extractTextBoxes: Exception while extracting "
                "text boxes: {}",
                e.what());
            // Return empty list on error - fallback will be used
        } catch (...) {
            LOG_ERROR(
                "SearchExecutor::extractTextBoxes: Unknown exception while "
                "extracting text boxes");
            // Return empty list on error - fallback will be used
        }

        return textBoxes;
    }

    QRectF combineTextBoxRects(const QList<QRectF>& textBoxes, int startChar,
                               int endChar) {
        if (textBoxes.isEmpty()) {
            return QRectF();
        }

        // For now, create a bounding rectangle that encompasses all relevant
        // text boxes In a real implementation, you would map character
        // positions to specific text boxes
        QRectF combinedRect;
        for (const QRectF& rect : textBoxes) {
            if (combinedRect.isEmpty()) {
                combinedRect = rect;
            } else {
                combinedRect = combinedRect.united(rect);
            }
        }

        // Adjust for the specific character range if needed
        // This is a simplified approach - a full implementation would be more
        // precise
        return combinedRect;
    }

    QRectF calculateEstimatedRect(int textPosition, int textLength) {
        // Fallback to estimated calculation when precise methods aren't
        // available

        // More sophisticated estimation based on typical PDF characteristics
        const double avgCharWidth =
            7.5;  // More realistic average character width
        const double lineHeight = 11.5;  // Standard line height for most PDFs
        const double pageMargin = 36.0;  // 0.5 inch margin on all sides
        const double usableWidth =
            612.0 - 2 * pageMargin;  // Standard letter size minus margins
        const double usableHeight =
            792.0 - 2 * pageMargin;  // Standard letter size minus margins

        // Calculate characters per line based on typical column width
        const int charsPerLine = static_cast<int>(usableWidth / avgCharWidth);

        // Calculate approximate position with margin offset
        int line = textPosition / charsPerLine;
        int column = textPosition % charsPerLine;

        // Ensure we stay within page bounds
        int maxLines = static_cast<int>(usableHeight / lineHeight);
        if (line >= maxLines) {
            line = maxLines - 1;
        }

        // Calculate bounding rectangle with margins
        double x = pageMargin + (column * avgCharWidth);
        double y = pageMargin + (line * lineHeight);
        double width = qMin(textLength * avgCharWidth,
                            usableWidth - (column * avgCharWidth));
        double height = lineHeight;

        // Ensure rectangle is within page bounds
        x = qBound(pageMargin, x, pageMargin + usableWidth - width);
        y = qBound(pageMargin, y, pageMargin + usableHeight - height);

        return QRectF(x, y, width, height);
    }

public:
    SearchExecutor* q_ptr;
    TextExtractor* textExtractor;
    SearchOptions options;
    SearchValidator* validator;
};

SearchExecutor::SearchExecutor(QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this)) {}

SearchExecutor::~SearchExecutor() = default;

void SearchExecutor::setTextExtractor(TextExtractor* extractor) {
    d->textExtractor = extractor;
}

void SearchExecutor::setOptions(const SearchOptions& options) {
    d->options = options;
}

QList<SearchResult> SearchExecutor::searchInPage(int pageNumber,
                                                 const QString& query) {
    try {
        // Validate inputs
        auto queryResult = d->validator->validateQuery(query);
        if (!queryResult.isValid) {
            emit searchError(QString("Invalid query: %1")
                                 .arg(queryResult.errorMessages.join("; ")));
            return QList<SearchResult>();
        }

        auto pageResult = d->validator->validatePageNumber(pageNumber, INT_MAX);
        if (!pageResult.isValid) {
            emit searchError(QString("Invalid page number: %1")
                                 .arg(pageResult.errorMessages.join("; ")));
            return QList<SearchResult>();
        }

        if (!d->textExtractor) {
            emit searchError("No text extractor available");
            return QList<SearchResult>();
        }

        QString pageText = d->textExtractor->extractPageText(pageNumber);
        return searchInText(pageText,
                            queryResult.sanitizedInput.isEmpty()
                                ? query
                                : queryResult.sanitizedInput,
                            pageNumber);

    } catch (const std::exception& e) {
        emit searchError(QString("Search error on page %1: %2")
                             .arg(pageNumber)
                             .arg(e.what()));
        return QList<SearchResult>();
    }
}

QList<SearchResult> SearchExecutor::searchInPages(const QList<int>& pageNumbers,
                                                  const QString& query) {
    QList<SearchResult> allResults;
    int total = pageNumbers.size();
    int current = 0;

    for (int pageNumber : pageNumbers) {
        QList<SearchResult> pageResults = searchInPage(pageNumber, query);
        allResults.append(pageResults);

        current++;
        emit searchProgress(current, total);

        if (allResults.size() >= d->options.maxResults) {
            break;
        }
    }

    return allResults;
}

QList<SearchResult> SearchExecutor::searchInText(const QString& text,
                                                 const QString& query,
                                                 int pageNumber) {
    return d->performSearch(text, query, pageNumber);
}

bool SearchExecutor::validateQuery(const QString& query) const {
    if (query.isEmpty()) {
        return false;
    }

    if (d->options.useRegex) {
        QRegularExpression regex(query);
        return regex.isValid();
    }

    return true;
}

QRegularExpression SearchExecutor::createSearchPattern(
    const QString& query) const {
    return createSearchPattern(query, d->options);
}

QRegularExpression SearchExecutor::createSearchPattern(
    const QString& query, const SearchOptions& options) const {
    QString pattern = query;

    if (!options.useRegex) {
        pattern = QRegularExpression::escape(query);
    }

    if (options.wholeWords) {
        pattern = "\\b" + pattern + "\\b";
    }

    QRegularExpression::PatternOptions regexOptions =
        QRegularExpression::NoPatternOption;
    if (!options.caseSensitive) {
        regexOptions |= QRegularExpression::CaseInsensitiveOption;
    }

    QRegularExpression regex(pattern, regexOptions);
    return regex;
}

QRectF SearchExecutor::calculateBoundingRect(int pageNumber, int textPosition,
                                             int textLength) {
    return d->calculateBoundingRect(pageNumber, textPosition, textLength);
}
