#include "SearchFeatures.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

class SearchFeatures::Implementation
{
public:
    Implementation(SearchFeatures* q)
        : q_ptr(q)
        , normalHighlightColor("#FFFF00")
        , currentHighlightColor("#FF6600")
        , maxHistorySize(100)
        , highlightEngine(new SearchHighlightEngine())
        , suggestionEngine(new SearchSuggestionEngine())
        , booleanParser(new BooleanSearchParser())
    {
        setupDefaultHighlightStyles();
    }

    ~Implementation()
    {
        delete highlightEngine;
        delete suggestionEngine;
        delete booleanParser;
    }

    void setupDefaultHighlightStyles()
    {
        SearchHighlightEngine::HighlightStyle defaultStyle;
        defaultStyle.backgroundColor = normalHighlightColor;
        defaultStyle.textColor = QColor("#000000");
        defaultStyle.borderColor = QColor("#CCCCCC");
        defaultStyle.borderWidth = 1;
        defaultStyle.opacity = 0.7;
        highlightEngine->setHighlightStyle("default", defaultStyle);

        SearchHighlightEngine::HighlightStyle currentStyle;
        currentStyle.backgroundColor = currentHighlightColor;
        currentStyle.textColor = QColor("#FFFFFF");
        currentStyle.borderColor = QColor("#FF0000");
        currentStyle.borderWidth = 2;
        currentStyle.opacity = 0.9;
        highlightEngine->setHighlightStyle("current", currentStyle);
    }

    SearchFeatures* q_ptr;
    QColor normalHighlightColor;
    QColor currentHighlightColor;
    
    QList<SearchFeatures::HistoryEntry> searchHistory;
    int maxHistorySize;
    
    SearchHighlightEngine* highlightEngine;
    SearchSuggestionEngine* suggestionEngine;
    BooleanSearchParser* booleanParser;
    
    SearchFeatures::SearchStatistics statistics;
    QMutex historyMutex;
    QMutex statisticsMutex;
};

SearchFeatures::SearchFeatures(QObject* parent)
    : QObject(parent)
    , d(new Implementation(this))
{
}

SearchFeatures::~SearchFeatures() = default;

QList<SearchFeatures::FuzzyMatch> SearchFeatures::fuzzySearch(
    const QString& text, const QString& pattern, int maxDistance, int maxResults)
{
    QList<FuzzyMatch> matches;
    
    if (pattern.isEmpty() || text.isEmpty()) {
        return matches;
    }
    
    QStringList words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    int currentPosition = 0;
    
    for (const QString& word : words) {
        int distance = FuzzySearchAlgorithms::levenshteinDistanceOptimized(word, pattern, maxDistance);
        
        if (distance <= maxDistance) {
            FuzzyMatch match;
            match.text = word;
            match.position = currentPosition;
            match.length = word.length();
            match.editDistance = distance;
            match.similarity = 1.0 - (static_cast<double>(distance) / qMax(word.length(), pattern.length()));
            
            // Extract context
            int contextStart = qMax(0, currentPosition - 50);
            int contextEnd = qMin(text.length(), currentPosition + word.length() + 50);
            match.context = text.mid(contextStart, contextEnd - contextStart);
            
            matches.append(match);
            
            if (maxResults > 0 && matches.size() >= maxResults) {
                break;
            }
        }
        
        currentPosition = text.indexOf(word, currentPosition) + word.length();
    }
    
    // Sort by similarity (descending)
    std::sort(matches.begin(), matches.end(), [](const FuzzyMatch& a, const FuzzyMatch& b) {
        return a.similarity > b.similarity;
    });
    
    emit fuzzySearchCompleted(matches);
    return matches;
}

int SearchFeatures::calculateLevenshteinDistance(const QString& str1, const QString& str2)
{
    return FuzzySearchAlgorithms::levenshteinDistance(str1, str2);
}

double SearchFeatures::calculateSimilarity(const QString& str1, const QString& str2)
{
    if (str1.isEmpty() && str2.isEmpty()) {
        return 1.0;
    }
    
    int distance = calculateLevenshteinDistance(str1, str2);
    int maxLength = qMax(str1.length(), str2.length());
    
    return 1.0 - (static_cast<double>(distance) / maxLength);
}

QList<SearchResult> SearchFeatures::wildcardSearch(const QString& text, const QString& pattern, int pageNumber)
{
    QList<SearchResult> results;
    
    // Convert wildcard pattern to regex
    QString regexPattern = pattern;
    regexPattern.replace("*", ".*");
    regexPattern.replace("?", ".");
    
    QRegularExpression regex(regexPattern, QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator iterator = regex.globalMatch(text);
    
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();

        SearchResult result;
        result.pageNumber = pageNumber;
        result.matchedText = match.captured();
        result.textPosition = match.capturedStart();
        result.textLength = match.capturedLength();

        // Extract context
        int contextStart = qMax(0, result.textPosition - 50);
        int contextEnd = qMin(text.length(), result.textPosition + result.textLength + 50);
        result.contextText = text.mid(contextStart, contextEnd - contextStart);

        results.append(result);
    }

    return results;
}

QList<SearchResult> SearchFeatures::phraseSearch(const QString& text, const QString& phrase, int pageNumber, int proximity)
{
    QList<SearchResult> results;
    
    if (proximity == 0) {
        // Exact phrase search
        int position = 0;
        while ((position = text.indexOf(phrase, position, Qt::CaseInsensitive)) != -1) {
            SearchResult result;
            result.pageNumber = pageNumber;
            result.matchedText = phrase;
            result.textPosition = position;
            result.textLength = phrase.length();

            // Extract context
            int contextStart = qMax(0, position - 50);
            int contextEnd = qMin(text.length(), position + phrase.length() + 50);
            result.contextText = text.mid(contextStart, contextEnd - contextStart);

            results.append(result);
            position += phrase.length();
        }
    } else {
        // Proximity phrase search
        QStringList phraseWords = phrase.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        ProximitySearchOptions options;
        options.maxDistance = proximity;
        options.ordered = true;
        options.caseSensitive = false;
        
        results = proximitySearch(text, phraseWords, options, pageNumber);
    }
    
    return results;
}

QList<SearchResult> SearchFeatures::booleanSearch(const QString& text, const QString& query, int pageNumber)
{
    auto queryTree = d->booleanParser->parseQuery(query);
    if (!queryTree) {
        return QList<SearchResult>();
    }
    
    return d->booleanParser->executeQuery(queryTree, text, pageNumber);
}

void SearchFeatures::setHighlightColors(const QColor& normalColor, const QColor& currentColor)
{
    d->normalHighlightColor = normalColor;
    d->currentHighlightColor = currentColor;
    d->setupDefaultHighlightStyles();
}

QColor SearchFeatures::getNormalHighlightColor() const
{
    return d->normalHighlightColor;
}

QColor SearchFeatures::getCurrentHighlightColor() const
{
    return d->currentHighlightColor;
}

QList<SearchFeatures::HighlightInfo> SearchFeatures::generateHighlights(
    const QList<SearchResult>& results, int currentResultIndex)
{
    QList<HighlightInfo> highlights;
    
    for (int i = 0; i < results.size(); ++i) {
        const SearchResult& result = results[i];
        
        HighlightInfo highlight;
        highlight.rect = result.boundingRect;
        highlight.text = result.matchedText;
        highlight.priority = results.size() - i; // Higher priority for earlier results
        highlight.isCurrentResult = (i == currentResultIndex);
        
        if (highlight.isCurrentResult) {
            highlight.color = d->currentHighlightColor;
        } else {
            highlight.color = d->normalHighlightColor;
        }
        
        highlights.append(highlight);
    }
    
    updateHighlightPriorities(highlights);
    emit highlightsGenerated(highlights);
    
    return highlights;
}

void SearchFeatures::updateHighlightPriorities(QList<HighlightInfo>& highlights)
{
    // Sort by priority (current result first, then by position)
    std::sort(highlights.begin(), highlights.end(), [](const HighlightInfo& a, const HighlightInfo& b) {
        if (a.isCurrentResult != b.isCurrentResult) {
            return a.isCurrentResult; // Current result has highest priority
        }
        return a.priority > b.priority;
    });
}

void SearchFeatures::addToHistory(const QString& query, const SearchOptions& options, 
                                        int resultCount, qint64 searchTime, bool successful)
{
    QMutexLocker locker(&d->historyMutex);
    
    HistoryEntry entry;
    entry.query = query;
    entry.options = options;
    entry.timestamp = QDateTime::currentDateTime();
    entry.resultCount = resultCount;
    entry.searchTime = searchTime;
    entry.successful = successful;
    
    d->searchHistory.prepend(entry);
    
    // Limit history size
    while (d->searchHistory.size() > d->maxHistorySize) {
        d->searchHistory.removeLast();
    }
    
    // Update statistics
    QMutexLocker statsLocker(&d->statisticsMutex);
    d->statistics.totalSearches++;
    if (successful) {
        d->statistics.successfulSearches++;
    }
    d->statistics.lastSearchTime = entry.timestamp;
    
    // Update averages
    if (d->statistics.totalSearches > 0) {
        d->statistics.averageSearchTime = 
            (d->statistics.averageSearchTime * (d->statistics.totalSearches - 1) + searchTime) / d->statistics.totalSearches;
        d->statistics.averageResultCount = 
            (d->statistics.averageResultCount * (d->statistics.totalSearches - 1) + resultCount) / d->statistics.totalSearches;
    }
    
    // Update query frequency
    d->statistics.queryFrequency[query]++;
    
    emit historyUpdated();
    emit statisticsUpdated(d->statistics);
}

QList<SearchFeatures::HistoryEntry> SearchFeatures::getSearchHistory(int maxEntries) const
{
    QMutexLocker locker(&d->historyMutex);
    
    if (maxEntries < 0 || maxEntries >= d->searchHistory.size()) {
        return d->searchHistory;
    }
    
    return d->searchHistory.mid(0, maxEntries);
}

QStringList SearchFeatures::getRecentQueries(int maxQueries) const
{
    QMutexLocker locker(&d->historyMutex);
    
    QStringList queries;
    QSet<QString> uniqueQueries;
    
    for (const HistoryEntry& entry : d->searchHistory) {
        if (!uniqueQueries.contains(entry.query)) {
            queries.append(entry.query);
            uniqueQueries.insert(entry.query);
            
            if (queries.size() >= maxQueries) {
                break;
            }
        }
    }
    
    return queries;
}

QStringList SearchFeatures::getPopularQueries(int maxQueries) const
{
    QMutexLocker locker(&d->statisticsMutex);
    
    QList<QPair<QString, int>> queryPairs;
    for (auto it = d->statistics.queryFrequency.begin(); it != d->statistics.queryFrequency.end(); ++it) {
        queryPairs.append(qMakePair(it.key(), it.value()));
    }
    
    // Sort by frequency (descending)
    std::sort(queryPairs.begin(), queryPairs.end(), [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
        return a.second > b.second;
    });
    
    QStringList popularQueries;
    for (int i = 0; i < qMin(maxQueries, queryPairs.size()); ++i) {
        popularQueries.append(queryPairs[i].first);
    }
    
    return popularQueries;
}

void SearchFeatures::clearHistory()
{
    QMutexLocker locker(&d->historyMutex);
    d->searchHistory.clear();
    emit historyUpdated();
}

void SearchFeatures::removeHistoryEntry(int index)
{
    QMutexLocker locker(&d->historyMutex);
    
    if (index >= 0 && index < d->searchHistory.size()) {
        d->searchHistory.removeAt(index);
        emit historyUpdated();
    }
}

QStringList SearchFeatures::generateSuggestions(const QString& partialQuery, int maxSuggestions)
{
    QStringList suggestions = d->suggestionEngine->generateSuggestions(partialQuery, maxSuggestions);
    emit suggestionsReady(suggestions);
    return suggestions;
}

QStringList SearchFeatures::getQueryCompletions(const QString& prefix, int maxCompletions)
{
    return d->suggestionEngine->ngramSuggestions(prefix, 3, maxCompletions);
}

void SearchFeatures::updateSuggestionModel(const QStringList& corpus)
{
    QHash<QString, int> frequencies;
    for (const QString& text : corpus) {
        QStringList words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        for (const QString& word : words) {
            frequencies[word.toLower()]++;
        }
    }
    
    QStringList queries;
    QList<int> freqs;
    for (auto it = frequencies.begin(); it != frequencies.end(); ++it) {
        queries.append(it.key());
        freqs.append(it.value());
    }
    
    d->suggestionEngine->trainModel(queries, freqs);
}

QList<SearchResult> SearchFeatures::proximitySearch(const QString& text, const QStringList& terms,
                                                           const ProximitySearchOptions& options, int pageNumber)
{
    QList<SearchResult> results;

    if (terms.isEmpty()) {
        return results;
    }

    // Find all occurrences of each term
    QHash<QString, QList<int>> termPositions;

    for (const QString& term : terms) {
        QRegularExpression regex(QRegularExpression::escape(term),
                               options.caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);

        if (options.wholeWords) {
            regex.setPattern("\\b" + regex.pattern() + "\\b");
        }

        QRegularExpressionMatchIterator iterator = regex.globalMatch(text);
        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();
            termPositions[term].append(match.capturedStart());
        }
    }

    // Find proximity matches
    if (terms.size() == 1) {
        // Single term - return all matches
        for (int pos : termPositions[terms[0]]) {
            SearchResult result;
            result.pageNumber = pageNumber;
            result.matchedText = terms[0];
            result.textPosition = pos;
            result.textLength = terms[0].length();

            int contextStart = qMax(0, pos - 50);
            int contextEnd = qMin(text.length(), pos + terms[0].length() + 50);
            result.contextText = text.mid(contextStart, contextEnd - contextStart);

            results.append(result);
        }
    } else {
        // Multiple terms - check proximity
        QList<int> firstTermPositions = termPositions[terms[0]];

        for (int firstPos : firstTermPositions) {
            bool allTermsFound = true;
            int minPos = firstPos;
            int maxPos = firstPos + terms[0].length();

            for (int i = 1; i < terms.size(); ++i) {
                const QString& term = terms[i];
                QList<int> positions = termPositions[term];

                bool termFound = false;
                for (int pos : positions) {
                    int distance = qAbs(pos - firstPos);

                    if (distance <= options.maxDistance * 10) { // Approximate word distance
                        if (!options.ordered || pos > firstPos) {
                            termFound = true;
                            minPos = qMin(minPos, pos);
                            maxPos = qMax(maxPos, pos + term.length());
                            break;
                        }
                    }
                }

                if (!termFound) {
                    allTermsFound = false;
                    break;
                }
            }

            if (allTermsFound) {
                SearchResult result;
                result.pageNumber = pageNumber;
                result.textPosition = minPos;
                result.textLength = maxPos - minPos;
                result.matchedText = text.mid(minPos, result.textLength);

                int contextStart = qMax(0, minPos - 50);
                int contextEnd = qMin(text.length(), maxPos + 50);
                result.contextText = text.mid(contextStart, contextEnd - contextStart);

                results.append(result);
            }
        }
    }

    return results;
}

QList<SearchResult> SearchFeatures::filterResults(const QList<SearchResult>& results, const QString& filterCriteria)
{
    QList<SearchResult> filteredResults;

    QRegularExpression filterRegex(filterCriteria, QRegularExpression::CaseInsensitiveOption);

    for (const SearchResult& result : results) {
        if (filterRegex.match(result.matchedText).hasMatch() ||
            filterRegex.match(result.contextText).hasMatch()) {
            filteredResults.append(result);
        }
    }

    return filteredResults;
}

QList<SearchResult> SearchFeatures::sortResults(const QList<SearchResult>& results,
                                                       SortCriteria criteria, bool ascending)
{
    QList<SearchResult> sortedResults = results;

    std::sort(sortedResults.begin(), sortedResults.end(), [criteria, ascending](const SearchResult& a, const SearchResult& b) {
        bool result = false;

        switch (criteria) {
        case ByPosition:
            result = a.textPosition < b.textPosition;
            break;
        case ByPageNumber:
            result = a.pageNumber < b.pageNumber;
            break;
        case ByLength:
            result = a.textLength < b.textLength;
            break;
        case ByRelevance:
        default:
            // For relevance, we'd need additional scoring data
            result = a.textPosition < b.textPosition; // Fallback to position
            break;
        }

        return ascending ? result : !result;
    });

    return sortedResults;
}

SearchFeatures::SearchStatistics SearchFeatures::getSearchStatistics() const
{
    QMutexLocker locker(&d->statisticsMutex);

    SearchStatistics stats = d->statistics;
    stats.mostPopularQueries = getPopularQueries(10);

    return stats;
}

void SearchFeatures::resetStatistics()
{
    QMutexLocker locker(&d->statisticsMutex);
    d->statistics = SearchStatistics();
    emit statisticsUpdated(d->statistics);
}

bool SearchFeatures::exportSearchHistory(const QString& filePath) const
{
    QMutexLocker locker(&d->historyMutex);

    QJsonArray historyArray;
    for (const HistoryEntry& entry : d->searchHistory) {
        QJsonObject entryObj;
        entryObj["query"] = entry.query;
        entryObj["timestamp"] = entry.timestamp.toString(Qt::ISODate);
        entryObj["resultCount"] = entry.resultCount;
        entryObj["searchTime"] = static_cast<qint64>(entry.searchTime);
        entryObj["successful"] = entry.successful;

        // Add search options
        QJsonObject optionsObj;
        optionsObj["caseSensitive"] = entry.options.caseSensitive;
        optionsObj["wholeWords"] = entry.options.wholeWords;
        optionsObj["useRegex"] = entry.options.useRegex;
        optionsObj["fuzzySearch"] = entry.options.fuzzySearch;
        optionsObj["fuzzyThreshold"] = entry.options.fuzzyThreshold;
        entryObj["options"] = optionsObj;

        historyArray.append(entryObj);
    }

    QJsonDocument doc(historyArray);

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        return true;
    }

    return false;
}

bool SearchFeatures::importSearchHistory(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isArray()) {
        return false;
    }

    QMutexLocker locker(&d->historyMutex);
    d->searchHistory.clear();

    QJsonArray historyArray = doc.array();
    for (const QJsonValue& value : historyArray) {
        QJsonObject entryObj = value.toObject();

        HistoryEntry entry;
        entry.query = entryObj["query"].toString();
        entry.timestamp = QDateTime::fromString(entryObj["timestamp"].toString(), Qt::ISODate);
        entry.resultCount = entryObj["resultCount"].toInt();
        entry.searchTime = entryObj["searchTime"].toVariant().toLongLong();
        entry.successful = entryObj["successful"].toBool();

        // Load search options
        QJsonObject optionsObj = entryObj["options"].toObject();
        entry.options.caseSensitive = optionsObj["caseSensitive"].toBool();
        entry.options.wholeWords = optionsObj["wholeWords"].toBool();
        entry.options.useRegex = optionsObj["useRegex"].toBool();
        entry.options.fuzzySearch = optionsObj["fuzzySearch"].toBool();
        entry.options.fuzzyThreshold = optionsObj["fuzzyThreshold"].toInt();

        d->searchHistory.append(entry);
    }

    emit historyUpdated();
    return true;
}

QString SearchFeatures::exportSearchResults(const QList<SearchResult>& results, const QString& format) const
{
    if (format.toLower() == "json") {
        QJsonArray resultsArray;

        for (const SearchResult& result : results) {
            QJsonObject resultObj;
            resultObj["pageNumber"] = result.pageNumber;
            resultObj["matchedText"] = result.matchedText;
            resultObj["contextText"] = result.contextText;
            resultObj["textPosition"] = result.textPosition;
            resultObj["textLength"] = result.textLength;

            QJsonObject rectObj;
            rectObj["x"] = result.boundingRect.x();
            rectObj["y"] = result.boundingRect.y();
            rectObj["width"] = result.boundingRect.width();
            rectObj["height"] = result.boundingRect.height();
            resultObj["boundingRect"] = rectObj;

            resultsArray.append(resultObj);
        }

        QJsonDocument doc(resultsArray);
        return doc.toJson();
    } else if (format.toLower() == "csv") {
        QString csv = "Page,Position,Length,Text,Context\n";

        for (const SearchResult& result : results) {
            QString escapedText = result.matchedText;
            escapedText.replace("\"", "\"\"");
            QString escapedContext = result.contextText;
            escapedContext.replace("\"", "\"\"");

            csv += QString("%1,%2,%3,\"%4\",\"%5\"\n")
                   .arg(result.pageNumber)
                   .arg(result.textPosition)
                   .arg(result.textLength)
                   .arg(escapedText)
                   .arg(escapedContext);
        }

        return csv;
    } else {
        // Plain text format
        QString text;
        for (const SearchResult& result : results) {
            text += QString("Page %1: %2 (Position: %3)\n  Context: %4\n\n")
                    .arg(result.pageNumber + 1)
                    .arg(result.matchedText)
                    .arg(result.textPosition)
                    .arg(result.contextText);
        }
        return text;
    }
}

// FuzzySearchAlgorithms implementation
int FuzzySearchAlgorithms::levenshteinDistance(const QString& str1, const QString& str2)
{
    int len1 = str1.length();
    int len2 = str2.length();

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

            matrix[i][j] = qMin(qMin(matrix[i-1][j] + 1, matrix[i][j-1] + 1), matrix[i-1][j-1] + cost);
        }
    }

    return matrix[len1][len2];
}

int FuzzySearchAlgorithms::levenshteinDistanceOptimized(const QString& str1, const QString& str2, int maxDistance)
{
    int len1 = str1.length();
    int len2 = str2.length();

    // Early termination if length difference exceeds max distance
    if (qAbs(len1 - len2) > maxDistance) {
        return maxDistance + 1;
    }

    if (len1 == 0) return len2;
    if (len2 == 0) return len1;

    // Use only two rows instead of full matrix
    QVector<int> prevRow(len2 + 1);
    QVector<int> currRow(len2 + 1);

    // Initialize first row
    for (int j = 0; j <= len2; ++j) {
        prevRow[j] = j;
    }

    for (int i = 1; i <= len1; ++i) {
        currRow[0] = i;

        int minInRow = i;
        for (int j = 1; j <= len2; ++j) {
            int cost = (str1[i-1] == str2[j-1]) ? 0 : 1;

            currRow[j] = qMin(qMin(currRow[j-1] + 1, prevRow[j] + 1), prevRow[j-1] + cost);

            minInRow = qMin(minInRow, currRow[j]);
        }

        // Early termination if minimum in row exceeds max distance
        if (minInRow > maxDistance) {
            return maxDistance + 1;
        }

        prevRow = currRow;
    }

    return currRow[len2];
}

int FuzzySearchAlgorithms::damerauLevenshteinDistance(const QString& str1, const QString& str2)
{
    int len1 = str1.length();
    int len2 = str2.length();

    if (len1 == 0) return len2;
    if (len2 == 0) return len1;

    QVector<QVector<int>> matrix(len1 + 1, QVector<int>(len2 + 1));

    // Initialize
    for (int i = 0; i <= len1; ++i) {
        matrix[i][0] = i;
    }
    for (int j = 0; j <= len2; ++j) {
        matrix[0][j] = j;
    }

    for (int i = 1; i <= len1; ++i) {
        for (int j = 1; j <= len2; ++j) {
            int cost = (str1[i-1] == str2[j-1]) ? 0 : 1;

            matrix[i][j] = qMin(qMin(matrix[i-1][j] + 1, matrix[i][j-1] + 1), matrix[i-1][j-1] + cost);

            // Transposition
            if (i > 1 && j > 1 && str1[i-1] == str2[j-2] && str1[i-2] == str2[j-1]) {
                matrix[i][j] = qMin(matrix[i][j], matrix[i-2][j-2] + cost);
            }
        }
    }

    return matrix[len1][len2];
}

double FuzzySearchAlgorithms::jaroWinklerSimilarity(const QString& str1, const QString& str2)
{
    if (str1 == str2) return 1.0;

    int len1 = str1.length();
    int len2 = str2.length();

    if (len1 == 0 || len2 == 0) return 0.0;

    int matchWindow = qMax(len1, len2) / 2 - 1;
    if (matchWindow < 0) matchWindow = 0;

    QVector<bool> str1Matches(len1, false);
    QVector<bool> str2Matches(len2, false);

    int matches = 0;
    int transpositions = 0;

    // Find matches
    for (int i = 0; i < len1; ++i) {
        int start = qMax(0, i - matchWindow);
        int end = qMin(i + matchWindow + 1, len2);

        for (int j = start; j < end; ++j) {
            if (str2Matches[j] || str1[i] != str2[j]) continue;

            str1Matches[i] = true;
            str2Matches[j] = true;
            matches++;
            break;
        }
    }

    if (matches == 0) return 0.0;

    // Count transpositions
    int k = 0;
    for (int i = 0; i < len1; ++i) {
        if (!str1Matches[i]) continue;

        while (!str2Matches[k]) k++;

        if (str1[i] != str2[k]) transpositions++;
        k++;
    }

    double jaro = (static_cast<double>(matches) / len1 +
                   static_cast<double>(matches) / len2 +
                   static_cast<double>(matches - transpositions/2) / matches) / 3.0;

    // Winkler modification
    if (jaro < 0.7) return jaro;

    int prefix = 0;
    for (int i = 0; i < qMin(len1, len2) && i < 4; ++i) {
        if (str1[i] == str2[i]) prefix++;
        else break;
    }

    return jaro + (0.1 * prefix * (1.0 - jaro));
}

double FuzzySearchAlgorithms::ngramSimilarity(const QString& str1, const QString& str2, int n)
{
    if (str1 == str2) return 1.0;
    if (str1.isEmpty() || str2.isEmpty()) return 0.0;

    QSet<QString> ngrams1, ngrams2;

    // Generate n-grams for str1
    for (int i = 0; i <= str1.length() - n; ++i) {
        ngrams1.insert(str1.mid(i, n));
    }

    // Generate n-grams for str2
    for (int i = 0; i <= str2.length() - n; ++i) {
        ngrams2.insert(str2.mid(i, n));
    }

    QSet<QString> intersection = ngrams1 & ngrams2;
    QSet<QString> unionSet = ngrams1 | ngrams2;

    return unionSet.isEmpty() ? 0.0 : static_cast<double>(intersection.size()) / unionSet.size();
}

QString FuzzySearchAlgorithms::soundex(const QString& word)
{
    if (word.isEmpty()) return "0000";

    QString result = word[0].toUpper();
    QString code = "01230120022455012623010202";

    for (int i = 1; i < word.length() && result.length() < 4; ++i) {
        QChar ch = word[i].toUpper();
        if (ch.isLetter()) {
            int index = ch.unicode() - 'A';
            if (index >= 0 && index < 26) {
                QChar digit = code[index];
                if (digit != '0' && (result.isEmpty() || result.back() != digit)) {
                    result += digit;
                }
            }
        }
    }

    while (result.length() < 4) {
        result += '0';
    }

    return result.left(4);
}

bool FuzzySearchAlgorithms::soundexMatch(const QString& word1, const QString& word2)
{
    return soundex(word1) == soundex(word2);
}

// SearchHighlightEngine implementation
SearchHighlightEngine::SearchHighlightEngine()
{
    // Initialize default styles
    HighlightStyle defaultStyle;
    defaultStyle.backgroundColor = QColor("#FFFF00");
    defaultStyle.textColor = QColor("#000000");
    defaultStyle.borderColor = QColor("#CCCCCC");
    defaultStyle.borderWidth = 1;
    defaultStyle.opacity = 0.7;
    defaultStyle.pattern = "background-color: %1; color: %2; border: %3px solid %4; opacity: %5;";

    m_styles["default"] = defaultStyle;

    HighlightStyle currentStyle;
    currentStyle.backgroundColor = QColor("#FF6600");
    currentStyle.textColor = QColor("#FFFFFF");
    currentStyle.borderColor = QColor("#FF0000");
    currentStyle.borderWidth = 2;
    currentStyle.opacity = 0.9;
    currentStyle.pattern = "background-color: %1; color: %2; border: %3px solid %4; opacity: %5;";

    m_styles["current"] = currentStyle;
}

void SearchHighlightEngine::setHighlightStyle(const QString& styleName, const HighlightStyle& style)
{
    m_styles[styleName] = style;
}

SearchHighlightEngine::HighlightStyle SearchHighlightEngine::getHighlightStyle(const QString& styleName) const
{
    return m_styles.value(styleName, m_styles["default"]);
}

QList<SearchFeatures::HighlightInfo> SearchHighlightEngine::createHighlights(
    const QList<SearchResult>& results, const QString& styleName)
{
    QList<SearchFeatures::HighlightInfo> highlights;
    HighlightStyle style = getHighlightStyle(styleName);

    for (int i = 0; i < results.size(); ++i) {
        const SearchResult& result = results[i];

        SearchFeatures::HighlightInfo highlight;
        highlight.rect = result.boundingRect;
        highlight.color = style.backgroundColor;
        highlight.text = result.matchedText;
        highlight.priority = results.size() - i;
        highlight.isCurrentResult = false;

        highlights.append(highlight);
    }

    return highlights;
}

void SearchHighlightEngine::optimizeHighlights(QList<SearchFeatures::HighlightInfo>& highlights)
{
    // Remove duplicate highlights
    for (int i = highlights.size() - 1; i >= 0; --i) {
        for (int j = i - 1; j >= 0; --j) {
            if (highlights[i].rect == highlights[j].rect && highlights[i].text == highlights[j].text) {
                highlights.removeAt(i);
                break;
            }
        }
    }

    // Merge overlapping highlights
    mergeOverlappingHighlights(highlights);
}

void SearchHighlightEngine::mergeOverlappingHighlights(QList<SearchFeatures::HighlightInfo>& highlights)
{
    for (int i = highlights.size() - 1; i >= 0; --i) {
        for (int j = i - 1; j >= 0; --j) {
            if (highlights[i].rect.intersects(highlights[j].rect)) {
                // Merge the rectangles
                QRectF merged = highlights[i].rect.united(highlights[j].rect);
                highlights[j].rect = merged;
                highlights[j].text += " " + highlights[i].text;
                highlights[j].priority = qMax(highlights[i].priority, highlights[j].priority);
                highlights[j].isCurrentResult = highlights[i].isCurrentResult || highlights[j].isCurrentResult;

                highlights.removeAt(i);
                break;
            }
        }
    }
}
