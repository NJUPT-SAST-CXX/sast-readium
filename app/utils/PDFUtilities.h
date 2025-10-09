#pragma once

#include <poppler-qt6.h>
#include <QJsonArray>
#include <QJsonObject>
#include <QPixmap>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QStringList>

// Forward declaration
class PDFAnnotation;

/**
 * Utility class for PDF operations and analysis
 */
class PDFUtilities {
public:
    // Document analysis functions
    static QJsonObject analyzeDocument(Poppler::Document* document);
    static QStringList extractAllText(Poppler::Document* document);
    static QList<QPixmap> extractAllImages(Poppler::Document* document);
    static QJsonArray extractDocumentStructure(Poppler::Document* document);

    // Page analysis functions
    static QJsonObject analyzePage(Poppler::Page* page, int pageNumber);
    static QString extractPageText(Poppler::Page* page);
    static QList<QPixmap> extractPageImages(Poppler::Page* page);
    static QList<QRectF> findTextBounds(Poppler::Page* page,
                                        const QString& searchText);
    static QSizeF getPageSize(Poppler::Page* page);
    static double getPageRotation(Poppler::Page* page);

    // Text analysis functions
    static int countWords(const QString& text);
    static int countSentences(const QString& text);
    static int countParagraphs(const QString& text);
    static QStringList extractKeywords(const QString& text,
                                       int maxKeywords = 10);
    static double calculateReadingTime(const QString& text,
                                       int wordsPerMinute = 200);
    static QString detectLanguage(const QString& text);

    // Image analysis functions
    static QJsonObject analyzeImage(const QPixmap& image);
    static bool isImageDuplicate(const QPixmap& image1, const QPixmap& image2,
                                 double threshold = 0.95);
    static QPixmap resizeImage(const QPixmap& image, const QSize& targetSize,
                               bool maintainAspectRatio = true);
    static QPixmap cropImage(const QPixmap& image, const QRectF& cropRect);
    static double calculateImageSimilarity(const QPixmap& image1,
                                           const QPixmap& image2);

    // Document comparison utilities
    static double calculateDocumentSimilarity(Poppler::Document* doc1,
                                              Poppler::Document* doc2);
    static QJsonObject compareDocumentMetadata(Poppler::Document* doc1,
                                               Poppler::Document* doc2);
    static QStringList findCommonPages(Poppler::Document* doc1,
                                       Poppler::Document* doc2,
                                       double threshold = 0.8);
    static QJsonArray findTextDifferences(const QString& text1,
                                          const QString& text2);

    // Rendering utilities
    static QPixmap renderPageToPixmap(Poppler::Page* page, double dpi = 150.0);
    static QPixmap renderPageRegion(Poppler::Page* page, const QRectF& region,
                                    double dpi = 150.0);
    static QList<QPixmap> renderDocumentThumbnails(Poppler::Document* document,
                                                   const QSize& thumbnailSize);
    static QPixmap createPagePreview(Poppler::Page* page,
                                     const QSize& previewSize);

    // Annotation utilities
    static QJsonArray extractAnnotations(Poppler::Page* page);
    static QJsonObject analyzeAnnotation(Poppler::Annotation* annotation);
    static int countAnnotations(Poppler::Document* document);
    static QStringList getAnnotationTypes(Poppler::Document* document);

    // Security and properties
    static QJsonObject getDocumentSecurity(Poppler::Document* document);
    static QJsonObject getDocumentProperties(Poppler::Document* document);
    static bool isDocumentEncrypted(Poppler::Document* document);
    static bool canExtractText(Poppler::Document* document);
    static bool canPrint(Poppler::Document* document);
    static bool canModify(Poppler::Document* document);

    // Export utilities
    static bool exportPageAsImage(Poppler::Page* page, const QString& filePath,
                                  const QString& format = "PNG");
    static bool exportDocumentAsImages(Poppler::Document* document,
                                       const QString& outputDir,
                                       const QString& format = "PNG");
    static bool exportTextToFile(const QString& text, const QString& filePath);
    static bool exportAnalysisToJson(const QJsonObject& analysis,
                                     const QString& filePath);

    // PDF save utilities
    static bool savePDFWithAnnotations(Poppler::Document* document,
                                       const QString& filePath);
    static bool savePDFWithAnnotations(Poppler::Document* document,
                                       const QString& filePath,
                                       const QList<PDFAnnotation>& annotations);

    // Search utilities
    static QList<QRectF> searchText(Poppler::Page* page,
                                    const QString& searchText,
                                    bool caseSensitive = false);
    static QJsonArray searchTextInDocument(Poppler::Document* document,
                                           const QString& searchText,
                                           bool caseSensitive = false);
    static QStringList findSimilarText(Poppler::Document* document,
                                       const QString& referenceText,
                                       double threshold = 0.7);
    static int countTextOccurrences(Poppler::Document* document,
                                    const QString& searchText,
                                    bool caseSensitive = false);

    // Quality assessment
    static QJsonObject assessDocumentQuality(Poppler::Document* document);
    static QJsonObject assessPageQuality(Poppler::Page* page);
    static double calculateTextClarity(Poppler::Page* page);
    static double calculateImageQuality(const QPixmap& image);
    static bool hasOptimalResolution(Poppler::Page* page,
                                     double targetDPI = 150.0);

    // Optimization utilities
    static QJsonObject suggestOptimizations(Poppler::Document* document);
    static QStringList identifyLargeImages(Poppler::Document* document,
                                           qint64 sizeThreshold = 1024 * 1024);
    static QStringList identifyDuplicateContent(Poppler::Document* document);
    static double estimateFileSize(Poppler::Document* document);

    // Accessibility utilities
    static QJsonObject assessAccessibility(Poppler::Document* document);
    static bool hasAlternativeText(Poppler::Document* document);
    static bool hasProperStructure(Poppler::Document* document);
    static QStringList identifyAccessibilityIssues(Poppler::Document* document);

    // Statistical functions
    static QJsonObject generateDocumentStatistics(Poppler::Document* document);
    static QJsonObject generatePageStatistics(Poppler::Page* page);
    static QJsonObject generateTextStatistics(const QString& text);
    static QJsonObject generateImageStatistics(const QList<QPixmap>& images);

private:
    // Helper functions
    static QString cleanText(const QString& text);
    static QStringList tokenizeText(const QString& text);
    static double calculateLevenshteinDistance(const QString& str1,
                                               const QString& str2);
    static QPixmap scalePixmap(const QPixmap& pixmap, const QSize& targetSize,
                               bool maintainAspectRatio);
    static QJsonObject pixmapToJson(const QPixmap& pixmap);
    static double calculateEntropy(const QString& text);
    static QStringList extractSentences(const QString& text);
    static QStringList extractParagraphs(const QString& text);
};
