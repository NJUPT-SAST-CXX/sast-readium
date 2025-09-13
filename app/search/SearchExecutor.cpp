#include "SearchExecutor.h"
#include "TextExtractor.h"
#include <QDebug>

class SearchExecutor::Implementation
{
public:
    Implementation(SearchExecutor* q)
        : q_ptr(q)
        , textExtractor(nullptr)
    {
    }

    QString extractContext(const QString& text, int position, int length) const
    {
        int contextStart = qMax(0, position - options.contextLength);
        int contextEnd = qMin(text.length(), position + length + options.contextLength);
        return text.mid(contextStart, contextEnd - contextStart);
    }

    QList<SearchResult> performSearch(const QString& text, const QString& query, int pageNumber)
    {
        QList<SearchResult> results;
        
        if (text.isEmpty() || query.isEmpty()) {
            return results;
        }

        QRegularExpression regex = q_ptr->createSearchPattern(query, options);
        if (!regex.isValid()) {
            emit q_ptr->searchError("Invalid search pattern: " + regex.errorString());
            return results;
        }

        QRegularExpressionMatchIterator iterator = regex.globalMatch(text);
        
        while (iterator.hasNext() && results.size() < options.maxResults) {
            QRegularExpressionMatch match = iterator.next();
            
            int position = match.capturedStart();
            int length = match.capturedLength();
            QString matchedText = match.captured();
            QString context = extractContext(text, position, length);
            
            // Create result with empty bounding rect (would need page object for actual bounds)
            SearchResult result(pageNumber, matchedText, context, QRectF(), position, length);
            results.append(result);
            
            emit q_ptr->resultFound(result);
        }
        
        return results;
    }

    SearchExecutor* q_ptr;
    TextExtractor* textExtractor;
    SearchOptions options;
};

SearchExecutor::SearchExecutor(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Implementation>(this))
{
}

SearchExecutor::~SearchExecutor() = default;

void SearchExecutor::setTextExtractor(TextExtractor* extractor)
{
    d->textExtractor = extractor;
}

void SearchExecutor::setOptions(const SearchOptions& options)
{
    d->options = options;
}

QList<SearchResult> SearchExecutor::searchInPage(int pageNumber, const QString& query)
{
    if (!d->textExtractor) {
        emit searchError("No text extractor available");
        return QList<SearchResult>();
    }

    QString pageText = d->textExtractor->extractPageText(pageNumber);
    return searchInText(pageText, query, pageNumber);
}

QList<SearchResult> SearchExecutor::searchInPages(const QList<int>& pageNumbers, const QString& query)
{
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

QList<SearchResult> SearchExecutor::searchInText(const QString& text, const QString& query, int pageNumber)
{
    return d->performSearch(text, query, pageNumber);
}

bool SearchExecutor::validateQuery(const QString& query) const
{
    if (query.isEmpty()) {
        return false;
    }

    if (d->options.useRegex) {
        QRegularExpression regex(query);
        return regex.isValid();
    }

    return true;
}

QRegularExpression SearchExecutor::createSearchPattern(const QString& query) const
{
    return createSearchPattern(query, d->options);
}

QRegularExpression SearchExecutor::createSearchPattern(const QString& query, const SearchOptions& options) const
{
    QString pattern = query;
    
    if (!options.useRegex) {
        pattern = QRegularExpression::escape(query);
    }
    
    if (options.wholeWords) {
        pattern = "\\b" + pattern + "\\b";
    }
    
    QRegularExpression::PatternOptions regexOptions = QRegularExpression::NoPatternOption;
    if (!options.caseSensitive) {
        regexOptions |= QRegularExpression::CaseInsensitiveOption;
    }
    
    QRegularExpression regex(pattern, regexOptions);
    return regex;
}
