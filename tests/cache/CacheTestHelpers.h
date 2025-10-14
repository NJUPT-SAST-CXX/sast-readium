#pragma once

#include <poppler-qt6.h>
#include <QFile>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPixmap>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTextStream>
#include "../../app/cache/CacheManager.h"
#include "../../app/cache/PDFCacheManager.h"
#include "../../app/cache/PageTextCache.h"
#include "../../app/cache/SearchResultCache.h"
#include "../../app/search/SearchConfiguration.h"
#include "../TestUtilities.h"

/**
 * @brief Helper utilities for cache testing
 *
 * Provides common functionality for creating test data, PDFs,
 * and validating cache behavior across all cache tests.
 */
class CacheTestHelpers {
public:
    /**
     * @brief Creates a minimal test PDF file
     * @param content Text content to include in the PDF
     * @param pageCount Number of pages to create
     * @return Path to the created PDF file
     */
    static QString createTestPdf(const QString& content = "Test PDF Content",
                                 int pageCount = 1) {
        QString fileName =
            QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
            QString("/cache_test_%1.pdf")
                .arg(QRandomGenerator::global()->generate());

        QPdfWriter writer(fileName);
        writer.setPageSize(QPageSize::A4);

        QPainter painter(&writer);
        if (!painter.isActive()) {
            return QString();
        }

        for (int page = 0; page < pageCount; ++page) {
            if (page > 0) {
                writer.newPage();
            }
            painter.drawText(100, 100,
                             QString("Page %1: %2").arg(page + 1).arg(content));
        }

        painter.end();
        return fileName;
    }

    /**
     * @brief Creates a test PDF with specific text on each page
     * @param pageTexts List of text content for each page
     * @return Path to the created PDF file
     */
    static QString createMultiPageTestPdf(const QStringList& pageTexts) {
        QString fileName =
            QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
            QString("/cache_test_multi_%1.pdf")
                .arg(QRandomGenerator::global()->generate());

        QPdfWriter writer(fileName);
        writer.setPageSize(QPageSize::A4);

        QPainter painter(&writer);
        if (!painter.isActive()) {
            return QString();
        }

        for (int i = 0; i < pageTexts.size(); ++i) {
            if (i > 0) {
                writer.newPage();
            }
            painter.drawText(100, 100, pageTexts[i]);
        }

        painter.end();
        return fileName;
    }

    /**
     * @brief Loads a PDF document from file
     * @param filePath Path to the PDF file
     * @return Pointer to loaded document (caller owns)
     */
    static Poppler::Document* loadTestDocument(const QString& filePath) {
        auto doc = Poppler::Document::load(filePath);
        return doc.release();
    }

    /**
     * @brief Creates a test pixmap
     * @param width Width of the pixmap
     * @param height Height of the pixmap
     * @param color Fill color
     * @return Created pixmap
     */
    static QPixmap createTestPixmap(int width = 100, int height = 100,
                                    const QColor& color = Qt::blue) {
        QPixmap pixmap(width, height);
        pixmap.fill(color);
        return pixmap;
    }

    /**
     * @brief Creates a test search result
     * @param pageNumber Page number
     * @param matchedText Matched text
     * @param contextText Context text
     * @return Created search result
     */
    static SearchResult createTestSearchResult(
        int pageNumber = 0, const QString& matchedText = "test",
        const QString& contextText = "This is a test document") {
        return SearchResult(pageNumber, matchedText, contextText,
                            QRectF(10, 10, 50, 20), 10, matchedText.length());
    }

    /**
     * @brief Creates a list of test search results
     * @param count Number of results to create
     * @return List of search results
     */
    static QList<SearchResult> createTestSearchResults(int count) {
        QList<SearchResult> results;
        for (int i = 0; i < count; ++i) {
            results.append(
                createTestSearchResult(i / 3, QString("test%1").arg(i),
                                       QString("Context for test%1").arg(i)));
        }
        return results;
    }

    /**
     * @brief Creates test search options
     * @param caseSensitive Case sensitivity flag
     * @param wholeWords Whole words flag
     * @param useRegex Regex flag
     * @return Created search options
     */
    static SearchOptions createTestSearchOptions(bool caseSensitive = false,
                                                 bool wholeWords = false,
                                                 bool useRegex = false) {
        SearchOptions options;
        options.caseSensitive = caseSensitive;
        options.wholeWords = wholeWords;
        options.useRegex = useRegex;
        options.maxResults = 100;
        options.contextLength = 50;
        return options;
    }

    /**
     * @brief Creates a test cache key for SearchResultCache
     * @param query Search query
     * @param documentId Document identifier
     * @return Created cache key
     */
    static SearchResultCache::CacheKey createTestCacheKey(
        const QString& query = "test", const QString& documentId = "test_doc") {
        SearchResultCache::CacheKey key;
        key.query = query;
        key.documentId = documentId;
        key.documentModified = QDateTime::currentMSecsSinceEpoch();
        key.options = createTestSearchOptions();
        return key;
    }

    /**
     * @brief Cleans up test files
     * @param files List of file paths to remove
     */
    static void cleanupTestFiles(const QStringList& files) {
        for (const QString& file : files) {
            QFile::remove(file);
        }
    }

    /**
     * @brief Generates random text content
     * @param wordCount Number of words to generate
     * @return Generated text
     */
    static QString generateRandomText(int wordCount = 100) {
        QStringList words = {"test",   "document", "content", "search",
                             "result", "cache",    "page",    "text",
                             "data",   "sample",   "example", "word"};
        QString result;
        for (int i = 0; i < wordCount; ++i) {
            result += words[QRandomGenerator::global()->bounded(words.size())];
            result += " ";
        }
        return result.trimmed();
    }

    /**
     * @brief Calculates approximate memory size of a pixmap
     * @param pixmap Pixmap to measure
     * @return Approximate memory size in bytes
     */
    static qint64 calculatePixmapSize(const QPixmap& pixmap) {
        return pixmap.width() * pixmap.height() * 4;  // 32-bit ARGB
    }

    /**
     * @brief Calculates approximate memory size of text
     * @param text Text to measure
     * @return Approximate memory size in bytes
     */
    static qint64 calculateTextSize(const QString& text) {
        return text.size() * sizeof(QChar);
    }
};

/**
 * @brief Mock ICacheComponent for testing CacheManager
 *
 * Note: This class is outside the namespace because Qt's moc
 * doesn't support Q_OBJECT in nested classes.
 */
class MockCacheComponent : public QObject, public ICacheComponent {
    Q_OBJECT

public:
    explicit MockCacheComponent(QObject* parent = nullptr);
    ~MockCacheComponent() override;

    qint64 getMemoryUsage() const override;
    qint64 getMaxMemoryLimit() const override;
    void setMaxMemoryLimit(qint64 limit) override;
    void clear() override;
    int getEntryCount() const override;
    void evictLRU(qint64 bytesToFree) override;
    qint64 getHitCount() const override;
    qint64 getMissCount() const override;
    void resetStatistics() override;
    void setEnabled(bool enabled) override;
    bool isEnabled() const override;

    // Test helpers
    void setMemoryUsage(qint64 usage);
    void setEntryCount(int count);
    void incrementHits();
    void incrementMisses();

private:
    qint64 m_memoryUsage;
    qint64 m_maxMemoryLimit;
    int m_entryCount;
    qint64 m_hitCount;
    qint64 m_missCount;
    bool m_enabled;
};
