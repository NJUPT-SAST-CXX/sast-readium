#pragma once

#include <QString>
#include <QRectF>

/**
 * Search configuration and options
 */
struct SearchOptions {
    bool caseSensitive = false;
    bool wholeWords = false;
    bool useRegex = false;
    int maxResults = 1000;
    int contextLength = 50;
    
    bool operator==(const SearchOptions& other) const {
        return caseSensitive == other.caseSensitive &&
               wholeWords == other.wholeWords &&
               useRegex == other.useRegex &&
               maxResults == other.maxResults &&
               contextLength == other.contextLength;
    }
};

/**
 * Individual search result
 */
class SearchResult {
public:
    SearchResult() = default;
    SearchResult(int page, const QString& text, const QString& context, 
                 const QRectF& rect, int position, int length)
        : pageNumber(page)
        , matchedText(text)
        , contextText(context)
        , boundingRect(rect)
        , textPosition(position)
        , textLength(length) {}

    int pageNumber = -1;
    QString matchedText;
    QString contextText;
    QRectF boundingRect;
    int textPosition = 0;
    int textLength = 0;
    
    bool isValid() const { return pageNumber >= 0; }
};

/**
 * Search engine configuration
 */
struct SearchEngineConfig {
    // Caching
    bool enableCache = true;
    qint64 maxCacheMemory = 100 * 1024 * 1024; // 100MB
    int maxCacheEntries = 1000;
    
    // Incremental search
    bool enableIncrementalSearch = true;
    int incrementalSearchDelay = 300; // milliseconds
    
    // Background processing
    bool enableBackgroundProcessing = true;
    int maxBackgroundThreads = 4;
    int textExtractionBatchSize = 10;
    
    // Performance
    int searchProgressInterval = 10; // Update progress every N pages
    bool prefetchAdjacentPages = true;
};
