#pragma once

#include <QColor>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QRectF>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <memory>
#include "SearchConfiguration.h"

/**
 * Search features implementation
 * Provides fuzzy search, highlighting, history, and other extended
 * functionality
 */
class SearchFeatures : public QObject {
    Q_OBJECT

public:
    explicit SearchFeatures(QObject* parent = nullptr);
    ~SearchFeatures();

    // Fuzzy search algorithms
    struct FuzzyMatch {
        QString text;
        int position;
        int length;
        int editDistance;
        double similarity;
        QString context;
    };

    QList<FuzzyMatch> fuzzySearch(const QString& text, const QString& pattern,
                                  int maxDistance = 2, int maxResults = -1);

    int calculateLevenshteinDistance(const QString& str1, const QString& str2);
    double calculateSimilarity(const QString& str1, const QString& str2);

    // Advanced pattern matching
    QList<SearchResult> wildcardSearch(const QString& text,
                                       const QString& pattern,
                                       int pageNumber = 0);
    QList<SearchResult> phraseSearch(const QString& text, const QString& phrase,
                                     int pageNumber = 0, int proximity = 0);
    QList<SearchResult> booleanSearch(const QString& text, const QString& query,
                                      int pageNumber = 0);

    // Search highlighting
    struct HighlightInfo {
        QRectF rect;
        QColor color;
        QString text;
        int priority;
        bool isCurrentResult;
    };

    void setHighlightColors(const QColor& normalColor,
                            const QColor& currentColor);
    QColor getNormalHighlightColor() const;
    QColor getCurrentHighlightColor() const;

    QList<HighlightInfo> generateHighlights(const QList<SearchResult>& results,
                                            int currentResultIndex = -1);
    void updateHighlightPriorities(QList<HighlightInfo>& highlights);

    // Search history management
    struct HistoryEntry {
        QString query;
        SearchOptions options;
        QDateTime timestamp;
        int resultCount;
        qint64 searchTime;
        bool successful;
    };

    void addToHistory(const QString& query, const SearchOptions& options,
                      int resultCount, qint64 searchTime,
                      bool successful = true);
    QList<HistoryEntry> getSearchHistory(int maxEntries = 50) const;
    QStringList getRecentQueries(int maxQueries = 10) const;
    QStringList getPopularQueries(int maxQueries = 10) const;
    void clearHistory();
    void removeHistoryEntry(int index);

    // Search suggestions and auto-completion
    QStringList generateSuggestions(const QString& partialQuery,
                                    int maxSuggestions = 5);
    QStringList getQueryCompletions(const QString& prefix,
                                    int maxCompletions = 10);
    void updateSuggestionModel(const QStringList& corpus);

    // Advanced search options
    struct ProximitySearchOptions {
        int maxDistance = 10;  // Maximum word distance
        bool ordered = false;  // Whether words must appear in order
        bool caseSensitive = false;
        bool wholeWords = true;
    };

    QList<SearchResult> proximitySearch(const QString& text,
                                        const QStringList& terms,
                                        const ProximitySearchOptions& options,
                                        int pageNumber = 0);

    // Search result filtering and sorting
    enum SortCriteria {
        ByRelevance,
        ByPosition,
        ByPageNumber,
        ByTimestamp,
        ByLength
    };

    QList<SearchResult> filterResults(const QList<SearchResult>& results,
                                      const QString& filterCriteria);
    QList<SearchResult> sortResults(const QList<SearchResult>& results,
                                    SortCriteria criteria,
                                    bool ascending = true);

    // Search statistics and analytics
    struct SearchStatistics {
        int totalSearches;
        int successfulSearches;
        double averageSearchTime;
        double averageResultCount;
        QStringList mostPopularQueries;
        QHash<QString, int> queryFrequency;
        QDateTime lastSearchTime;
    };

    SearchStatistics getSearchStatistics() const;
    void resetStatistics();

    // Export and import functionality
    bool exportSearchHistory(const QString& filePath) const;
    bool importSearchHistory(const QString& filePath);
    QString exportSearchResults(const QList<SearchResult>& results,
                                const QString& format = "json") const;

signals:
    void fuzzySearchCompleted(const QList<FuzzyMatch>& matches);
    void highlightsGenerated(const QList<HighlightInfo>& highlights);
    void historyUpdated();
    void suggestionsReady(const QStringList& suggestions);
    void statisticsUpdated(const SearchStatistics& stats);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};

/**
 * Fuzzy search algorithm implementations
 */
class FuzzySearchAlgorithms {
public:
    // Levenshtein distance with optimizations
    static int levenshteinDistance(const QString& str1, const QString& str2);
    static int levenshteinDistanceOptimized(const QString& str1,
                                            const QString& str2,
                                            int maxDistance);

    // Damerau-Levenshtein distance (handles transpositions)
    static int damerauLevenshteinDistance(const QString& str1,
                                          const QString& str2);

    // Jaro-Winkler similarity
    static double jaroWinklerSimilarity(const QString& str1,
                                        const QString& str2);

    // N-gram similarity
    static double ngramSimilarity(const QString& str1, const QString& str2,
                                  int n = 2);

    // Soundex algorithm for phonetic matching
    static QString soundex(const QString& word);
    static bool soundexMatch(const QString& word1, const QString& word2);
};

/**
 * Search highlighting engine
 */
class SearchHighlightEngine {
public:
    SearchHighlightEngine();

    struct HighlightStyle {
        QColor backgroundColor;
        QColor textColor;
        QColor borderColor;
        int borderWidth;
        double opacity;
        QString pattern;  // CSS-like pattern for custom styling
    };

    void setHighlightStyle(const QString& styleName,
                           const HighlightStyle& style);
    HighlightStyle getHighlightStyle(const QString& styleName) const;

    QList<SearchFeatures::HighlightInfo> createHighlights(
        const QList<SearchResult>& results,
        const QString& styleName = "default");

    void optimizeHighlights(QList<SearchFeatures::HighlightInfo>& highlights);
    void mergeOverlappingHighlights(
        QList<SearchFeatures::HighlightInfo>& highlights);

private:
    QHash<QString, HighlightStyle> m_styles;
};

/**
 * Search suggestion engine with machine learning capabilities
 */
class SearchSuggestionEngine {
public:
    SearchSuggestionEngine();
    ~SearchSuggestionEngine();

    // Explicit copy/move semantics
    SearchSuggestionEngine(const SearchSuggestionEngine& other);
    SearchSuggestionEngine& operator=(const SearchSuggestionEngine& other);
    SearchSuggestionEngine(SearchSuggestionEngine&& other) noexcept;
    SearchSuggestionEngine& operator=(SearchSuggestionEngine&& other) noexcept;

    void trainModel(const QStringList& queries, const QList<int>& frequencies);
    QStringList generateSuggestions(const QString& partialQuery,
                                    int maxSuggestions = 5);

    void addQueryToModel(const QString& query, int frequency = 1);
    void updateQueryFrequency(const QString& query, int frequency);
    int getQueryFrequency(const QString& query) const;
    QStringList getMostFrequentQueries(int count = 10) const;

    // N-gram based suggestions
    QStringList ngramSuggestions(const QString& partialQuery, int n = 3,
                                 int maxSuggestions = 5);

    // Fuzzy matching suggestions
    QStringList fuzzySuggestions(const QString& partialQuery,
                                 int maxDistance = 2, int maxSuggestions = 5);

    // Context-aware suggestions
    QStringList contextualSuggestions(const QString& partialQuery,
                                      const QStringList& context,
                                      int maxSuggestions = 5);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};

/**
 * Boolean search query parser and executor
 */
class BooleanSearchParser {
public:
    enum Operator { AND, OR, NOT, NEAR, PHRASE };

    struct QueryNode {
        QString term;
        Operator op;
        std::shared_ptr<QueryNode> left;
        std::shared_ptr<QueryNode> right;
        int proximity;  // For NEAR operator
    };

    std::shared_ptr<QueryNode> parseQuery(const QString& query);
    QList<SearchResult> executeQuery(std::shared_ptr<QueryNode> root,
                                     const QString& text, int pageNumber = 0);

private:
    QStringList tokenize(const QString& query);
    std::shared_ptr<QueryNode> parseExpression(const QStringList& tokens,
                                               int& index);
    std::shared_ptr<QueryNode> parseTerm(const QStringList& tokens, int& index);

    QList<SearchResult> evaluateNode(std::shared_ptr<QueryNode> node,
                                     const QString& text, int pageNumber);
    QList<SearchResult> combineResults(const QList<SearchResult>& left,
                                       const QList<SearchResult>& right,
                                       Operator op);
};

// Type alias for backward compatibility with tests
using AdvancedSearchFeatures = SearchFeatures;
