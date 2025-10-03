#include "SearchModel.h"
#include "../logging/LoggingMacros.h"
#include <QDebug>
// #include <QtConcurrent> // Not available in this setup
#include <QApplication>
#include <QRegularExpression>
#include <QRectF>
#include <QtGlobal>
#include <QTransform>
#include <QSizeF>
#include <QSize>
#include <QPointF>
#include <cmath>

SearchModel::SearchModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_currentResultIndex(-1)
    , m_isSearching(false)
    , m_document(nullptr)
    , m_searchWatcher(new QFutureWatcher<QList<SearchResult>>(this))
    , m_realTimeSearchTimer(new QTimer(this))
    , m_isRealTimeSearchEnabled(true)
    , m_realTimeSearchDelay(300)
    , m_advancedSearchEnabled(true)
    , m_maxHistorySize(20)
{
    connect(m_searchWatcher, &QFutureWatcher<QList<SearchResult>>::finished,
            this, &SearchModel::onSearchFinished);

    // Setup real-time search timer
    m_realTimeSearchTimer->setSingleShot(true);
    connect(m_realTimeSearchTimer, &QTimer::timeout,
            this, &SearchModel::performRealTimeSearch);

    // Advanced search features will be implemented directly in this class
}

int SearchModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    return m_results.size();
}

QVariant SearchModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_results.size()) {
        return QVariant();
    }

    const SearchResult& result = m_results.at(index.row());
    
    switch (role) {
        case Qt::DisplayRole:
            return QString("Page %1: %2").arg(result.pageNumber + 1).arg(result.contextText);
        case PageNumberRole:
            return result.pageNumber;
        case TextRole:
            return result.matchedText;
        case ContextRole:
            return result.contextText;
        case BoundingRectRole:
            return result.boundingRect;
        case StartIndexRole:
            return result.textPosition;
        case LengthRole:
            return result.textLength;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> SearchModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "display";
    roles[PageNumberRole] = "pageNumber";
    roles[TextRole] = "text";
    roles[ContextRole] = "context";
    roles[BoundingRectRole] = "boundingRect";
    roles[StartIndexRole] = "startIndex";
    roles[LengthRole] = "length";
    return roles;
}

void SearchModel::startSearch(Poppler::Document* document, const QString& query, const SearchOptions& options) {
    if (m_isSearching) {
        cancelSearch();
    }

    if (!document || query.isEmpty()) {
        emit searchError("Invalid document or empty query");
        return;
    }

    m_document = document;
    m_currentQuery = query;
    m_currentOptions = options;
    m_isSearching = true;
    m_currentResultIndex = -1;

    clearResults();

    // Use advanced search features if enabled
    if (m_advancedSearchEnabled) {
        // Advanced search features implemented directly
        emit searchStarted();
        performSearch();
        emit searchFinished(m_searchResults.size());
    } else {
        emit searchStarted();
        // Start search (synchronous for now)
        performSearch();
        emit searchFinished(m_searchResults.size());
    }
}

void SearchModel::startRealTimeSearch(Poppler::Document* document, const QString& query, const SearchOptions& options) {
    if (!m_isRealTimeSearchEnabled || query.isEmpty()) {
        return;
    }

    // Cancel any pending real-time search
    m_realTimeSearchTimer->stop();

    // Store search parameters
    m_document = document;
    m_currentQuery = query;
    m_currentOptions = options;

    // Start debounced search
    m_realTimeSearchTimer->start(m_realTimeSearchDelay);
}

void SearchModel::clearResults() {
    beginResetModel();
    m_results.clear();
    m_currentResultIndex = -1;
    endResetModel();
    emit resultsCleared();
}

void SearchModel::cancelSearch() {
    if (m_isSearching && !m_searchFuture.isFinished()) {
        m_searchFuture.cancel();
        m_isSearching = false;
        emit searchCancelled();
    }
}

SearchResult SearchModel::getResult(int index) const {
    if (index >= 0 && index < m_results.size()) {
        return m_results.at(index);
    }
    return SearchResult();
}

void SearchModel::setCurrentResultIndex(int index) {
    if (index >= -1 && index < m_results.size() && index != m_currentResultIndex) {
        m_currentResultIndex = index;
        emit currentResultChanged(index);
    }
}

bool SearchModel::hasNext() const {
    return m_currentResultIndex < m_results.size() - 1;
}

bool SearchModel::hasPrevious() const {
    return m_currentResultIndex > 0;
}

SearchResult SearchModel::nextResult() {
    if (hasNext()) {
        setCurrentResultIndex(m_currentResultIndex + 1);
        return m_results.at(m_currentResultIndex);
    }
    return SearchResult();
}

SearchResult SearchModel::previousResult() {
    if (hasPrevious()) {
        setCurrentResultIndex(m_currentResultIndex - 1);
        return m_results.at(m_currentResultIndex);
    }
    return SearchResult();
}

void SearchModel::onSearchFinished() {
    if (m_searchFuture.isCanceled()) {
        m_isSearching = false;
        emit searchCancelled();
        return;
    }

    try {
        QList<SearchResult> results = m_searchFuture.result();
        
        beginResetModel();
        m_results = results;
        endResetModel();
        
        if (!m_results.isEmpty()) {
            setCurrentResultIndex(0);
        }
        
        m_isSearching = false;
        emit searchFinished(m_results.size());
        
    } catch (const std::exception& e) {
        m_isSearching = false;
        emit searchError(QString("Search failed: %1").arg(e.what()));
    }
}

void SearchModel::performSearch() {
    QList<SearchResult> allResults;

    if (!m_document) {
        emit searchError("Document is null");
        return;
    }

    const int pageCount = m_document->numPages();
    if (pageCount <= 0) {
        emit searchError("Document has no pages");
        return;
    }

    // Simple search without complex error handling for testing
    for (int i = 0; i < pageCount; ++i) {
        std::unique_ptr<Poppler::Page> page(m_document->page(i));
        if (!page) {
            continue; // Skip invalid pages but continue search
        }

        QList<SearchResult> pageResults = searchInPage(page.get(), i, m_currentQuery, m_currentOptions);
        allResults.append(pageResults);

        if (allResults.size() >= m_currentOptions.maxResults) {
            break;
        }
    }

    // Update the model with results
    beginResetModel();
    m_searchResults = allResults;
    m_results = allResults; // Ensure both result lists are synchronized
    m_isSearching = false;
    endResetModel();
}

QList<SearchResult> SearchModel::searchInPage(Poppler::Page* page, int pageNumber, 
                                            const QString& query, const SearchOptions& options) {
    QList<SearchResult> results;
    
    if (!page) {
        return results;
    }

    QString pageText = page->text(QRectF());
    if (pageText.isEmpty()) {
        return results;
    }

    QRegularExpression regex = createSearchRegex(query, options);
    QRegularExpressionMatchIterator iterator = regex.globalMatch(pageText);
    
    while (iterator.hasNext() && results.size() < options.maxResults) {
        QRegularExpressionMatch match = iterator.next();
        int startPos = match.capturedStart();
        int length = match.capturedLength();
        QString matchedText = match.captured();
        
        // Extract context around the match
        QString context = extractContext(pageText, startPos, length);
        
        // Get bounding rectangle for the matched text
        QList<QRectF> rects = page->search(matchedText);
        
        QRectF boundingRect;
        if (!rects.isEmpty()) {
            boundingRect = rects.first();
        }
        
        SearchResult result(pageNumber, matchedText, context, boundingRect, startPos, length);
        results.append(result);
    }
    
    return results;
}

QString SearchModel::extractContext(const QString& pageText, int position, int length, int contextLength) {
    int start = qMax(0, position - contextLength);
    int end = qMin(pageText.length(), position + length + contextLength);
    
    QString context = pageText.mid(start, end - start);
    
    // Add ellipsis if we truncated
    if (start > 0) {
        context = "..." + context;
    }
    if (end < pageText.length()) {
        context = context + "...";
    }
    
    return context.simplified(); // Remove extra whitespace
}

QRegularExpression SearchModel::createSearchRegex(const QString& query, const SearchOptions& options) {
    QString pattern = query;
    
    if (!options.useRegex) {
        pattern = QRegularExpression::escape(pattern);
    }
    
    if (options.wholeWords) {
        pattern = "\\b" + pattern + "\\b";
    }
    
    QRegularExpression::PatternOptions regexOptions = QRegularExpression::MultilineOption;
    if (!options.caseSensitive) {
        regexOptions |= QRegularExpression::CaseInsensitiveOption;
    }

    return QRegularExpression(pattern, regexOptions);
}

// Real-time search implementation
void SearchModel::performRealTimeSearch() {
    if (!m_document || m_currentQuery.isEmpty()) {
        return;
    }

    emit realTimeSearchStarted();

    QList<SearchResult> allResults;
    const int pageCount = m_document->numPages();

    for (int i = 0; i < pageCount; ++i) {
        std::unique_ptr<Poppler::Page> page(m_document->page(i));
        if (page) {
            QList<SearchResult> pageResults = searchInPage(page.get(), i, m_currentQuery, m_currentOptions);
            allResults.append(pageResults);

            // Emit progress and partial results for real-time feedback
            emit realTimeSearchProgress(i + 1, pageCount);
            if (!allResults.isEmpty()) {
                emit realTimeResultsUpdated(allResults);
            }

            // Limit results for performance
            if (allResults.size() >= m_currentOptions.maxResults) {
                break;
            }
        }
    }

    // Update the model with final results
    beginResetModel();
    m_searchResults = allResults;
    m_results = allResults; // Keep both for compatibility
    endResetModel();

    emit searchFinished(allResults.size());
}

// SearchResult coordinate transformation now implemented in SearchConfiguration.cpp

// Advanced search methods implementation
void SearchModel::setAdvancedSearchEnabled(bool enabled)
{
    m_advancedSearchEnabled = enabled;
    // Advanced search features are implemented directly in this class
}

void SearchModel::onAdvancedSearchFinished(const QList<SearchResult>& results)
{
    beginResetModel();
    m_results = results;
    m_searchResults = results;
    m_isSearching = false;
    endResetModel();

    emit searchFinished(results.size());

    if (!results.isEmpty()) {
        m_currentResultIndex = 0;
        emit currentResultChanged(0);
    }
}

// Advanced search implementations
void SearchModel::startFuzzySearch(Poppler::Document* document, const QString& query, const SearchOptions& options)
{
    if (m_isSearching) {
        cancelSearch();
    }

    if (!document || query.isEmpty()) {
        emit searchError("Invalid document or empty query");
        return;
    }

    m_document = document;
    m_currentQuery = query;
    m_currentOptions = options;
    m_isSearching = true;
    m_currentResultIndex = -1;

    clearResults();
    addToSearchHistory(query);

    emit searchStarted();

    // Perform fuzzy search
    QList<SearchResult> results = performFuzzySearch(query, options);

    beginResetModel();
    m_searchResults = results;
    m_results = results;
    m_isSearching = false;
    endResetModel();

    emit searchFinished(results.size());
}

void SearchModel::startPageRangeSearch(Poppler::Document* document, const QString& query, int startPage, int endPage, const SearchOptions& options)
{
    if (m_isSearching) {
        cancelSearch();
    }

    if (!document || query.isEmpty()) {
        emit searchError("Invalid document or empty query");
        return;
    }

    m_document = document;
    m_currentQuery = query;
    m_currentOptions = options;
    m_isSearching = true;
    m_currentResultIndex = -1;

    clearResults();
    addToSearchHistory(query);

    emit searchStarted();

    // Perform page range search
    QList<SearchResult> results = performPageRangeSearch(query, startPage, endPage, options);

    beginResetModel();
    m_searchResults = results;
    m_results = results;
    m_isSearching = false;
    endResetModel();

    emit searchFinished(results.size());
}

// Search history management
void SearchModel::addToSearchHistory(const QString& query)
{
    if (query.isEmpty()) return;

    // Remove if already exists to move to front
    m_searchHistory.removeAll(query);

    // Add to front
    m_searchHistory.prepend(query);

    // Limit history size
    while (m_searchHistory.size() > m_maxHistorySize) {
        m_searchHistory.removeLast();
    }
}

void SearchModel::clearSearchHistory()
{
    m_searchHistory.clear();
}

// Advanced search algorithm implementations
QList<SearchResult> SearchModel::performFuzzySearch(const QString& query, const SearchOptions& options)
{
    QList<SearchResult> allResults;

    if (!m_document) {
        return allResults;
    }

    const int pageCount = m_document->numPages();

    for (int i = 0; i < pageCount; ++i) {
        std::unique_ptr<Poppler::Page> page(m_document->page(i));
        if (page) {
            QString pageText = page->text(QRectF());

            // Split page text into words for fuzzy matching
            QStringList words = pageText.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

            for (int wordIndex = 0; wordIndex < words.size(); ++wordIndex) {
                const QString& word = words[wordIndex];

                if (isFuzzyMatch(word, query, options.fuzzyThreshold)) {
                    // Find the position of this word in the original text
                    int position = pageText.indexOf(word, 0, options.caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);

                    if (position >= 0) {
                        SearchResult result;
                        result.pageNumber = i;
                        result.matchedText = word;
                        result.contextText = extractContext(pageText, position, word.length());
                        result.textPosition = position;
                        result.textLength = word.length();
                        result.isCurrentResult = false;

                        // Get bounding box for the word
                        Poppler::Page::SearchFlags flags = Poppler::Page::NoSearchFlags;
                        if (!options.caseSensitive) {
                            flags |= Poppler::Page::IgnoreCase;
                        }
                        QList<QRectF> boxes = page->search(word, flags);
                        if (!boxes.isEmpty()) {
                            result.boundingRect = boxes.first();
                        }

                        allResults.append(result);

                        if (allResults.size() >= options.maxResults) {
                            return allResults;
                        }
                    }
                }
            }
        }
    }

    return allResults;
}

QList<SearchResult> SearchModel::performPageRangeSearch(const QString& query, int startPage, int endPage, const SearchOptions& options)
{
    QList<SearchResult> allResults;

    if (!m_document) {
        return allResults;
    }

    const int pageCount = m_document->numPages();
    int actualStartPage = qMax(0, startPage);
    int actualEndPage = qMin(pageCount - 1, endPage);

    if (actualStartPage > actualEndPage) {
        return allResults;
    }

    for (int i = actualStartPage; i <= actualEndPage; ++i) {
        std::unique_ptr<Poppler::Page> page(m_document->page(i));
        if (page) {
            QList<SearchResult> pageResults = searchInPage(page.get(), i, query, options);
            allResults.append(pageResults);

            if (allResults.size() >= options.maxResults) {
                break;
            }
        }
    }

    return allResults;
}

int SearchModel::calculateLevenshteinDistance(const QString& str1, const QString& str2)
{
    const int len1 = str1.length();
    const int len2 = str2.length();

    if (len1 == 0) return len2;
    if (len2 == 0) return len1;

    QVector<QVector<int>> matrix(len1 + 1, QVector<int>(len2 + 1));

    // Initialize first row and column
    for (int i = 0; i <= len1; ++i) {
        matrix[i][0] = i;
    }
    for (int j = 0; j <= len2; ++j) {
        matrix[0][j] = j;
    }

    // Fill the matrix
    for (int i = 1; i <= len1; ++i) {
        for (int j = 1; j <= len2; ++j) {
            int cost = (str1[i-1] == str2[j-1]) ? 0 : 1;

            int deletion = matrix[i-1][j] + 1;
            int insertion = matrix[i][j-1] + 1;
            int substitution = matrix[i-1][j-1] + cost;

            matrix[i][j] = qMin(qMin(deletion, insertion), substitution);
        }
    }

    return matrix[len1][len2];
}

bool SearchModel::isFuzzyMatch(const QString& text, const QString& query, int threshold)
{
    if (text.isEmpty() || query.isEmpty()) {
        return false;
    }

    // Exact match
    if (text.compare(query, Qt::CaseInsensitive) == 0) {
        return true;
    }

    // Check if the text contains the query as substring
    if (text.contains(query, Qt::CaseInsensitive)) {
        return true;
    }

    // Calculate edit distance for fuzzy matching
    int distance = calculateLevenshteinDistance(text.toLower(), query.toLower());
    return distance <= threshold;
}
