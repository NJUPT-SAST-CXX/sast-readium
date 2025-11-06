#pragma once

#include <QMetaType>
#include <QRectF>
#include <QString>

/**
 * Comprehensive search configuration and options
 * Unified structure combining all search features
 */
struct SearchOptions {
    // Basic search options
    bool caseSensitive = false;
    bool wholeWords = false;
    bool useRegex = false;
    bool searchBackward = false;
    int maxResults = 1000;
    int contextLength = 50;
    QString highlightColor = "#FFFF00";

    // Advanced search features
    bool fuzzySearch = false;
    int fuzzyThreshold = 2;  // Maximum edit distance for fuzzy search
    int startPage = -1;      // -1 means search all pages
    int endPage = -1;        // -1 means search all pages
    bool searchInSelection = false;
    QRectF selectionRect;  // For search within selection

    // Performance options
    bool useIndexedSearch = true;
    bool enableSearchCache = true;
    bool enableIncrementalSearch = true;
    int searchTimeout = 30000;  // 30 seconds timeout

    SearchOptions() = default;

    bool operator==(const SearchOptions& other) const {
        return caseSensitive == other.caseSensitive &&
               wholeWords == other.wholeWords && useRegex == other.useRegex &&
               searchBackward == other.searchBackward &&
               maxResults == other.maxResults &&
               contextLength == other.contextLength &&
               highlightColor == other.highlightColor &&
               fuzzySearch == other.fuzzySearch &&
               fuzzyThreshold == other.fuzzyThreshold &&
               startPage == other.startPage && endPage == other.endPage &&
               searchInSelection == other.searchInSelection &&
               selectionRect == other.selectionRect &&
               useIndexedSearch == other.useIndexedSearch &&
               enableSearchCache == other.enableSearchCache &&
               enableIncrementalSearch == other.enableIncrementalSearch &&
               searchTimeout == other.searchTimeout;
    }
};

Q_DECLARE_METATYPE(SearchOptions)

/**
 * Comprehensive search result with enhanced features
 * Unified structure combining all search result functionality
 *
 * Note: This class uses a single set of member variables with descriptive
 * names. The old "alias" approach (text/matchedText, etc.) was removed because
 * C++ reference members prevent the class from being copyable/movable, which
 * breaks QList usage.
 */
class SearchResult {
public:
    SearchResult() = default;
    SearchResult(int page, const QString& textMatch,
                 const QString& contextMatch, const QRectF& rect, int position,
                 int len)
        : pageNumber(page),
          matchedText(textMatch),
          contextText(contextMatch),
          boundingRect(rect),
          textPosition(position),
          textLength(len),
          isCurrentResult(false) {}

    // Primary properties
    int pageNumber = -1;
    QString matchedText;   // The matched text
    QString contextText;   // Context around the match
    QRectF boundingRect;   // PDF coordinates from Poppler
    int textPosition = 0;  // Position in the page text
    int textLength = 0;    // Length of the matched text

    // Enhanced features
    QRectF widgetRect;  // Transformed widget coordinates for highlighting
    bool isCurrentResult =
        false;  // Whether this is the currently selected result

    // Validation methods
    bool isValid() const { return pageNumber >= 0; }
    bool isValidForHighlight() const {
        return pageNumber >= 0 && !boundingRect.isEmpty();
    }

    // Transform PDF coordinates to widget coordinates
    void transformToWidgetCoordinates(double scaleFactor, int rotation,
                                      const QSizeF& pageSize,
                                      const QSize& widgetSize);
};

Q_DECLARE_METATYPE(SearchResult)
Q_DECLARE_METATYPE(QList<SearchResult>)

/**
 * Search engine configuration
 */
struct SearchEngineConfig {
    // Caching
    bool enableCache = true;
    qint64 maxCacheMemory = 100 * 1024 * 1024;  // 100MB
    int maxCacheEntries = 1000;

    // Incremental search
    bool enableIncrementalSearch = true;
    int incrementalSearchDelay = 300;  // milliseconds

    // Background processing
    bool enableBackgroundProcessing = true;
    int maxBackgroundThreads = 4;
    int textExtractionBatchSize = 10;

    // Performance
    int searchProgressInterval = 10;  // Update progress every N pages
    bool prefetchAdjacentPages = true;
};
