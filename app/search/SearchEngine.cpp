#include "SearchEngine.h"
#include "SearchConfiguration.h"
#include "TextExtractor.h"
#include "SearchExecutor.h"
#include "BackgroundProcessor.h"
#include "IncrementalSearchManager.h"
#include "SearchMetrics.h"
#include "MemoryManager.h"
#include "SearchValidator.h"
#include "SearchErrorRecovery.h"
#include "SearchThreadSafety.h"
#include "SearchPerformance.h"
#include "SearchFeatures.h"
#include "../cache/SearchResultCache.h"
#include "../cache/CacheManager.h"
#include "../cache/PageTextCache.h"
#include <poppler-qt6.h>
#include <QDebug>
#include <QElapsedTimer>

class SearchEngine::Implementation
{
public:
    Implementation(SearchEngine* q)
        : q_ptr(q)
        , document(nullptr)
        , isSearching(false)
        , cacheEnabled(true)
        , incrementalSearchEnabled(true)
        , backgroundProcessingEnabled(true)
        , documentMutex(SearchThreadSafety::MutexHierarchy::DocumentLevel)
        , searchMutex(SearchThreadSafety::MutexHierarchy::SearchLevel)
        , cacheMutex(SearchThreadSafety::MutexHierarchy::CacheLevel)
        , metricsMutex(SearchThreadSafety::MutexHierarchy::MetricsLevel)
    {
        initializeComponents();
        connectSignals();
    }

    ~Implementation()
    {
        cancelCurrentSearch();
    }

    void initializeComponents()
    {
        // Initialize core components
        textExtractor = new TextExtractor(q_ptr);
        searchExecutor = new SearchExecutor(q_ptr);
        backgroundProcessor = new BackgroundProcessor(q_ptr);
        incrementalManager = new IncrementalSearchManager(q_ptr);
        metrics = new SearchMetrics(q_ptr);
        resultCache = new SearchResultCache(q_ptr);
        memoryOptimizer = new MemoryManager(q_ptr);

        // Initialize validator with strict validation for security
        ValidationConfig validationConfig;
        validationConfig.level = Strict;
        validationConfig.enableSanitization = true;
        validationConfig.preventResourceExhaustion = true;
        validator = new SearchValidator(validationConfig);

        // Initialize error recovery system
        errorRecovery = new SearchErrorRecovery(q_ptr);
        setupErrorRecovery();

        // Initialize performance optimizer
        performanceOptimizer = new SearchPerformance(q_ptr);
        setupPerformanceOptimizer();

        // Initialize advanced search features
        advancedFeatures = new SearchFeatures(q_ptr);
        setupAdvancedFeatures();

        // Configure components
        searchExecutor->setTextExtractor(textExtractor);
        backgroundProcessor->setMaxThreadCount(4);
        incrementalManager->setDelay(300);
    }

    void setupErrorRecovery()
    {
        // Configure error recovery for different components
        SearchErrorRecovery::RecoveryConfig searchConfig;
        searchConfig.strategy = SearchErrorRecovery::Fallback;
        searchConfig.maxRetries = 2;
        searchConfig.retryDelayMs = 1000;
        searchConfig.enableFallback = true;
        errorRecovery->setRecoveryConfig(SearchErrorRecovery::SearchError, searchConfig);

        SearchErrorRecovery::RecoveryConfig documentConfig;
        documentConfig.strategy = SearchErrorRecovery::Retry;
        documentConfig.maxRetries = 3;
        documentConfig.retryDelayMs = 2000;
        errorRecovery->setRecoveryConfig(SearchErrorRecovery::DocumentError, documentConfig);

        SearchErrorRecovery::RecoveryConfig cacheConfig;
        cacheConfig.strategy = SearchErrorRecovery::Skip;
        cacheConfig.maxRetries = 1;
        errorRecovery->setRecoveryConfig(SearchErrorRecovery::CacheError, cacheConfig);

        // Register fallback functions
        errorRecovery->registerFallback(SearchErrorRecovery::SearchError, "search",
            [this](const SearchErrorRecovery::ErrorContext& context) -> QVariant {
                // Fallback to simple text search without regex
                SearchOptions simpleOptions;
                simpleOptions.useRegex = false;
                simpleOptions.caseSensitive = false;
                simpleOptions.wholeWords = false;

                QString query = context.metadata.value("query").toString();
                if (!query.isEmpty()) {
                    return QVariant::fromValue(executeSimpleSearch(query, simpleOptions));
                }
                return QVariant();
            });

        errorRecovery->registerFallback(SearchErrorRecovery::DocumentError, "page_access",
            [this](const SearchErrorRecovery::ErrorContext& context) -> QVariant {
                // Skip problematic pages and continue
                int pageNumber = context.metadata.value("pageNumber").toInt();
                qDebug() << "Skipping problematic page:" << pageNumber;
                return QVariant(true);
            });

        // Enable circuit breakers for critical operations
        errorRecovery->enableCircuitBreaker("document_load", 3, 30000);
        errorRecovery->enableCircuitBreaker("text_extraction", 5, 60000);
        errorRecovery->enableCircuitBreaker("search_execution", 10, 120000);
    }

    void setupPerformanceOptimizer()
    {
        // Initialize memory pool for search operations
        performanceOptimizer->initializeMemoryPool(2 * 1024 * 1024); // 2MB pool

        // Enable predictive caching
        performanceOptimizer->enablePredictiveCache(true);

        // Set optimal thread count for parallel operations
        performanceOptimizer->setOptimalThreadCount();

        // Enable work stealing for better load balancing
        performanceOptimizer->enableWorkStealing(true);

        // Set ranking factors for result relevance
        SearchPerformance::RankingFactors factors;
        factors.termFrequency = 1.2;
        factors.documentFrequency = 0.8;
        factors.positionWeight = 1.0;
        factors.contextRelevance = 1.5;
        factors.exactMatchBonus = 2.0;
        factors.proximityBonus = 1.3;
        performanceOptimizer->setRankingFactors(factors);

        // Connect performance signals
        QObject::connect(performanceOptimizer, &SearchPerformance::optimizationCompleted,
            [this](const SearchPerformance::PerformanceMetrics& metrics) {
                qDebug() << "Search optimization completed:"
                         << "Algorithm:" << metrics.algorithmUsed
                         << "Time:" << metrics.searchTime << "ms"
                         << "Results:" << metrics.resultsFound;
            });

        QObject::connect(performanceOptimizer, &SearchPerformance::algorithmSelected,
            [this](const QString& algorithm, const QString& reason) {
                qDebug() << "Selected algorithm:" << algorithm << "Reason:" << reason;
            });
    }

    void setupAdvancedFeatures()
    {
        // Configure highlight colors
        advancedFeatures->setHighlightColors(QColor("#FFFF00"), QColor("#FF6600"));

        // Connect advanced search signals
        QObject::connect(advancedFeatures, &SearchFeatures::fuzzySearchCompleted,
            [this](const QList<SearchFeatures::FuzzyMatch>& matches) {
                qDebug() << "Fuzzy search completed with" << matches.size() << "matches";
            });

        QObject::connect(advancedFeatures, &SearchFeatures::highlightsGenerated,
            [this](const QList<SearchFeatures::HighlightInfo>& highlights) {
                qDebug() << "Generated" << highlights.size() << "highlights";
            });

        QObject::connect(advancedFeatures, &SearchFeatures::historyUpdated,
            [this]() {
                qDebug() << "Search history updated";
            });

        QObject::connect(advancedFeatures, &SearchFeatures::suggestionsReady,
            [this](const QStringList& suggestions) {
                qDebug() << "Search suggestions ready:" << suggestions.size() << "items";
            });
    }

    void connectSignals()
    {
        // Connect incremental search manager
        QObject::connect(incrementalManager, &IncrementalSearchManager::searchTriggered,
            [this](const QString& query, const SearchOptions& options) {
                performSearch(query, options);
            });

        // Connect search executor progress
        QObject::connect(searchExecutor, &SearchExecutor::searchProgress,
            q_ptr, &SearchEngine::searchProgress);

        // Connect background processor
        QObject::connect(backgroundProcessor, &BackgroundProcessor::taskFinished,
            [this]() {
                if (!isSearching.isSet() && backgroundProcessor->isIdle()) {
                    isSearching.clear();
                }
            });

        // Connect metrics warnings
        QObject::connect(metrics, &SearchMetrics::performanceWarning,
            [this](const QString& warning) {
                qWarning() << "Performance:" << warning;
            });
    }

    void performSearch(const QString& query, const SearchOptions& options)
    {
        if (!document) {
            emit q_ptr->searchError("No document loaded");
            return;
        }

        QElapsedTimer timer;
        timer.start();

        // Check cache first
        if (cacheEnabled.isSet()) {
            SearchResultCache::CacheKey key = createCacheKey(query, options);

            if (resultCache->hasResults(key)) {
                QList<SearchResult> cachedResults = resultCache->getResults(key);
                currentResults.set(cachedResults);
                currentQuery.set(query);
                currentOptions.set(options);
                
                metrics->recordCacheHit(query);
                
                // Record metrics
                SearchMetrics::Metric metric;
                metric.query = query;
                metric.duration = timer.elapsed();
                metric.resultCount = cachedResults.size();
                metric.cacheHit = true;
                metric.incremental = false;
                metric.timestamp = QDateTime::currentDateTime();
                metrics->recordSearch(metric);
                
                emit q_ptr->searchFinished(cachedResults);
                return;
            }
            
            metrics->recordCacheMiss(query);
        }

        // Check for incremental search opportunity
        if (incrementalSearchEnabled.isSet() &&
            incrementalManager->canRefineSearch(query, currentQuery.copy())) {

            QList<SearchResult> refinedResults =
                incrementalManager->refineResults(currentResults.copy(), query, currentQuery.copy());
            
            if (!refinedResults.isEmpty()) {
                currentResults.set(refinedResults);
                currentQuery.set(query);
                
                // Record metrics
                SearchMetrics::Metric metric;
                metric.query = query;
                metric.duration = timer.elapsed();
                metric.resultCount = refinedResults.size();
                metric.cacheHit = false;
                metric.incremental = true;
                metric.timestamp = QDateTime::currentDateTime();
                metrics->recordSearch(metric);
                
                emit q_ptr->searchFinished(refinedResults);
                return;
            }
        }

        // Perform full search
        isSearching.set();
        currentQuery.set(query);
        currentOptions.set(options);
        emit q_ptr->searchStarted();

        if (backgroundProcessingEnabled.isSet()) {
            // Asynchronous search
            backgroundProcessor->executeAsync([this, query, options, timer]() {
                QList<SearchResult> results = executeFullSearch(query, options);
                
                // Record metrics
                SearchMetrics::Metric metric;
                metric.query = query;
                metric.duration = timer.elapsed();
                metric.resultCount = results.size();
                metric.pagesSearched = document->numPages();
                metric.cacheHit = false;
                metric.incremental = false;
                metric.timestamp = QDateTime::currentDateTime();
                metrics->recordSearch(metric);
                
                // Update state on main thread
                QMetaObject::invokeMethod(q_ptr, [this, results]() {
                    currentResults.set(results);
                    isSearching.clear();
                    emit q_ptr->searchFinished(results);
                }, Qt::QueuedConnection);
            });
        } else {
            // Synchronous search
            QList<SearchResult> results = executeFullSearch(query, options);
            
            // Record metrics
            SearchMetrics::Metric metric;
            metric.query = query;
            metric.duration = timer.elapsed();
            metric.resultCount = results.size();
            metric.pagesSearched = document->numPages();
            metric.cacheHit = false;
            metric.incremental = false;
            metric.timestamp = QDateTime::currentDateTime();
            metrics->recordSearch(metric);
            
            currentResults.set(results);
            isSearching.clear();
            emit q_ptr->searchFinished(results);
        }
    }

    QList<SearchResult> executeFullSearch(const QString& query, const SearchOptions& options)
    {
        if (!document) {
            return QList<SearchResult>();
        }

        searchExecutor->setOptions(options);

        // Create page list
        QList<int> pages;
        for (int i = 0; i < document->numPages(); ++i) {
            pages.append(i);
        }

        // Perform search
        QList<SearchResult> results = searchExecutor->searchInPages(pages, query);

        // Cache results
        if (cacheEnabled.isSet()) {
            SearchResultCache::CacheKey key = createCacheKey(query, options);
            resultCache->storeResults(key, results);
        }

        return results;
    }

    QList<SearchResult> executeSimpleSearch(const QString& query, const SearchOptions& options)
    {
        if (!document || query.isEmpty()) {
            return QList<SearchResult>();
        }

        QList<SearchResult> results;

        // Simple text search without regex - more reliable fallback
        for (int i = 0; i < document->numPages() && results.size() < options.maxResults; ++i) {
            try {
                std::unique_ptr<Poppler::Page> page(document->page(i));
                if (!page) {
                    continue; // Skip invalid pages
                }

                QString pageText = page->text(QRectF());
                if (pageText.isEmpty()) {
                    continue;
                }

                // Simple case-insensitive text search
                QString searchText = options.caseSensitive ? pageText : pageText.toLower();
                QString searchQuery = options.caseSensitive ? query : query.toLower();

                int pos = 0;
                while ((pos = searchText.indexOf(searchQuery, pos)) != -1 && results.size() < options.maxResults) {
                    // Extract context around the match
                    int contextStart = qMax(0, pos - options.contextLength);
                    int contextEnd = qMin(pageText.length(), pos + searchQuery.length() + options.contextLength);
                    QString context = pageText.mid(contextStart, contextEnd - contextStart);

                    // Create search result
                    SearchResult result(i, searchQuery, context, QRectF(), pos, searchQuery.length());
                    results.append(result);

                    pos += searchQuery.length();
                }

            } catch (const std::exception& e) {
                // Skip problematic pages in simple search
                qDebug() << "Skipping page" << i << "in simple search due to error:" << e.what();
                continue;
            }
        }

        return results;
    }

    void cancelCurrentSearch()
    {
        if (isSearching.isSet()) {
            backgroundProcessor->cancelAll();
            incrementalManager->cancelScheduledSearch();
            isSearching.clear();
            emit q_ptr->searchCancelled();
        }
    }

    SearchResultCache::CacheKey createCacheKey(const QString& query, const SearchOptions& options)
    {
        SearchResultCache::CacheKey key;
        key.query = query;
        key.options = options;
        key.documentId = documentId;
        return key;
    }

    void updateDocumentId()
    {
        if (document) {
            // Generate unique ID based on document pointer and timestamp
            documentId = QString("doc_%1_%2")
                .arg(reinterpret_cast<quintptr>(document))
                .arg(QDateTime::currentMSecsSinceEpoch());
        } else {
            documentId.clear();
        }
    }

    // Parent pointer
    SearchEngine* q_ptr;

    // Core components
    TextExtractor* textExtractor;
    SearchExecutor* searchExecutor;
    BackgroundProcessor* backgroundProcessor;
    IncrementalSearchManager* incrementalManager;
    SearchMetrics* metrics;
    SearchResultCache* resultCache;
    MemoryManager* memoryOptimizer;
    SearchValidator* validator;
    SearchErrorRecovery* errorRecovery;
    SearchPerformance* performanceOptimizer;
    SearchFeatures* advancedFeatures;

    // Document state
    Poppler::Document* document;
    QString documentId;

    // Search state (thread-safe)
    SearchThreadSafety::SharedData<QList<SearchResult>> currentResults;
    SearchThreadSafety::SharedData<QString> currentQuery;
    SearchThreadSafety::SharedData<SearchOptions> currentOptions;
    SearchThreadSafety::AtomicFlag isSearching;
    SearchThreadSafety::AtomicCounter searchId;

    // Configuration (thread-safe)
    SearchThreadSafety::AtomicFlag cacheEnabled;
    SearchThreadSafety::AtomicFlag incrementalSearchEnabled;
    SearchThreadSafety::AtomicFlag backgroundProcessingEnabled;

    // Thread safety mutexes (hierarchical)
    SearchThreadSafety::MutexHierarchy::HierarchicalMutex documentMutex;
    SearchThreadSafety::MutexHierarchy::HierarchicalMutex searchMutex;
    SearchThreadSafety::MutexHierarchy::HierarchicalMutex cacheMutex;
    SearchThreadSafety::MutexHierarchy::HierarchicalMutex metricsMutex;
};

// Public interface implementation

SearchEngine::SearchEngine(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Implementation>(this))
{
    // Register caches with the unified cache manager
    CacheManager& cacheManager = CacheManager::instance();

    // Register SearchResultCache
    cacheManager.registerCache(CacheManager::SearchResultCache, d->resultCache);

    // Register TextExtractor cache through adapter
    TextExtractorCacheAdapter* textCacheAdapter = new TextExtractorCacheAdapter(d->textExtractor, this);
    cacheManager.registerCache(CacheManager::PageTextCache, textCacheAdapter);

    // Register with memory optimizer
    d->memoryOptimizer->registerSearchEngine(this);
    d->memoryOptimizer->registerTextExtractor(d->textExtractor);
}

SearchEngine::~SearchEngine() = default;

void SearchEngine::setDocument(Poppler::Document* document)
{
    if (d->document != document) {
        d->cancelCurrentSearch();
        d->document = document;
        d->updateDocumentId();
        d->textExtractor->setDocument(document);
        d->currentResults.set(QList<SearchResult>());
        d->currentQuery.set(QString());
        
        // Clear cache for old document
        if (document) {
            d->resultCache->invalidateDocument(d->documentId);
        }
    }
}

Poppler::Document* SearchEngine::document() const
{
    return d->document;
}

void SearchEngine::search(const QString& query, const SearchOptions& options)
{
    SEARCH_ERROR_SCOPE(d->errorRecovery, SearchErrorRecovery::SearchError, "search", "SearchEngine");

    try {
        // Comprehensive input validation
        auto validationResult = d->validator->validateSearchRequest(query, options, d->document);
        if (!validationResult.isValid) {
            QString errorMsg = QString("Search validation failed: %1").arg(validationResult.errorMessages.join("; "));
            emit searchError(errorMsg);
            return;
        }

        // Use sanitized query if available
        QString sanitizedQuery = validationResult.sanitizedInput.isEmpty() ? query : validationResult.sanitizedInput;

        if (sanitizedQuery.isEmpty()) {
            clearResults();
            return;
        }

        // Execute search with error recovery
        auto searchOperation = [this, sanitizedQuery, options]() -> bool {
            d->performSearch(sanitizedQuery, options);
            return true;
        };

        SearchErrorRecovery::ErrorContext context(SearchErrorRecovery::SearchError, "search", "SearchEngine");
        context.metadata["query"] = sanitizedQuery;
        context.metadata["options"] = QVariant::fromValue(options);

        d->errorRecovery->executeWithRecovery<bool>(searchOperation, context);

    } catch (const SearchException& e) {
        SearchErrorRecovery::ErrorContext context(e.type(), "search", "SearchEngine", e.what());
        context.metadata["query"] = query;
        d->errorRecovery->handleError(e, context);
        emit searchError(QString("Search failed: %1").arg(e.what()));

    } catch (const std::exception& e) {
        SearchErrorRecovery::ErrorContext context(SearchErrorRecovery::UnknownError, "search", "SearchEngine", e.what());
        context.metadata["query"] = query;
        d->errorRecovery->handleError(e, context);
        emit searchError(QString("Unexpected search error: %1").arg(e.what()));
    }
}

void SearchEngine::searchIncremental(const QString& query, const SearchOptions& options)
{
    if (query.isEmpty()) {
        clearResults();
        return;
    }

    if (d->incrementalSearchEnabled.isSet()) {
        d->incrementalManager->scheduleSearch(query, options);
    } else {
        search(query, options);
    }
}

void SearchEngine::cancelSearch()
{
    d->cancelCurrentSearch();
}

void SearchEngine::clearResults()
{
    d->currentResults.set(QList<SearchResult>());
    d->currentQuery.set(QString());
    emit resultsUpdated(QList<SearchResult>());
}

void SearchEngine::setCacheEnabled(bool enabled)
{
    if (enabled) {
        d->cacheEnabled.set();
    } else {
        d->cacheEnabled.clear();
    }
    d->textExtractor->setCacheEnabled(enabled);
    if (!enabled) {
        d->resultCache->clear();
    }
}

bool SearchEngine::isCacheEnabled() const
{
    return d->cacheEnabled.isSet();
}

void SearchEngine::setIncrementalSearchEnabled(bool enabled)
{
    if (enabled) {
        d->incrementalSearchEnabled.set();
    } else {
        d->incrementalSearchEnabled.clear();
    }
    d->incrementalManager->setEnabled(enabled);
}

bool SearchEngine::isIncrementalSearchEnabled() const
{
    return d->incrementalSearchEnabled.isSet();
}

void SearchEngine::setBackgroundProcessingEnabled(bool enabled)
{
    if (enabled) {
        d->backgroundProcessingEnabled.set();
    } else {
        d->backgroundProcessingEnabled.clear();
    }
}

bool SearchEngine::isBackgroundProcessingEnabled() const
{
    return d->backgroundProcessingEnabled.isSet();
}

QList<SearchResult> SearchEngine::results() const
{
    return d->currentResults.copy();
}

int SearchEngine::resultCount() const
{
    return d->currentResults.copy().size();
}

bool SearchEngine::isSearching() const
{
    return d->isSearching.isSet();
}

QString SearchEngine::currentQuery() const
{
    return d->currentQuery.copy();
}

double SearchEngine::cacheHitRatio() const
{
    return d->metrics->cacheHitRatio();
}

qint64 SearchEngine::cacheMemoryUsage() const
{
    return d->resultCache->getMemoryUsage() + d->textExtractor->cacheMemoryUsage();
}

void SearchEngine::resetStatistics()
{
    d->metrics->clearHistory();
    d->resultCache->resetStatistics();
}

// Advanced search operations implementation
void SearchEngine::fuzzySearch(const QString& query, int maxDistance, const SearchOptions& options)
{
    if (!d->document || query.isEmpty()) {
        emit searchError("Invalid document or empty query for fuzzy search");
        return;
    }

    try {
        QElapsedTimer timer;
        timer.start();

        emit searchStarted();
        d->isSearching.set();

        QList<SearchResult> allResults;

        // Perform fuzzy search on all pages
        for (int pageNum = 0; pageNum < d->document->numPages(); ++pageNum) {
            std::unique_ptr<Poppler::Page> page(d->document->page(pageNum));
            if (!page) continue;

            QString pageText = page->text(QRectF());
            auto fuzzyMatches = d->advancedFeatures->fuzzySearch(pageText, query, maxDistance);

            // Convert fuzzy matches to SearchResult
            for (const auto& match : fuzzyMatches) {
                SearchResult result;
                result.pageNumber = pageNum;
                result.matchedText = match.text;
                result.contextText = match.context;
                result.textPosition = match.position;
                result.textLength = match.length;
                // Note: boundingRect would need coordinate mapping from text position

                allResults.append(result);
            }
        }

        // Add to search history
        qint64 searchTime = timer.elapsed();
        d->advancedFeatures->addToHistory(query, options, allResults.size(), searchTime, true);

        d->currentResults.set(allResults);
        d->isSearching.clear();

        emit searchFinished(allResults);

    } catch (const std::exception& e) {
        d->isSearching.clear();
        emit searchError(QString("Fuzzy search error: %1").arg(e.what()));
    }
}

void SearchEngine::wildcardSearch(const QString& pattern, const SearchOptions& options)
{
    if (!d->document || pattern.isEmpty()) {
        emit searchError("Invalid document or empty pattern for wildcard search");
        return;
    }

    try {
        QElapsedTimer timer;
        timer.start();

        emit searchStarted();
        d->isSearching.set();

        QList<SearchResult> allResults;

        // Perform wildcard search on all pages
        for (int pageNum = 0; pageNum < d->document->numPages(); ++pageNum) {
            std::unique_ptr<Poppler::Page> page(d->document->page(pageNum));
            if (!page) continue;

            QString pageText = page->text(QRectF());
            auto wildcardResults = d->advancedFeatures->wildcardSearch(pageText, pattern, pageNum);
            allResults.append(wildcardResults);
        }

        // Add to search history
        qint64 searchTime = timer.elapsed();
        d->advancedFeatures->addToHistory(pattern, options, allResults.size(), searchTime, true);

        d->currentResults.set(allResults);
        d->isSearching.clear();

        emit searchFinished(allResults);

    } catch (const std::exception& e) {
        d->isSearching.clear();
        emit searchError(QString("Wildcard search error: %1").arg(e.what()));
    }
}

void SearchEngine::phraseSearch(const QString& phrase, int proximity, const SearchOptions& options)
{
    if (!d->document || phrase.isEmpty()) {
        emit searchError("Invalid document or empty phrase for phrase search");
        return;
    }

    try {
        QElapsedTimer timer;
        timer.start();

        emit searchStarted();
        d->isSearching.set();

        QList<SearchResult> allResults;

        // Perform phrase search on all pages
        for (int pageNum = 0; pageNum < d->document->numPages(); ++pageNum) {
            std::unique_ptr<Poppler::Page> page(d->document->page(pageNum));
            if (!page) continue;

            QString pageText = page->text(QRectF());
            auto phraseResults = d->advancedFeatures->phraseSearch(pageText, phrase, pageNum, proximity);
            allResults.append(phraseResults);
        }

        // Add to search history
        qint64 searchTime = timer.elapsed();
        d->advancedFeatures->addToHistory(phrase, options, allResults.size(), searchTime, true);

        d->currentResults.set(allResults);
        d->isSearching.clear();

        emit searchFinished(allResults);

    } catch (const std::exception& e) {
        d->isSearching.clear();
        emit searchError(QString("Phrase search error: %1").arg(e.what()));
    }
}

void SearchEngine::booleanSearch(const QString& query, const SearchOptions& options)
{
    if (!d->document || query.isEmpty()) {
        emit searchError("Invalid document or empty query for boolean search");
        return;
    }

    try {
        QElapsedTimer timer;
        timer.start();

        emit searchStarted();
        d->isSearching.set();

        QList<SearchResult> allResults;

        // Perform boolean search on all pages
        for (int pageNum = 0; pageNum < d->document->numPages(); ++pageNum) {
            std::unique_ptr<Poppler::Page> page(d->document->page(pageNum));
            if (!page) continue;

            QString pageText = page->text(QRectF());
            auto booleanResults = d->advancedFeatures->booleanSearch(pageText, query, pageNum);
            allResults.append(booleanResults);
        }

        // Add to search history
        qint64 searchTime = timer.elapsed();
        d->advancedFeatures->addToHistory(query, options, allResults.size(), searchTime, true);

        d->currentResults.set(allResults);
        d->isSearching.clear();

        emit searchFinished(allResults);

    } catch (const std::exception& e) {
        d->isSearching.clear();
        emit searchError(QString("Boolean search error: %1").arg(e.what()));
    }
}

void SearchEngine::proximitySearch(const QStringList& terms, int maxDistance, bool ordered, const SearchOptions& options)
{
    if (!d->document || terms.isEmpty()) {
        emit searchError("Invalid document or empty terms for proximity search");
        return;
    }

    try {
        QElapsedTimer timer;
        timer.start();

        emit searchStarted();
        d->isSearching.set();

        QList<SearchResult> allResults;

        // Configure proximity search options
        SearchFeatures::ProximitySearchOptions proxOptions;
        proxOptions.maxDistance = maxDistance;
        proxOptions.ordered = ordered;
        proxOptions.caseSensitive = options.caseSensitive;
        proxOptions.wholeWords = options.wholeWords;

        // Perform proximity search on all pages
        for (int pageNum = 0; pageNum < d->document->numPages(); ++pageNum) {
            std::unique_ptr<Poppler::Page> page(d->document->page(pageNum));
            if (!page) continue;

            QString pageText = page->text(QRectF());
            auto proximityResults = d->advancedFeatures->proximitySearch(pageText, terms, proxOptions, pageNum);
            allResults.append(proximityResults);
        }

        // Add to search history
        QString queryString = terms.join(" NEAR ");
        qint64 searchTime = timer.elapsed();
        d->advancedFeatures->addToHistory(queryString, options, allResults.size(), searchTime, true);

        d->currentResults.set(allResults);
        d->isSearching.clear();

        emit searchFinished(allResults);

    } catch (const std::exception& e) {
        d->isSearching.clear();
        emit searchError(QString("Proximity search error: %1").arg(e.what()));
    }
}

// Advanced features access methods
SearchFeatures* SearchEngine::advancedFeatures() const
{
    return d->advancedFeatures;
}

void SearchEngine::setHighlightColors(const QColor& normalColor, const QColor& currentColor)
{
    d->advancedFeatures->setHighlightColors(normalColor, currentColor);
}

QStringList SearchEngine::getSearchSuggestions(const QString& partialQuery, int maxSuggestions)
{
    return d->advancedFeatures->generateSuggestions(partialQuery, maxSuggestions);
}

QStringList SearchEngine::getSearchHistory(int maxEntries)
{
    auto historyEntries = d->advancedFeatures->getSearchHistory(maxEntries);
    QStringList queries;

    for (const auto& entry : historyEntries) {
        queries.append(entry.query);
    }

    return queries;
}

void SearchEngine::clearSearchHistory()
{
    d->advancedFeatures->clearHistory();
}

// Synchronous search operations for testing compatibility
void SearchEngine::startSearch(Poppler::Document* document, const QString& query, const SearchOptions& options)
{
    // Handle null document or empty query immediately
    if (!document || query.isEmpty()) {
        // Don't wait for signals, just return empty results
        return;
    }

    setDocument(document);

    // Check if document has pages
    if (document->numPages() == 0) {
        // Don't wait for signals, just return empty results
        return;
    }

    search(query, options);

    // Wait for search to complete (for testing purposes)
    // In a real application, you would use signals/slots
    QEventLoop loop;
    connect(this, &SearchEngine::searchFinished, &loop, &QEventLoop::quit);
    connect(this, &SearchEngine::searchError, &loop, &QEventLoop::quit);

    // Set a shorter timeout to prevent infinite waiting
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);
    loop.exec();
}

QList<SearchResult> SearchEngine::getResults() const
{
    return results();
}
