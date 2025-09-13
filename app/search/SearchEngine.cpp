#include "SearchEngine.h"
#include "SearchConfiguration.h"
#include "TextExtractor.h"
#include "SearchExecutor.h"
#include "BackgroundProcessor.h"
#include "IncrementalSearchManager.h"
#include "SearchMetrics.h"
#include "../cache/SearchResultCache.h"
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

        // Configure components
        searchExecutor->setTextExtractor(textExtractor);
        backgroundProcessor->setMaxThreadCount(4);
        incrementalManager->setDelay(300);
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
                if (!isSearching && backgroundProcessor->isIdle()) {
                    isSearching = false;
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
        if (cacheEnabled) {
            SearchResultCache::CacheKey key = createCacheKey(query, options);
            
            if (resultCache->hasResults(key)) {
                QList<SearchResult> cachedResults = resultCache->getResults(key);
                currentResults = cachedResults;
                currentQuery = query;
                currentOptions = options;
                
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
        if (incrementalSearchEnabled && 
            incrementalManager->canRefineSearch(query, currentQuery)) {
            
            QList<SearchResult> refinedResults = 
                incrementalManager->refineResults(currentResults, query, currentQuery);
            
            if (!refinedResults.isEmpty()) {
                currentResults = refinedResults;
                currentQuery = query;
                
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
        isSearching = true;
        currentQuery = query;
        currentOptions = options;
        emit q_ptr->searchStarted();

        if (backgroundProcessingEnabled) {
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
                    currentResults = results;
                    isSearching = false;
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
            
            currentResults = results;
            isSearching = false;
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
        if (cacheEnabled) {
            SearchResultCache::CacheKey key = createCacheKey(query, options);
            resultCache->storeResults(key, results);
        }

        return results;
    }

    void cancelCurrentSearch()
    {
        if (isSearching) {
            backgroundProcessor->cancelAll();
            incrementalManager->cancelScheduledSearch();
            isSearching = false;
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

    // Document state
    Poppler::Document* document;
    QString documentId;

    // Search state
    QList<SearchResult> currentResults;
    QString currentQuery;
    SearchOptions currentOptions;
    bool isSearching;

    // Configuration
    bool cacheEnabled;
    bool incrementalSearchEnabled;
    bool backgroundProcessingEnabled;
};

// Public interface implementation

SearchEngine::SearchEngine(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Implementation>(this))
{
}

SearchEngine::~SearchEngine() = default;

void SearchEngine::setDocument(Poppler::Document* document)
{
    if (d->document != document) {
        d->cancelCurrentSearch();
        d->document = document;
        d->updateDocumentId();
        d->textExtractor->setDocument(document);
        d->currentResults.clear();
        d->currentQuery.clear();
        
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
    if (query.isEmpty()) {
        clearResults();
        return;
    }

    d->performSearch(query, options);
}

void SearchEngine::searchIncremental(const QString& query, const SearchOptions& options)
{
    if (query.isEmpty()) {
        clearResults();
        return;
    }

    if (d->incrementalSearchEnabled) {
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
    d->currentResults.clear();
    d->currentQuery.clear();
    emit resultsUpdated(QList<SearchResult>());
}

void SearchEngine::setCacheEnabled(bool enabled)
{
    d->cacheEnabled = enabled;
    d->textExtractor->setCacheEnabled(enabled);
    if (!enabled) {
        d->resultCache->clear();
    }
}

bool SearchEngine::isCacheEnabled() const
{
    return d->cacheEnabled;
}

void SearchEngine::setIncrementalSearchEnabled(bool enabled)
{
    d->incrementalSearchEnabled = enabled;
    d->incrementalManager->setEnabled(enabled);
}

bool SearchEngine::isIncrementalSearchEnabled() const
{
    return d->incrementalSearchEnabled;
}

void SearchEngine::setBackgroundProcessingEnabled(bool enabled)
{
    d->backgroundProcessingEnabled = enabled;
}

bool SearchEngine::isBackgroundProcessingEnabled() const
{
    return d->backgroundProcessingEnabled;
}

QList<SearchResult> SearchEngine::results() const
{
    return d->currentResults;
}

int SearchEngine::resultCount() const
{
    return d->currentResults.size();
}

bool SearchEngine::isSearching() const
{
    return d->isSearching;
}

QString SearchEngine::currentQuery() const
{
    return d->currentQuery;
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
