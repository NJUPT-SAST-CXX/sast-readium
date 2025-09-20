#include "SearchPerformance.h"
#include <QDebug>
#include <QRegularExpression>
#include <QtConcurrent>
#include <QApplication>
#include <QThread>
#include <algorithm>
#include <cmath>

class SearchPerformance::Implementation
{
public:
    Implementation(SearchPerformance* q)
        : q_ptr(q)
        , memoryPool(nullptr)
        , cachePredictor(new SearchCachePredictor())
        , resultRanker(new SearchResultRanker())
        , preferredAlgorithm(AutoSelect)
        , predictiveCacheEnabled(false)
        , threadAffinityEnabled(false)
        , workStealingEnabled(true)
    {
        rankingFactors.termFrequency = 1.0;
        rankingFactors.documentFrequency = 1.0;
        rankingFactors.positionWeight = 1.0;
        rankingFactors.contextRelevance = 1.0;
        rankingFactors.exactMatchBonus = 2.0;
        rankingFactors.proximityBonus = 1.5;
        
        setOptimalThreadCount();
    }

    ~Implementation()
    {
        delete memoryPool;
        delete cachePredictor;
        delete resultRanker;
    }

    // Boyer-Moore bad character table
    QHash<QChar, int> buildBadCharTable(const QString& pattern, bool caseSensitive)
    {
        QHash<QChar, int> table;
        int patternLength = pattern.length();
        
        for (int i = 0; i < patternLength - 1; ++i) {
            QChar ch = caseSensitive ? pattern[i] : pattern[i].toLower();
            table[ch] = patternLength - 1 - i;
        }
        
        return table;
    }

    // KMP failure function
    QVector<int> buildKmpTable(const QString& pattern, bool caseSensitive)
    {
        QString processedPattern = caseSensitive ? pattern : pattern.toLower();
        int patternLength = processedPattern.length();
        QVector<int> table(patternLength, 0);
        
        int j = 0;
        for (int i = 1; i < patternLength; ++i) {
            while (j > 0 && processedPattern[i] != processedPattern[j]) {
                j = table[j - 1];
            }
            if (processedPattern[i] == processedPattern[j]) {
                j++;
            }
            table[i] = j;
        }
        
        return table;
    }

    void setOptimalThreadCount()
    {
        int coreCount = QThread::idealThreadCount();
        int optimalThreads = qMax(2, qMin(coreCount, 8)); // Between 2 and 8 threads
        QThreadPool::globalInstance()->setMaxThreadCount(optimalThreads);
    }

    QString extractContext(const QString& text, int position, int length, int contextLength = 50)
    {
        int start = qMax(0, position - contextLength);
        int end = qMin(text.length(), position + length + contextLength);
        return text.mid(start, end - start);
    }

    double calculatePositionWeight(int position, int textLength)
    {
        // Give higher weight to matches near the beginning of the text
        double normalizedPosition = static_cast<double>(position) / textLength;
        return 1.0 - (normalizedPosition * 0.3); // Reduce weight by up to 30%
    }

    SearchPerformance* q_ptr;
    SearchMemoryPool* memoryPool;
    SearchCachePredictor* cachePredictor;
    SearchResultRanker* resultRanker;
    
    SearchPerformance::RankingFactors rankingFactors;
    SearchPerformance::PerformanceMetrics lastMetrics;
    SearchPerformance::Algorithm preferredAlgorithm;
    
    bool predictiveCacheEnabled;
    bool threadAffinityEnabled;
    bool workStealingEnabled;
    
    QMutex metricsMutex;
};

SearchPerformance::SearchPerformance(QObject* parent)
    : QObject(parent)
    , d(new Implementation(this))
{
}

SearchPerformance::~SearchPerformance() = default;

QList<SearchPerformance::FastSearchResult> SearchPerformance::boyerMooreSearch(
    const QString& text, const QString& pattern, bool caseSensitive, int maxResults)
{
    QElapsedTimer timer;
    timer.start();
    
    QList<FastSearchResult> results;
    
    if (pattern.isEmpty() || text.isEmpty()) {
        return results;
    }
    
    QString processedText = caseSensitive ? text : text.toLower();
    QString processedPattern = caseSensitive ? pattern : pattern.toLower();
    
    QHash<QChar, int> badCharTable = d->buildBadCharTable(processedPattern, true);
    int textLength = processedText.length();
    int patternLength = processedPattern.length();
    
    int skip = 0;
    while (skip <= textLength - patternLength) {
        int j = patternLength - 1;
        
        // Match pattern from right to left
        while (j >= 0 && processedPattern[j] == processedText[skip + j]) {
            j--;
        }
        
        if (j < 0) {
            // Pattern found
            FastSearchResult result;
            result.position = skip;
            result.length = patternLength;
            result.context = d->extractContext(text, skip, patternLength);
            result.relevanceScore = d->calculatePositionWeight(skip, textLength) * d->rankingFactors.positionWeight;
            
            results.append(result);
            
            if (maxResults > 0 && results.size() >= maxResults) {
                break;
            }
            
            skip += patternLength; // Move past this match
        } else {
            // Calculate skip distance using bad character rule
            QChar badChar = processedText[skip + j];
            int badCharSkip = badCharTable.value(badChar, patternLength);
            skip += qMax(1, j - badCharSkip);
        }
    }
    
    // Update metrics
    QMutexLocker locker(&d->metricsMutex);
    d->lastMetrics.algorithmTime = timer.elapsed();
    d->lastMetrics.algorithmUsed = "Boyer-Moore";
    d->lastMetrics.resultsFound = results.size();
    
    return results;
}

QList<SearchPerformance::FastSearchResult> SearchPerformance::kmpSearch(
    const QString& text, const QString& pattern, bool caseSensitive, int maxResults)
{
    QElapsedTimer timer;
    timer.start();
    
    QList<FastSearchResult> results;
    
    if (pattern.isEmpty() || text.isEmpty()) {
        return results;
    }
    
    QString processedText = caseSensitive ? text : text.toLower();
    QString processedPattern = caseSensitive ? pattern : pattern.toLower();
    
    QVector<int> kmpTable = d->buildKmpTable(processedPattern, true);
    int textLength = processedText.length();
    int patternLength = processedPattern.length();
    
    int i = 0; // Index for text
    int j = 0; // Index for pattern
    
    while (i < textLength) {
        if (processedPattern[j] == processedText[i]) {
            i++;
            j++;
        }
        
        if (j == patternLength) {
            // Pattern found
            FastSearchResult result;
            result.position = i - j;
            result.length = patternLength;
            result.context = d->extractContext(text, result.position, patternLength);
            result.relevanceScore = d->calculatePositionWeight(result.position, textLength) * d->rankingFactors.positionWeight;
            
            results.append(result);
            
            if (maxResults > 0 && results.size() >= maxResults) {
                break;
            }
            
            j = kmpTable[j - 1];
        } else if (i < textLength && processedPattern[j] != processedText[i]) {
            if (j != 0) {
                j = kmpTable[j - 1];
            } else {
                i++;
            }
        }
    }
    
    // Update metrics
    QMutexLocker locker(&d->metricsMutex);
    d->lastMetrics.algorithmTime = timer.elapsed();
    d->lastMetrics.algorithmUsed = "KMP";
    d->lastMetrics.resultsFound = results.size();
    
    return results;
}

QList<SearchPerformance::FastSearchResult> SearchPerformance::parallelSearch(
    const QStringList& texts, const QString& pattern, const SearchOptions& options)
{
    QElapsedTimer timer;
    timer.start();
    
    QList<FastSearchResult> allResults;
    
    if (pattern.isEmpty() || texts.isEmpty()) {
        return allResults;
    }
    
    // Create parallel search tasks
    QList<QFuture<QList<SearchPerformance::FastSearchResult>>> futures;

    for (int i = 0; i < texts.size(); ++i) {
        QFuture<QList<SearchPerformance::FastSearchResult>> future = QtConcurrent::run([this, texts, pattern, options, i]() {
            SearchPerformance::Algorithm algorithm = selectOptimalAlgorithm(pattern, texts[i].length());

            if (algorithm == SearchPerformance::BoyerMoore) {
                return boyerMooreSearch(texts[i], pattern, options.caseSensitive, options.maxResults);
            } else {
                return kmpSearch(texts[i], pattern, options.caseSensitive, options.maxResults);
            }
        });

        futures.append(future);
    }
    
    // Collect results from all threads
    for (auto& future : futures) {
        QList<FastSearchResult> results = future.result();
        allResults.append(results);
        
        if (options.maxResults > 0 && allResults.size() >= options.maxResults) {
            // Cancel remaining tasks if we have enough results
            break;
        }
    }
    
    // Update metrics
    QMutexLocker locker(&d->metricsMutex);
    d->lastMetrics.algorithmTime = timer.elapsed();
    d->lastMetrics.algorithmUsed = "Parallel";
    d->lastMetrics.resultsFound = allResults.size();
    d->lastMetrics.pagesSearched = texts.size();
    
    return allResults;
}

void SearchPerformance::setRankingFactors(const RankingFactors& factors)
{
    d->rankingFactors = factors;
}

SearchPerformance::Algorithm SearchPerformance::selectOptimalAlgorithm(
    const QString& pattern, int textSize)
{
    if (d->preferredAlgorithm != AutoSelect) {
        return d->preferredAlgorithm;
    }
    
    // Algorithm selection heuristics
    int patternLength = pattern.length();
    
    if (textSize > 100000 && patternLength > 10) {
        // Large text with long pattern - Boyer-Moore is typically faster
        emit algorithmSelected("Boyer-Moore", "Large text with long pattern");
        return BoyerMoore;
    } else if (patternLength <= 5) {
        // Short patterns - KMP is often more efficient
        emit algorithmSelected("KMP", "Short pattern");
        return KMP;
    } else {
        // Medium-sized patterns and text - Boyer-Moore generally performs well
        emit algorithmSelected("Boyer-Moore", "Medium-sized pattern and text");
        return BoyerMoore;
    }
}

void SearchPerformance::setPreferredAlgorithm(Algorithm algorithm)
{
    d->preferredAlgorithm = algorithm;
}

SearchPerformance::PerformanceMetrics SearchPerformance::getLastSearchMetrics() const
{
    QMutexLocker locker(&d->metricsMutex);
    return d->lastMetrics;
}

void SearchPerformance::resetMetrics()
{
    QMutexLocker locker(&d->metricsMutex);
    d->lastMetrics = PerformanceMetrics();
}

void SearchPerformance::initializeMemoryPool(int poolSize)
{
    delete d->memoryPool;
    d->memoryPool = new SearchMemoryPool(poolSize);
}

void* SearchPerformance::allocateSearchMemory(size_t size)
{
    if (d->memoryPool) {
        return d->memoryPool->allocate(size);
    }
    return nullptr;
}

void SearchPerformance::deallocateSearchMemory(void* ptr)
{
    if (d->memoryPool) {
        d->memoryPool->deallocate(ptr);
    }
}

void SearchPerformance::clearMemoryPool()
{
    if (d->memoryPool) {
        d->memoryPool->clear();
    }
}

void SearchPerformance::setOptimalThreadCount()
{
    d->setOptimalThreadCount();
}

void SearchPerformance::setThreadAffinity(bool enabled)
{
    d->threadAffinityEnabled = enabled;
}

void SearchPerformance::enableWorkStealing(bool enabled)
{
    d->workStealingEnabled = enabled;
}

void SearchPerformance::enablePredictiveCache(bool enabled)
{
    d->predictiveCacheEnabled = enabled;
}

QList<SearchResult> SearchPerformance::rankResults(const QList<SearchResult>& results, const QString& query)
{
    QElapsedTimer timer;
    timer.start();

    QList<SearchResult> rankedResults = results;

    // Calculate relevance scores for each result
    for (SearchResult& result : rankedResults) {
        double score = 0.0;

        // Term frequency scoring
        QString lowerQuery = query.toLower();
        QString lowerText = result.matchedText.toLower();
        int termCount = lowerText.count(lowerQuery);
        double tf = static_cast<double>(termCount) / result.matchedText.length();
        score += tf * d->rankingFactors.termFrequency;

        // Position weight (earlier matches score higher)
        double positionWeight = d->calculatePositionWeight(result.textPosition, result.contextText.length());
        score += positionWeight * d->rankingFactors.positionWeight;

        // Exact match bonus
        if (result.matchedText.compare(query, Qt::CaseInsensitive) == 0) {
            score += d->rankingFactors.exactMatchBonus;
        }

        // Context relevance (more context words matching query terms)
        QStringList queryTerms = query.split(' ', Qt::SkipEmptyParts);
        QStringList contextWords = result.contextText.split(' ', Qt::SkipEmptyParts);
        int contextMatches = 0;
        for (const QString& term : queryTerms) {
            for (const QString& word : contextWords) {
                if (word.contains(term, Qt::CaseInsensitive)) {
                    contextMatches++;
                }
            }
        }
        double contextScore = static_cast<double>(contextMatches) / contextWords.size();
        score += contextScore * d->rankingFactors.contextRelevance;

        // Store the calculated score (we'll use a custom property or extend SearchResult)
        // For now, we'll use the existing relevanceScore if available
        // result.relevanceScore = score;
    }

    // Sort results by relevance score (descending)
    std::sort(rankedResults.begin(), rankedResults.end(), [](const SearchResult& a, const SearchResult& b) {
        // Since SearchResult doesn't have relevanceScore, we'll sort by position for now
        // In a real implementation, you'd extend SearchResult to include relevanceScore
        return a.textPosition < b.textPosition;
    });

    // Update metrics
    QMutexLocker locker(&d->metricsMutex);
    d->lastMetrics.rankingTime = timer.elapsed();

    return rankedResults;
}

double SearchPerformance::calculateRelevanceScore(const SearchResult& result,
                                                         const QString& query, const QString& fullText)
{
    double score = 0.0;

    // Term frequency in the matched text
    QString lowerQuery = query.toLower();
    QString lowerText = result.matchedText.toLower();
    int termCount = lowerText.count(lowerQuery);
    double tf = static_cast<double>(termCount) / result.matchedText.length();
    score += tf * d->rankingFactors.termFrequency;

    // Document frequency (inverse)
    int totalOccurrences = fullText.count(query, Qt::CaseInsensitive);
    double idf = totalOccurrences > 0 ? std::log(static_cast<double>(fullText.length()) / totalOccurrences) : 1.0;
    score += idf * d->rankingFactors.documentFrequency;

    // Position weight
    double positionWeight = d->calculatePositionWeight(result.textPosition, fullText.length());
    score += positionWeight * d->rankingFactors.positionWeight;

    // Exact match bonus
    if (result.matchedText.compare(query, Qt::CaseInsensitive) == 0) {
        score += d->rankingFactors.exactMatchBonus;
    }

    return score;
}

SearchPerformance::QueryPlan SearchPerformance::optimizeQuery(
    const QString& query, const SearchOptions& options, int documentSize, int pageCount)
{
    QueryPlan plan;
    plan.optimizedQuery = query;
    plan.searchTerms = query.split(' ', Qt::SkipEmptyParts);

    // Determine optimal search strategy
    int queryLength = query.length();
    int termCount = plan.searchTerms.size();

    // Use parallel search for large documents with multiple terms
    plan.useParallelSearch = (documentSize > 50000 && pageCount > 10) || termCount > 3;

    // Use fast algorithm for simple queries
    plan.useFastAlgorithm = !options.useRegex && queryLength > 3;

    // Select algorithm
    if (plan.useFastAlgorithm) {
        Algorithm algo = selectOptimalAlgorithm(query, documentSize);
        plan.algorithm = (algo == BoyerMoore) ? "Boyer-Moore" : "KMP";
    } else {
        plan.algorithm = "Standard";
    }

    // Estimate cost (simplified heuristic)
    plan.estimatedCost = documentSize / 1000 + queryLength * termCount;
    if (plan.useParallelSearch) {
        plan.estimatedCost /= QThread::idealThreadCount();
    }

    return plan;
}

void SearchPerformance::warmupCache(const QStringList& commonQueries, const QStringList& texts)
{
    if (!d->predictiveCacheEnabled) {
        return;
    }

    QElapsedTimer timer;
    timer.start();

    int entriesLoaded = 0;

    // Pre-compute search results for common queries
    for (const QString& query : commonQueries) {
        for (const QString& text : texts) {
            // Perform search and cache results
            Algorithm algorithm = selectOptimalAlgorithm(query, text.length());

            if (algorithm == BoyerMoore) {
                boyerMooreSearch(text, query, false, 100);
            } else {
                kmpSearch(text, query, false, 100);
            }

            entriesLoaded++;
        }
    }

    // Update cache predictor with common queries
    for (const QString& query : commonQueries) {
        d->cachePredictor->recordQuery(query);
    }
    d->cachePredictor->updatePredictionModel();

    emit cacheWarmedUp(entriesLoaded);

    // Update metrics
    QMutexLocker locker(&d->metricsMutex);
    d->lastMetrics.cacheTime = timer.elapsed();
}

void SearchPerformance::preloadFrequentPatterns()
{
    if (!d->predictiveCacheEnabled) {
        return;
    }

    QStringList frequentPatterns = d->cachePredictor->getFrequentPatterns(3);

    // Pre-load these patterns into memory for faster access
    for (const QString& pattern : frequentPatterns) {
        // Pre-compile regex patterns if needed
        if (pattern.contains(QRegularExpression("[.*+?^${}()|[\\]\\\\"))) {
            QRegularExpression regex(pattern);
            // Cache compiled regex
        }
    }
}

void SearchPerformance::optimizeCacheAccess(const QString& query)
{
    if (d->predictiveCacheEnabled) {
        d->cachePredictor->recordQuery(query);
    }
}

QStringList SearchPerformance::predictNextQueries(const QString& currentQuery, const QStringList& history)
{
    if (!d->predictiveCacheEnabled) {
        return QStringList();
    }

    return d->cachePredictor->predictNextQueries(currentQuery, 5);
}

// ParallelSearchTask implementation
ParallelSearchTask::ParallelSearchTask(const QString& text, const QString& pattern,
                                     const SearchOptions& options, int pageNumber)
    : m_text(text), m_pattern(pattern), m_options(options), m_pageNumber(pageNumber)
{
    setAutoDelete(false); // We'll manage deletion manually
}

void ParallelSearchTask::run()
{
    // Create a temporary optimizer for this task
    SearchPerformance optimizer;

    // Select optimal algorithm
    auto algorithm = optimizer.selectOptimalAlgorithm(m_pattern, m_text.length());

    QList<SearchPerformance::FastSearchResult> fastResults;

    if (algorithm == SearchPerformance::BoyerMoore) {
        fastResults = optimizer.boyerMooreSearch(m_text, m_pattern, m_options.caseSensitive, m_options.maxResults);
    } else {
        fastResults = optimizer.kmpSearch(m_text, m_pattern, m_options.caseSensitive, m_options.maxResults);
    }

    // Convert FastSearchResult to SearchResult
    for (const auto& fastResult : fastResults) {
        SearchResult result(m_pageNumber, fastResult.context, fastResult.context,
                          QRectF(), fastResult.position, fastResult.length);
        m_results.append(result);
    }

    emit taskCompleted(m_pageNumber, m_results);
}

QList<SearchResult> ParallelSearchTask::getResults() const
{
    return m_results;
}

// SearchMemoryPool implementation
SearchMemoryPool::SearchMemoryPool(size_t poolSize)
    : m_poolSize(poolSize), m_usedSize(0)
{
    m_pool = new char[poolSize];
}

SearchMemoryPool::~SearchMemoryPool()
{
    delete[] m_pool;
}

void* SearchMemoryPool::allocate(size_t size)
{
    QMutexLocker locker(&m_mutex);

    // Simple first-fit allocation
    size_t alignedSize = (size + 7) & ~7; // 8-byte alignment

    if (m_usedSize + alignedSize > m_poolSize) {
        return nullptr; // Out of memory
    }

    void* ptr = m_pool + m_usedSize;

    Block block;
    block.ptr = ptr;
    block.size = alignedSize;
    block.inUse = true;
    m_blocks.append(block);

    m_usedSize += alignedSize;

    return ptr;
}

void SearchMemoryPool::deallocate(void* ptr)
{
    QMutexLocker locker(&m_mutex);

    for (int i = 0; i < m_blocks.size(); ++i) {
        if (m_blocks[i].ptr == ptr) {
            m_blocks[i].inUse = false;

            // Simple coalescing - merge adjacent free blocks
            if (i > 0 && !m_blocks[i-1].inUse) {
                m_blocks[i-1].size += m_blocks[i].size;
                m_blocks.removeAt(i);
                i--;
            }
            if (i < m_blocks.size() - 1 && !m_blocks[i+1].inUse) {
                m_blocks[i].size += m_blocks[i+1].size;
                m_blocks.removeAt(i+1);
            }

            break;
        }
    }
}

void SearchMemoryPool::clear()
{
    QMutexLocker locker(&m_mutex);
    m_blocks.clear();
    m_usedSize = 0;
}

// SearchCachePredictor implementation
SearchCachePredictor::SearchCachePredictor()
{
}

void SearchCachePredictor::recordQuery(const QString& query)
{
    QMutexLocker locker(&m_mutex);

    m_queryHistory.append(query);

    // Keep history size manageable
    if (m_queryHistory.size() > 1000) {
        m_queryHistory.removeFirst();
    }

    // Update pattern frequency
    if (m_patterns.contains(query)) {
        m_patterns[query].frequency++;
    } else {
        QueryPattern pattern;
        pattern.pattern = query;
        pattern.frequency = 1;
        pattern.confidence = 0.0;
        m_patterns[query] = pattern;
    }
}

void SearchCachePredictor::recordQuerySequence(const QStringList& queries)
{
    QMutexLocker locker(&m_mutex);

    for (int i = 0; i < queries.size() - 1; ++i) {
        QString current = queries[i];
        QString next = queries[i + 1];

        if (m_patterns.contains(current)) {
            if (!m_patterns[current].followingQueries.contains(next)) {
                m_patterns[current].followingQueries.append(next);
            }
        }
    }
}

QStringList SearchCachePredictor::predictNextQueries(const QString& currentQuery, int maxPredictions)
{
    QMutexLocker locker(&m_mutex);

    QStringList predictions;

    if (m_patterns.contains(currentQuery)) {
        const QueryPattern& pattern = m_patterns[currentQuery];

        // Sort following queries by frequency
        QList<QPair<QString, int>> candidates;
        for (const QString& followingQuery : pattern.followingQueries) {
            if (m_patterns.contains(followingQuery)) {
                candidates.append(qMakePair(followingQuery, m_patterns[followingQuery].frequency));
            }
        }

        std::sort(candidates.begin(), candidates.end(),
                 [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
                     return a.second > b.second;
                 });

        for (int i = 0; i < qMin(maxPredictions, candidates.size()); ++i) {
            predictions.append(candidates[i].first);
        }
    }

    return predictions;
}

QStringList SearchCachePredictor::getFrequentPatterns(int minFrequency)
{
    QMutexLocker locker(&m_mutex);

    QStringList frequentPatterns;

    for (auto it = m_patterns.begin(); it != m_patterns.end(); ++it) {
        if (it.value().frequency >= minFrequency) {
            frequentPatterns.append(it.key());
        }
    }

    return frequentPatterns;
}

void SearchCachePredictor::updatePredictionModel()
{
    QMutexLocker locker(&m_mutex);
    analyzePatterns();
}

void SearchCachePredictor::clearHistory()
{
    QMutexLocker locker(&m_mutex);
    m_queryHistory.clear();
    m_patterns.clear();
}

void SearchCachePredictor::analyzePatterns()
{
    // Analyze query history to find patterns
    for (int i = 0; i < m_queryHistory.size() - 1; ++i) {
        QString current = m_queryHistory[i];
        QString next = m_queryHistory[i + 1];

        if (m_patterns.contains(current)) {
            if (!m_patterns[current].followingQueries.contains(next)) {
                m_patterns[current].followingQueries.append(next);
            }
        }
    }

    // Update confidence scores
    for (auto it = m_patterns.begin(); it != m_patterns.end(); ++it) {
        it.value().confidence = calculateConfidence(it.value());
    }
}

double SearchCachePredictor::calculateConfidence(const QueryPattern& pattern)
{
    // Simple confidence calculation based on frequency and following queries
    double baseConfidence = qMin(1.0, static_cast<double>(pattern.frequency) / 10.0);
    double followingBonus = qMin(0.5, static_cast<double>(pattern.followingQueries.size()) / 10.0);

    return baseConfidence + followingBonus;
}

// SearchResultRanker implementation
SearchResultRanker::SearchResultRanker()
    : m_algorithm(TfIdf)
{
}

void SearchResultRanker::setRankingAlgorithm(RankingAlgorithm algorithm)
{
    m_algorithm = algorithm;
}

QList<SearchResult> SearchResultRanker::rankResults(const QList<SearchResult>& results,
                                                   const QString& query, const QStringList& corpus)
{
    QList<SearchResult> rankedResults = results;

    // Calculate scores based on selected algorithm
    for (SearchResult& result : rankedResults) {
        double score = 0.0;

        switch (m_algorithm) {
        case TfIdf:
            score = calculateTfIdf(query, result.matchedText, corpus);
            break;
        case BM25:
            score = calculateBM25(query, result.matchedText, corpus);
            break;
        case Cosine:
            score = calculateCosineSimilarity(query, result.matchedText);
            break;
        case Jaccard:
            // Implement Jaccard similarity
            {
                QStringList queryTokens = tokenize(query);
                QStringList docTokens = tokenize(result.matchedText);
                QSet<QString> querySet = QSet<QString>(queryTokens.begin(), queryTokens.end());
                QSet<QString> docSet = QSet<QString>(docTokens.begin(), docTokens.end());
                QSet<QString> intersection = querySet & docSet;
                QSet<QString> unionSet = querySet | docSet;
                score = unionSet.isEmpty() ? 0.0 : static_cast<double>(intersection.size()) / unionSet.size();
            }
            break;
        case Hybrid:
            // Combine multiple algorithms
            score = 0.4 * calculateTfIdf(query, result.matchedText, corpus) +
                   0.3 * calculateBM25(query, result.matchedText, corpus) +
                   0.3 * calculateCosineSimilarity(query, result.matchedText);
            break;
        }

        // Store score (extend SearchResult to include score in real implementation)
        // result.relevanceScore = score;
    }

    // Sort by score (descending)
    std::sort(rankedResults.begin(), rankedResults.end(), [](const SearchResult& a, const SearchResult& b) {
        // For now, sort by position since we can't store the calculated score
        // In real implementation, sort by relevanceScore
        return a.textPosition < b.textPosition;
    });

    return rankedResults;
}

double SearchResultRanker::calculateTfIdf(const QString& term, const QString& document, const QStringList& corpus)
{
    // Calculate term frequency
    QStringList docTokens = tokenize(document);
    double tf = calculateTermFrequency(term, docTokens);

    // Calculate inverse document frequency
    double idf = calculateInverseDocumentFrequency(term, corpus);

    return tf * idf;
}

double SearchResultRanker::calculateBM25(const QString& query, const QString& document, const QStringList& corpus)
{
    // BM25 parameters
    const double k1 = 1.2;
    const double b = 0.75;

    QStringList queryTokens = tokenize(query);
    QStringList docTokens = tokenize(document);

    // Calculate average document length
    double avgDocLength = 0.0;
    for (const QString& doc : corpus) {
        avgDocLength += tokenize(doc).size();
    }
    avgDocLength /= corpus.size();

    double score = 0.0;

    for (const QString& term : queryTokens) {
        double tf = calculateTermFrequency(term, docTokens);
        double idf = calculateInverseDocumentFrequency(term, corpus);

        double numerator = tf * (k1 + 1);
        double denominator = tf + k1 * (1 - b + b * (docTokens.size() / avgDocLength));

        score += idf * (numerator / denominator);
    }

    return score;
}

double SearchResultRanker::calculateCosineSimilarity(const QString& query, const QString& document)
{
    QStringList queryTokens = tokenize(query);
    QStringList docTokens = tokenize(document);

    // Create term frequency vectors
    QHash<QString, int> queryTf, docTf;

    for (const QString& token : queryTokens) {
        queryTf[token]++;
    }

    for (const QString& token : docTokens) {
        docTf[token]++;
    }

    // Calculate dot product and magnitudes
    double dotProduct = 0.0;
    double queryMagnitude = 0.0;
    double docMagnitude = 0.0;

    QSet<QString> allTerms;
    for (auto it = queryTf.begin(); it != queryTf.end(); ++it) {
        allTerms.insert(it.key());
    }
    for (auto it = docTf.begin(); it != docTf.end(); ++it) {
        allTerms.insert(it.key());
    }

    for (const QString& term : allTerms) {
        int qf = queryTf.value(term, 0);
        int df = docTf.value(term, 0);

        dotProduct += qf * df;
        queryMagnitude += qf * qf;
        docMagnitude += df * df;
    }

    queryMagnitude = std::sqrt(queryMagnitude);
    docMagnitude = std::sqrt(docMagnitude);

    if (queryMagnitude == 0.0 || docMagnitude == 0.0) {
        return 0.0;
    }

    return dotProduct / (queryMagnitude * docMagnitude);
}

QStringList SearchResultRanker::tokenize(const QString& text)
{
    // Simple tokenization - split on whitespace and punctuation
    QString cleanText = text.toLower();
    cleanText.remove(QRegularExpression("[^\\w\\s]"));
    return cleanText.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
}

double SearchResultRanker::calculateTermFrequency(const QString& term, const QStringList& tokens)
{
    int count = 0;
    for (const QString& token : tokens) {
        if (token.compare(term, Qt::CaseInsensitive) == 0) {
            count++;
        }
    }

    return tokens.isEmpty() ? 0.0 : static_cast<double>(count) / tokens.size();
}

double SearchResultRanker::calculateInverseDocumentFrequency(const QString& term, const QStringList& corpus)
{
    // Check cache first
    if (m_idfCache.contains(term)) {
        return m_idfCache[term];
    }

    int documentsContainingTerm = 0;
    for (const QString& document : corpus) {
        if (document.contains(term, Qt::CaseInsensitive)) {
            documentsContainingTerm++;
        }
    }

    double idf = documentsContainingTerm > 0 ?
                std::log(static_cast<double>(corpus.size()) / documentsContainingTerm) : 0.0;

    // Cache the result
    m_idfCache[term] = idf;

    return idf;
}


