#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include <QMutex>
#include <QThread>
#include <QThreadPool>
#include <QFuture>
#include <QElapsedTimer>
#include <QAtomicInt>
#include <memory>
#include <functional>
#include "SearchConfiguration.h"

class QRunnable;

/**
 * Search performance management framework
 * Provides intelligent algorithms, caching, and parallel processing
 */
class SearchPerformance : public QObject
{
    Q_OBJECT

public:
    explicit SearchPerformance(QObject* parent = nullptr);
    ~SearchPerformance();

    // Fast string matching algorithms
    struct FastSearchResult {
        int position;
        int length;
        double relevanceScore;
        QString context;
    };

    // Boyer-Moore algorithm implementation
    QList<FastSearchResult> boyerMooreSearch(const QString& text, const QString& pattern, 
                                           bool caseSensitive = false, int maxResults = -1);
    
    // KMP algorithm implementation  
    QList<FastSearchResult> kmpSearch(const QString& text, const QString& pattern,
                                    bool caseSensitive = false, int maxResults = -1);
    
    // Parallel search across multiple texts
    QList<FastSearchResult> parallelSearch(const QStringList& texts, const QString& pattern,
                                          const SearchOptions& options = SearchOptions());

    // Search result ranking and relevance scoring
    struct RankingFactors {
        double termFrequency = 1.0;
        double documentFrequency = 1.0;
        double positionWeight = 1.0;
        double contextRelevance = 1.0;
        double exactMatchBonus = 2.0;
        double proximityBonus = 1.5;
    };

    void setRankingFactors(const RankingFactors& factors);
    QList<SearchResult> rankResults(const QList<SearchResult>& results, const QString& query);
    double calculateRelevanceScore(const SearchResult& result, const QString& query, 
                                 const QString& fullText);

    // Query optimization
    struct QueryPlan {
        QString optimizedQuery;
        QStringList searchTerms;
        bool useParallelSearch;
        bool useFastAlgorithm;
        int estimatedCost;
        QString algorithm;
    };

    QueryPlan optimizeQuery(const QString& query, const SearchOptions& options, 
                          int documentSize, int pageCount);
    
    // Performance monitoring
    struct PerformanceMetrics {
        qint64 searchTime;
        qint64 algorithmTime;
        qint64 rankingTime;
        qint64 cacheTime;
        int resultsFound;
        int pagesSearched;
        QString algorithmUsed;
        double cacheHitRatio;
    };

    PerformanceMetrics getLastSearchMetrics() const;
    void resetMetrics();

    // Memory pool management
    void initializeMemoryPool(int poolSize = 1024 * 1024); // 1MB default
    void* allocateSearchMemory(size_t size);
    void deallocateSearchMemory(void* ptr);
    void clearMemoryPool();

    // Intelligent caching strategies
    void enablePredictiveCache(bool enabled);
    void warmupCache(const QStringList& commonQueries, const QStringList& texts);
    void preloadFrequentPatterns();
    
    // Cache performance optimization
    void optimizeCacheAccess(const QString& query);
    QStringList predictNextQueries(const QString& currentQuery, const QStringList& history);

    // Thread pool optimization
    void setOptimalThreadCount();
    void setThreadAffinity(bool enabled);
    void enableWorkStealing(bool enabled);

    // Search algorithm selection
    enum Algorithm {
        AutoSelect,
        BoyerMoore,
        KMP,
        Parallel,
        Hybrid
    };

    void setPreferredAlgorithm(Algorithm algorithm);
    Algorithm selectOptimalAlgorithm(const QString& pattern, int textSize);

signals:
    void optimizationCompleted(const PerformanceMetrics& metrics);
    void cacheWarmedUp(int entriesLoaded);
    void algorithmSelected(const QString& algorithm, const QString& reason);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};

/**
 * Parallel search task for thread pool execution
 */
class ParallelSearchTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    ParallelSearchTask(const QString& text, const QString& pattern, 
                      const SearchOptions& options, int pageNumber);
    
    void run() override;
    QList<SearchResult> getResults() const;

signals:
    void taskCompleted(int pageNumber, const QList<SearchResult>& results);

private:
    QString m_text;
    QString m_pattern;
    SearchOptions m_options;
    int m_pageNumber;
    QList<SearchResult> m_results;
};

/**
 * Memory pool for efficient search operations
 */
class SearchMemoryPool
{
public:
    SearchMemoryPool(size_t poolSize);
    ~SearchMemoryPool();

    void* allocate(size_t size);
    void deallocate(void* ptr);
    void clear();
    
    size_t getTotalSize() const { return m_poolSize; }
    size_t getUsedSize() const { return m_usedSize; }
    size_t getAvailableSize() const { return m_poolSize - m_usedSize; }

private:
    struct Block {
        void* ptr;
        size_t size;
        bool inUse;
    };

    char* m_pool;
    size_t m_poolSize;
    size_t m_usedSize;
    QList<Block> m_blocks;
    QMutex m_mutex;
};

/**
 * Intelligent cache predictor for search optimization
 */
class SearchCachePredictor
{
public:
    SearchCachePredictor();
    
    void recordQuery(const QString& query);
    void recordQuerySequence(const QStringList& queries);
    
    QStringList predictNextQueries(const QString& currentQuery, int maxPredictions = 5);
    QStringList getFrequentPatterns(int minFrequency = 3);
    
    void updatePredictionModel();
    void clearHistory();

private:
    struct QueryPattern {
        QString pattern;
        int frequency;
        QStringList followingQueries;
        double confidence;
    };

    QHash<QString, QueryPattern> m_patterns;
    QStringList m_queryHistory;
    QMutex m_mutex;
    
    void analyzePatterns();
    double calculateConfidence(const QueryPattern& pattern);
};

/**
 * Advanced search result ranker with multiple ranking algorithms
 */
class SearchResultRanker
{
public:
    SearchResultRanker();
    
    enum RankingAlgorithm {
        TfIdf,          // Term Frequency-Inverse Document Frequency
        BM25,           // Best Matching 25
        Cosine,         // Cosine Similarity
        Jaccard,        // Jaccard Similarity
        Hybrid          // Combination of multiple algorithms
    };

    void setRankingAlgorithm(RankingAlgorithm algorithm);
    QList<SearchResult> rankResults(const QList<SearchResult>& results, 
                                  const QString& query, const QStringList& corpus);
    
    double calculateTfIdf(const QString& term, const QString& document, 
                         const QStringList& corpus);
    double calculateBM25(const QString& query, const QString& document, 
                        const QStringList& corpus);
    double calculateCosineSimilarity(const QString& query, const QString& document);

private:
    RankingAlgorithm m_algorithm;
    QHash<QString, double> m_idfCache;
    
    QStringList tokenize(const QString& text);
    double calculateTermFrequency(const QString& term, const QStringList& tokens);
    double calculateInverseDocumentFrequency(const QString& term, const QStringList& corpus);
};
