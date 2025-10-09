#include "PDFUtilities.h"
#include <poppler/qt6/poppler-qt6.h>
#include <QBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QImageWriter>
#include <QJsonDocument>
#include <QList>
#include <QPixmap>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTextStream>
#include <QtCore>
#include <QtGlobal>
#include <QtMath>
#include <algorithm>
#include <exception>
#include <memory>
#include <vector>
#include "../model/AnnotationModel.h"
#include "Logger.h"

QJsonObject PDFUtilities::analyzeDocument(Poppler::Document* document) {
    QJsonObject analysis;

    if (!document) {
        analysis["error"] = "Invalid document";
        return analysis;
    }

    // Basic document info
    analysis["pageCount"] = document->numPages();
    analysis["title"] = document->info("Title");
    analysis["author"] = document->info("Author");
    analysis["subject"] = document->info("Subject");
    analysis["creator"] = document->info("Creator");
    analysis["producer"] = document->info("Producer");
    analysis["creationDate"] = document->info("CreationDate");
    analysis["modificationDate"] = document->info("ModDate");

    // Security info
    analysis["security"] = getDocumentSecurity(document);
    analysis["properties"] = getDocumentProperties(document);

    // Content analysis
    QStringList allText = extractAllText(document);
    QString fullText = allText.join(" ");

    analysis["textStatistics"] = generateTextStatistics(fullText);
    analysis["totalWords"] = countWords(fullText);
    analysis["totalSentences"] = countSentences(fullText);
    analysis["totalParagraphs"] = countParagraphs(fullText);
    analysis["estimatedReadingTime"] = calculateReadingTime(fullText);
    analysis["detectedLanguage"] = detectLanguage(fullText);

    // Image analysis
    QList<QPixmap> allImages = extractAllImages(document);
    analysis["imageStatistics"] = generateImageStatistics(allImages);
    analysis["totalImages"] = allImages.size();

    // Annotation analysis
    analysis["totalAnnotations"] = countAnnotations(document);
    QStringList annotationTypes = getAnnotationTypes(document);
    QJsonArray annotationTypesArray;
    for (const QString& type : annotationTypes) {
        annotationTypesArray.append(type);
    }
    analysis["annotationTypes"] = annotationTypesArray;

    // Quality assessment
    analysis["qualityAssessment"] = assessDocumentQuality(document);

    // Accessibility assessment
    analysis["accessibilityAssessment"] = assessAccessibility(document);

    // Optimization suggestions
    analysis["optimizationSuggestions"] = suggestOptimizations(document);

    analysis["analysisTimestamp"] =
        QDateTime::currentDateTime().toString(Qt::ISODate);

    return analysis;
}

QStringList PDFUtilities::extractAllText(Poppler::Document* document) {
    QStringList textList;

    if (!document) {
        Logger::instance().warning(
            "[utils] PDFUtilities::extractAllText: Invalid document pointer");
        return textList;
    }

    int pageCount = document->numPages();
    if (pageCount <= 0) {
        Logger::instance().warning(
            "[utils] PDFUtilities::extractAllText: Document has no pages");
        return textList;
    }

    if (pageCount > 10000) {
        Logger::instance().warning(
            "[utils] PDFUtilities::extractAllText: Very large document ({} "
            "pages) - this may take a long time",
            pageCount);
    }

    try {
        for (int i = 0; i < pageCount; ++i) {
            std::unique_ptr<Poppler::Page> page(document->page(i));
            if (page) {
                QString pageText = extractPageText(page.get());
                textList.append(pageText);
            } else {
                Logger::instance().warning(
                    "[utils] PDFUtilities::extractAllText: Failed to load page "
                    "{}",
                    i);
                textList.append(
                    QString());  // Add empty string to maintain page indexing
            }
        }
    } catch (const std::exception& e) {
        Logger::instance().warning(
            "[utils] PDFUtilities::extractAllText: Exception occurred: {}",
            e.what());
        return QStringList();  // Return empty list on error
    } catch (...) {
        Logger::instance().warning(
            "[utils] PDFUtilities::extractAllText: Unknown exception occurred");
        return QStringList();
    }

    return textList;
}

QList<QPixmap> PDFUtilities::extractAllImages(Poppler::Document* document) {
    QList<QPixmap> imageList;

    if (!document) {
        return imageList;
    }

    for (int i = 0; i < document->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            QList<QPixmap> pageImages = extractPageImages(page.get());
            imageList.append(pageImages);
        }
    }

    return imageList;
}

QJsonArray PDFUtilities::extractDocumentStructure(Poppler::Document* document) {
    QJsonArray structure;

    if (!document) {
        return structure;
    }

    for (int i = 0; i < document->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            QJsonObject pageInfo = analyzePage(page.get(), i);
            structure.append(pageInfo);
        }
    }

    return structure;
}

QJsonObject PDFUtilities::analyzePage(Poppler::Page* page, int pageNumber) {
    QJsonObject pageInfo;

    if (!page) {
        pageInfo["error"] = "Invalid page";
        return pageInfo;
    }

    pageInfo["pageNumber"] = pageNumber;
    pageInfo["size"] = QJsonObject{{"width", getPageSize(page).width()},
                                   {"height", getPageSize(page).height()}};
    pageInfo["rotation"] = getPageRotation(page);

    // Text analysis
    QString pageText = extractPageText(page);
    pageInfo["textLength"] = pageText.length();
    pageInfo["wordCount"] = countWords(pageText);
    pageInfo["sentenceCount"] = countSentences(pageText);
    pageInfo["paragraphCount"] = countParagraphs(pageText);

    // Image analysis
    QList<QPixmap> pageImages = extractPageImages(page);
    pageInfo["imageCount"] = pageImages.size();

    // Annotation analysis
    QJsonArray annotations = extractAnnotations(page);
    pageInfo["annotations"] = annotations;
    pageInfo["annotationCount"] = annotations.size();

    // Quality assessment
    pageInfo["qualityAssessment"] = assessPageQuality(page);

    return pageInfo;
}

QString PDFUtilities::extractPageText(Poppler::Page* page) {
    if (!page) {
        return QString();
    }

    return page->text(QRectF());
}

QList<QPixmap> PDFUtilities::extractPageImages(Poppler::Page* page) {
    QList<QPixmap> images;

    if (!page) {
        return images;
    }

    // This is a simplified implementation
    // In a real implementation, you would extract actual embedded images
    // For now, we'll render the page as an image
    QPixmap pageImage = renderPageToPixmap(page);
    if (!pageImage.isNull()) {
        images.append(pageImage);
    }

    return images;
}

QList<QRectF> PDFUtilities::findTextBounds(Poppler::Page* page,
                                           const QString& searchText) {
    QList<QRectF> bounds;

    if (!page || searchText.isEmpty()) {
        return bounds;
    }

    return PDFUtilities::searchText(page, searchText, false);
}

QSizeF PDFUtilities::getPageSize(Poppler::Page* page) {
    if (!page) {
        return QSizeF();
    }

    return page->pageSizeF();
}

double PDFUtilities::getPageRotation(Poppler::Page* page) {
    if (!page) {
        return 0.0;
    }

    return static_cast<double>(page->orientation());
}

int PDFUtilities::countWords(const QString& text) {
    if (text.isEmpty()) {
        return 0;
    }

    QStringList words = tokenizeText(text);
    return words.size();
}

int PDFUtilities::countSentences(const QString& text) {
    if (text.isEmpty()) {
        return 0;
    }

    QStringList sentences = extractSentences(text);
    return sentences.size();
}

int PDFUtilities::countParagraphs(const QString& text) {
    if (text.isEmpty()) {
        return 0;
    }

    QStringList paragraphs = extractParagraphs(text);
    return paragraphs.size();
}

QStringList PDFUtilities::extractKeywords(const QString& text,
                                          int maxKeywords) {
    QStringList keywords;

    if (text.isEmpty()) {
        return keywords;
    }

    // Simple keyword extraction based on word frequency
    QStringList words = tokenizeText(text.toLower());
    QHash<QString, int> wordCount;

    // Common stop words to filter out
    QStringList stopWords = {
        "the",   "a",      "an",  "and",   "or",  "but",  "in",
        "on",    "at",     "to",  "for",   "of",  "with", "by",
        "is",    "are",    "was", "were",  "be",  "been", "have",
        "has",   "had",    "do",  "does",  "did", "will", "would",
        "could", "should", "may", "might", "can", "this", "that",
        "these", "those",  "i",   "you",   "he",  "she",  "it",
        "we",    "they",   "me",  "him",   "her", "us",   "them"};

    for (const QString& word : words) {
        if (word.length() > 3 && !stopWords.contains(word)) {
            wordCount[word]++;
        }
    }

    // Sort by frequency and take top keywords
    QList<QPair<int, QString>> sortedWords;
    for (auto it = wordCount.begin(); it != wordCount.end(); ++it) {
        sortedWords.append(qMakePair(it.value(), it.key()));
    }

    std::sort(sortedWords.begin(), sortedWords.end(),
              std::greater<QPair<int, QString>>());

    for (int i = 0; i < qMin(maxKeywords, sortedWords.size()); ++i) {
        keywords.append(sortedWords[i].second);
    }

    return keywords;
}

double PDFUtilities::calculateReadingTime(const QString& text,
                                          int wordsPerMinute) {
    int wordCount = countWords(text);
    return static_cast<double>(wordCount) / wordsPerMinute;
}

QString PDFUtilities::detectLanguage(const QString& text) {
    // Simplified language detection based on common words
    // In a real implementation, you would use a proper language detection
    // library

    if (text.isEmpty()) {
        return "unknown";
    }

    QString lowerText = text.toLower();

    // English indicators
    QStringList englishWords = {"the", "and",  "that", "have", "for",
                                "not", "with", "you",  "this", "but"};
    int englishCount = 0;
    for (const QString& word : englishWords) {
        englishCount += lowerText.count(word);
    }

    // Chinese indicators (simplified check)
    QRegularExpression chineseRegex("[\\u4e00-\\u9fff]");
    int chineseCount = lowerText.count(chineseRegex);

    if (chineseCount > englishCount) {
        return "chinese";
    } else if (englishCount > 0) {
        return "english";
    }

    return "unknown";
}

QJsonObject PDFUtilities::analyzeImage(const QPixmap& image) {
    QJsonObject analysis;

    if (image.isNull()) {
        analysis["error"] = "Invalid image";
        return analysis;
    }

    analysis["width"] = image.width();
    analysis["height"] = image.height();
    analysis["depth"] = image.depth();
    analysis["hasAlpha"] = image.hasAlpha();
    analysis["isNull"] = image.isNull();

    // Calculate approximate file size
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    analysis["approximateSize"] = imageData.size();

    // Calculate quality metrics
    analysis["quality"] = calculateImageQuality(image);

    return analysis;
}

bool PDFUtilities::isImageDuplicate(const QPixmap& image1,
                                    const QPixmap& image2, double threshold) {
    if (image1.isNull() || image2.isNull()) {
        return false;
    }

    double similarity = calculateImageSimilarity(image1, image2);
    return similarity >= threshold;
}

QPixmap PDFUtilities::resizeImage(const QPixmap& image, const QSize& targetSize,
                                  bool maintainAspectRatio) {
    if (image.isNull()) {
        return QPixmap();
    }

    Qt::AspectRatioMode aspectMode =
        maintainAspectRatio ? Qt::KeepAspectRatio : Qt::IgnoreAspectRatio;
    return image.scaled(targetSize, aspectMode, Qt::SmoothTransformation);
}

QPixmap PDFUtilities::cropImage(const QPixmap& image, const QRectF& cropRect) {
    if (image.isNull() || cropRect.isEmpty()) {
        return QPixmap();
    }

    return image.copy(cropRect.toRect());
}

double PDFUtilities::calculateImageSimilarity(const QPixmap& image1,
                                              const QPixmap& image2) {
    if (image1.isNull() || image2.isNull()) {
        return 0.0;
    }

    // Simple similarity calculation based on size
    if (image1.size() != image2.size()) {
        return 0.5;  // Different sizes, moderate similarity
    }

    // Convert to images for pixel comparison
    QImage img1 = image1.toImage();
    QImage img2 = image2.toImage();

    if (img1.format() != img2.format()) {
        img1 = img1.convertToFormat(QImage::Format_RGB32);
        img2 = img2.convertToFormat(QImage::Format_RGB32);
    }

    int width = img1.width();
    int height = img1.height();
    int totalPixels = width * height;
    int differentPixels = 0;

    // Sample pixels for performance (every 4th pixel)
    for (int y = 0; y < height; y += 4) {
        for (int x = 0; x < width; x += 4) {
            if (img1.pixel(x, y) != img2.pixel(x, y)) {
                differentPixels++;
            }
        }
    }

    int sampledPixels = (width / 4) * (height / 4);
    return sampledPixels > 0
               ? 1.0 - (static_cast<double>(differentPixels) / sampledPixels)
               : 1.0;
}

// Helper functions
QString PDFUtilities::cleanText(const QString& text) {
    QString cleaned = text;
    cleaned = cleaned.replace(QRegularExpression("\\s+"), " ");
    cleaned = cleaned.trimmed();
    return cleaned;
}

QStringList PDFUtilities::tokenizeText(const QString& text) {
    QString cleaned = cleanText(text);
    QRegularExpression wordRegex("\\b\\w+\\b");
    QRegularExpressionMatchIterator matches = wordRegex.globalMatch(cleaned);

    QStringList words;
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        words.append(match.captured(0));
    }

    return words;
}

QStringList PDFUtilities::extractSentences(const QString& text) {
    QStringList sentences;
    QRegularExpression sentenceRegex("[.!?]+");
    QStringList parts = text.split(sentenceRegex, Qt::SkipEmptyParts);

    for (const QString& part : parts) {
        QString sentence = part.trimmed();
        if (!sentence.isEmpty()) {
            sentences.append(sentence);
        }
    }

    return sentences;
}

QStringList PDFUtilities::extractParagraphs(const QString& text) {
    QStringList paragraphs;
    QStringList parts =
        text.split(QRegularExpression("\\n\\s*\\n"), Qt::SkipEmptyParts);

    for (const QString& part : parts) {
        QString paragraph = part.trimmed();
        if (!paragraph.isEmpty()) {
            paragraphs.append(paragraph);
        }
    }

    return paragraphs;
}

double PDFUtilities::calculateLevenshteinDistance(const QString& str1,
                                                  const QString& str2) {
    int len1 = str1.length();
    int len2 = str2.length();

    if (len1 == 0)
        return len2;
    if (len2 == 0)
        return len1;

    QVector<QVector<int>> matrix(len1 + 1, QVector<int>(len2 + 1));

    for (int i = 0; i <= len1; i++) {
        matrix[i][0] = i;
    }

    for (int j = 0; j <= len2; j++) {
        matrix[0][j] = j;
    }

    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (str1[i - 1] == str2[j - 1]) ? 0 : 1;
            matrix[i][j] = qMin(qMin(matrix[i - 1][j] + 1,   // deletion
                                     matrix[i][j - 1] + 1),  // insertion
                                matrix[i - 1][j - 1] + cost  // substitution
            );
        }
    }

    return matrix[len1][len2];
}

// Additional utility functions
double PDFUtilities::calculateDocumentSimilarity(Poppler::Document* doc1,
                                                 Poppler::Document* doc2) {
    if (!doc1 || !doc2) {
        return 0.0;
    }

    // Compare page counts
    double pageCountSimilarity =
        1.0 - qAbs(doc1->numPages() - doc2->numPages()) /
                  static_cast<double>(qMax(doc1->numPages(), doc2->numPages()));

    // Compare text content
    QStringList text1 = extractAllText(doc1);
    QStringList text2 = extractAllText(doc2);

    QString fullText1 = text1.join(" ");
    QString fullText2 = text2.join(" ");

    double textSimilarity =
        1.0 - (calculateLevenshteinDistance(fullText1, fullText2) /
               qMax(fullText1.length(), fullText2.length()));

    // Weighted average
    return (pageCountSimilarity * 0.3 + textSimilarity * 0.7);
}

QJsonObject PDFUtilities::compareDocumentMetadata(Poppler::Document* doc1,
                                                  Poppler::Document* doc2) {
    QJsonObject comparison;

    if (!doc1 || !doc2) {
        comparison["error"] = "Invalid documents";
        return comparison;
    }

    comparison["pageCount"] =
        QJsonObject{{"doc1", doc1->numPages()},
                    {"doc2", doc2->numPages()},
                    {"same", doc1->numPages() == doc2->numPages()}};

    comparison["title"] =
        QJsonObject{{"doc1", doc1->info("Title")},
                    {"doc2", doc2->info("Title")},
                    {"same", doc1->info("Title") == doc2->info("Title")}};

    comparison["author"] =
        QJsonObject{{"doc1", doc1->info("Author")},
                    {"doc2", doc2->info("Author")},
                    {"same", doc1->info("Author") == doc2->info("Author")}};

    return comparison;
}

QPixmap PDFUtilities::renderPageToPixmap(Poppler::Page* page, double dpi) {
    if (!page) {
        Logger::instance().warning(
            "[utils] PDFUtilities::renderPageToPixmap: Invalid page pointer");
        return QPixmap();
    }

    if (dpi <= 0 || dpi > 600) {
        Logger::instance().warning(
            "[utils] PDFUtilities::renderPageToPixmap: Invalid DPI value: {} - "
            "using default 150 DPI",
            dpi);
        dpi = 150.0;
    }

    try {
        QImage image = page->renderToImage(dpi, dpi);
        if (image.isNull()) {
            Logger::instance().warning(
                "[utils] PDFUtilities::renderPageToPixmap: Failed to render "
                "page to image");
            return QPixmap();
        }
        return QPixmap::fromImage(image);
    } catch (const std::exception& e) {
        Logger::instance().warning(
            "[utils] PDFUtilities::renderPageToPixmap: Exception occurred: {}",
            e.what());
        return QPixmap();
    } catch (...) {
        Logger::instance().warning(
            "[utils] PDFUtilities::renderPageToPixmap: Unknown exception "
            "occurred");
        return QPixmap();
    }
}

QPixmap PDFUtilities::renderPageRegion(Poppler::Page* page,
                                       const QRectF& region, double dpi) {
    if (!page) {
        return QPixmap();
    }

    QImage image = page->renderToImage(dpi, dpi, region.x(), region.y(),
                                       region.width(), region.height());
    return QPixmap::fromImage(image);
}

QList<QPixmap> PDFUtilities::renderDocumentThumbnails(
    Poppler::Document* document, const QSize& thumbnailSize) {
    QList<QPixmap> thumbnails;

    if (!document) {
        return thumbnails;
    }

    for (int i = 0; i < document->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            QPixmap pagePixmap =
                renderPageToPixmap(page.get(), 72.0);  // Low DPI for thumbnails
            QPixmap thumbnail = resizeImage(pagePixmap, thumbnailSize, true);
            thumbnails.append(thumbnail);
        }
    }

    return thumbnails;
}

QPixmap PDFUtilities::createPagePreview(Poppler::Page* page,
                                        const QSize& previewSize) {
    if (!page) {
        return QPixmap();
    }

    QPixmap pagePixmap = renderPageToPixmap(page, 150.0);
    return resizeImage(pagePixmap, previewSize, true);
}

QJsonArray PDFUtilities::extractAnnotations(Poppler::Page* page) {
    QJsonArray annotations;

    if (!page) {
        return annotations;
    }

    std::vector<std::unique_ptr<Poppler::Annotation>> pageAnnotations =
        page->annotations();

    for (const auto& annotation : pageAnnotations) {
        QJsonObject annotationObj = analyzeAnnotation(annotation.get());
        annotations.append(annotationObj);
    }

    return annotations;
}

QJsonObject PDFUtilities::analyzeAnnotation(Poppler::Annotation* annotation) {
    QJsonObject analysis;

    if (!annotation) {
        analysis["error"] = "Invalid annotation";
        return analysis;
    }

    analysis["type"] = static_cast<int>(annotation->subType());
    analysis["author"] = annotation->author();
    analysis["contents"] = annotation->contents();
    analysis["creationDate"] = annotation->creationDate().toString(Qt::ISODate);
    analysis["modificationDate"] =
        annotation->modificationDate().toString(Qt::ISODate);

    QRectF boundary = annotation->boundary();
    analysis["boundary"] = QJsonObject{{"x", boundary.x()},
                                       {"y", boundary.y()},
                                       {"width", boundary.width()},
                                       {"height", boundary.height()}};

    return analysis;
}

int PDFUtilities::countAnnotations(Poppler::Document* document) {
    if (!document) {
        return 0;
    }

    int totalAnnotations = 0;

    for (int i = 0; i < document->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            totalAnnotations += page->annotations().size();
        }
    }

    return totalAnnotations;
}

QStringList PDFUtilities::getAnnotationTypes(Poppler::Document* document) {
    QStringList types;
    QSet<int> uniqueTypes;

    if (!document) {
        return types;
    }

    for (int i = 0; i < document->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            std::vector<std::unique_ptr<Poppler::Annotation>> annotations =
                page->annotations();
            for (const auto& annotation : annotations) {
                uniqueTypes.insert(static_cast<int>(annotation->subType()));
            }
        }
    }

    for (int type : uniqueTypes) {
        types.append(QString::number(type));
    }

    return types;
}

QJsonObject PDFUtilities::getDocumentSecurity(Poppler::Document* document) {
    QJsonObject security;

    if (!document) {
        security["error"] = "Invalid document";
        return security;
    }

    security["isEncrypted"] = isDocumentEncrypted(document);
    security["canExtractText"] = canExtractText(document);
    security["canPrint"] = canPrint(document);
    security["canModify"] = canModify(document);

    return security;
}

QJsonObject PDFUtilities::getDocumentProperties(Poppler::Document* document) {
    QJsonObject properties;

    if (!document) {
        properties["error"] = "Invalid document";
        return properties;
    }

    properties["title"] = document->info("Title");
    properties["author"] = document->info("Author");
    properties["subject"] = document->info("Subject");
    properties["keywords"] = document->info("Keywords");
    properties["creator"] = document->info("Creator");
    properties["producer"] = document->info("Producer");
    properties["creationDate"] = document->info("CreationDate");
    properties["modificationDate"] = document->info("ModDate");

    return properties;
}

bool PDFUtilities::isDocumentEncrypted(Poppler::Document* document) {
    if (!document) {
        return false;
    }

    return document->isEncrypted();
}

bool PDFUtilities::canExtractText(Poppler::Document* document) {
    if (!document) {
        return false;
    }

    // Try to extract text from first page to test permissions
    if (document->numPages() > 0) {
        std::unique_ptr<Poppler::Page> page(document->page(0));
        if (page) {
            QString text = page->text(QRectF());
            return !text.isEmpty() ||
                   true;  // Allow even if no text (might be image-only)
        }
    }

    return true;
}

bool PDFUtilities::canPrint(Poppler::Document* document) {
    if (!document) {
        return false;
    }

    // This would need to check document permissions
    // For now, assume printing is allowed
    return true;
}

bool PDFUtilities::canModify(Poppler::Document* document) {
    if (!document) {
        return false;
    }

    // This would need to check document permissions
    // For now, assume modification is not allowed for security
    return false;
}

bool PDFUtilities::exportPageAsImage(Poppler::Page* page,
                                     const QString& filePath,
                                     const QString& format) {
    if (!page) {
        return false;
    }

    QPixmap pagePixmap = renderPageToPixmap(page);
    return pagePixmap.save(filePath, format.toUtf8().constData());
}

bool PDFUtilities::exportDocumentAsImages(Poppler::Document* document,
                                          const QString& outputDir,
                                          const QString& format) {
    if (!document) {
        return false;
    }

    QDir dir(outputDir);
    if (!dir.exists()) {
        dir.mkpath(outputDir);
    }

    bool success = true;

    for (int i = 0; i < document->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            QString fileName = QString("page_%1.%2")
                                   .arg(i + 1, 3, 10, QChar('0'))
                                   .arg(format.toLower());
            QString filePath = dir.absoluteFilePath(fileName);

            if (!exportPageAsImage(page.get(), filePath, format)) {
                success = false;
            }
        }
    }

    return success;
}

bool PDFUtilities::exportTextToFile(const QString& text,
                                    const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << text;

    return true;
}

bool PDFUtilities::exportAnalysisToJson(const QJsonObject& analysis,
                                        const QString& filePath) {
    QJsonDocument doc(analysis);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    return true;
}

bool PDFUtilities::savePDFWithAnnotations(
    Poppler::Document* document, const QString& filePath,
    const QList<PDFAnnotation>& annotations) {
    if (!document) {
        Logger::instance().warning("[utils] Cannot save PDF: document is null");
        return false;
    }

    if (filePath.isEmpty()) {
        Logger::instance().warning(
            "[utils] Cannot save PDF: file path is empty");
        return false;
    }

    try {
        // Note: Poppler doesn't directly support saving modified PDFs with
        // annotations This implementation provides a basic file copy approach
        // In a production environment, you would need a PDF writing library
        // like QPdf or PoDoFo

        // For now, we'll copy the original document and note that annotation
        // embedding would require additional PDF manipulation libraries

        // Try to determine source file path - this is a limitation of current
        // approach In a real implementation, the document source path should be
        // tracked separately
        QString sourceFile;

        // Since we can't reliably get the source path from Poppler::Document,
        // we'll need to pass it as a parameter or track it elsewhere
        // For now, we'll create a basic copy mechanism

        // Create a temporary approach: render pages and create new PDF
        // This is not ideal but works as a fallback
        Logger::instance().warning(
            "[utils] PDF save with annotations requires additional PDF writing "
            "libraries");
        Logger::instance().warning(
            "[utils] Current implementation provides basic file operations "
            "only");

        // For now, return false to indicate this feature needs additional
        // implementation The UI will show an appropriate error message
        return false;

    } catch (const std::exception& e) {
        Logger::instance().warning("[utils] Exception while saving PDF: {}",
                                   e.what());
        return false;
    } catch (...) {
        Logger::instance().warning(
            "[utils] Unknown exception while saving PDF");
        return false;
    }
}

// Missing function implementations

QJsonObject PDFUtilities::generateTextStatistics(const QString& text) {
    QJsonObject stats;

    if (text.isEmpty()) {
        stats["wordCount"] = 0;
        stats["characterCount"] = 0;
        stats["sentenceCount"] = 0;
        stats["paragraphCount"] = 0;
        return stats;
    }

    stats["wordCount"] = countWords(text);
    stats["characterCount"] = text.length();
    stats["sentenceCount"] = countSentences(text);
    stats["paragraphCount"] = countParagraphs(text);
    stats["averageWordsPerSentence"] =
        stats["sentenceCount"].toInt() > 0
            ? static_cast<double>(stats["wordCount"].toInt()) /
                  stats["sentenceCount"].toInt()
            : 0.0;

    return stats;
}

QJsonObject PDFUtilities::generateImageStatistics(
    const QList<QPixmap>& images) {
    QJsonObject stats;

    stats["totalImages"] = images.size();

    if (images.isEmpty()) {
        stats["averageWidth"] = 0;
        stats["averageHeight"] = 0;
        stats["totalPixels"] = 0;
        return stats;
    }

    int totalWidth = 0;
    int totalHeight = 0;
    qint64 totalPixels = 0;

    for (const QPixmap& image : images) {
        totalWidth += image.width();
        totalHeight += image.height();
        totalPixels += static_cast<qint64>(image.width()) * image.height();
    }

    stats["averageWidth"] = static_cast<double>(totalWidth) / images.size();
    stats["averageHeight"] = static_cast<double>(totalHeight) / images.size();
    stats["totalPixels"] = totalPixels;

    return stats;
}

QJsonObject PDFUtilities::assessDocumentQuality(Poppler::Document* document) {
    QJsonObject quality;

    if (!document) {
        quality["error"] = "Invalid document";
        return quality;
    }

    double qualityScore = 1.0;
    QStringList issues;

    // Basic quality checks
    int pageCount = document->numPages();
    if (pageCount < 1) {
        qualityScore -= 0.5;
        issues.append("No pages found");
    } else if (pageCount > 1000) {
        qualityScore -= 0.1;
        issues.append("Very large document");
    }

    // Check for text content in first few pages
    bool hasText = false;
    for (int i = 0; i < qMin(5, pageCount); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            QString pageText = page->text(QRectF());
            if (!pageText.trimmed().isEmpty()) {
                hasText = true;
                break;
            }
        }
    }

    if (!hasText) {
        qualityScore -= 0.3;
        issues.append("No readable text found");
    }

    quality["score"] = qMax(0.0, qualityScore);
    QJsonArray issuesArray;
    for (const QString& issue : issues) {
        issuesArray.append(issue);
    }
    quality["issues"] = issuesArray;

    return quality;
}

QJsonObject PDFUtilities::assessAccessibility(Poppler::Document* document) {
    QJsonObject accessibility;

    if (!document) {
        accessibility["error"] = "Invalid document";
        return accessibility;
    }

    double accessibilityScore = 1.0;
    QStringList issues;

    // Check for text content (important for screen readers)
    bool hasText = false;
    for (int i = 0; i < qMin(3, document->numPages()); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            QString pageText = page->text(QRectF());
            if (!pageText.trimmed().isEmpty()) {
                hasText = true;
                break;
            }
        }
    }

    if (!hasText) {
        accessibilityScore -= 0.5;
        issues.append(
            "No readable text found - may not be accessible to screen readers");
    }

    // Check document metadata
    QString title = document->info("Title");
    if (title.isEmpty()) {
        accessibilityScore -= 0.2;
        issues.append("Missing document title");
    }

    accessibility["score"] = qMax(0.0, accessibilityScore);
    QJsonArray issuesArray;
    for (const QString& issue : issues) {
        issuesArray.append(issue);
    }
    accessibility["issues"] = issuesArray;

    return accessibility;
}

// Overloaded savePDFWithAnnotations function
bool PDFUtilities::savePDFWithAnnotations(Poppler::Document* document,
                                          const QString& filePath) {
    QList<PDFAnnotation> emptyAnnotations;
    return savePDFWithAnnotations(document, filePath, emptyAnnotations);
}

// Missing function implementations

QJsonObject PDFUtilities::suggestOptimizations(Poppler::Document* document) {
    QJsonObject suggestions;
    QJsonArray optimizations;

    if (!document) {
        suggestions["error"] = "Invalid document";
        return suggestions;
    }

    // Check document size and suggest compression
    int pageCount = document->numPages();
    if (pageCount > 100) {
        QJsonObject opt;
        opt["type"] = "compression";
        opt["description"] = "Large document detected - consider compression";
        opt["priority"] = "medium";
        optimizations.append(opt);
    }

    // Check for images and suggest optimization
    bool hasImages = false;
    for (int i = 0; i < qMin(5, pageCount); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (page) {
            // Simple heuristic: check if page has significant non-text content
            QSizeF pageSize = page->pageSizeF();
            QString pageText = page->text(QRectF());
            if (pageText.length() <
                pageSize.width() * pageSize.height() / 1000) {
                hasImages = true;  // Likely has images if text is sparse
                break;
            }
        }
    }

    if (hasImages) {
        QJsonObject opt;
        opt["type"] = "image_optimization";
        opt["description"] =
            "Images detected - consider optimizing image quality/size";
        opt["priority"] = "low";
        optimizations.append(opt);
    }

    suggestions["optimizations"] = optimizations;
    suggestions["count"] = optimizations.size();

    return suggestions;
}

QJsonObject PDFUtilities::assessPageQuality(Poppler::Page* page) {
    QJsonObject quality;

    if (!page) {
        quality["error"] = "Invalid page";
        return quality;
    }

    double qualityScore = 1.0;
    QStringList issues;

    // Check page size
    QSizeF pageSize = page->pageSizeF();
    if (pageSize.width() < 100 || pageSize.height() < 100) {
        qualityScore -= 0.3;
        issues.append("Very small page size");
    }

    // Check text content
    QString pageText = page->text(QRectF());
    if (pageText.trimmed().isEmpty()) {
        qualityScore -= 0.4;
        issues.append("No readable text found");
    } else if (pageText.length() < 50) {
        qualityScore -= 0.2;
        issues.append("Very little text content");
    }

    quality["score"] = qMax(0.0, qualityScore);
    QJsonArray issuesArray;
    for (const QString& issue : issues) {
        issuesArray.append(issue);
    }
    quality["issues"] = issuesArray;

    return quality;
}

QList<QRectF> PDFUtilities::searchText(Poppler::Page* page,
                                       const QString& searchText,
                                       bool caseSensitive) {
    QList<QRectF> results;

    if (!page || searchText.isEmpty()) {
        return results;
    }

    // Use Poppler's search functionality
    Poppler::Page::SearchFlags flags = Poppler::Page::NoSearchFlags;
    if (caseSensitive) {
        flags |= Poppler::Page::IgnoreCase;
    }

    return page->search(searchText, flags);
}

double PDFUtilities::calculateImageQuality(const QPixmap& image) {
    if (image.isNull()) {
        return 0.0;
    }

    // Simple quality assessment based on size and depth
    double quality = 1.0;

    // Check resolution
    int width = image.width();
    int height = image.height();
    int totalPixels = width * height;

    if (totalPixels < 10000) {  // Very small image
        quality -= 0.3;
    } else if (totalPixels > 4000000) {  // Very large image
        quality += 0.1;                  // Bonus for high resolution
    }

    // Check color depth
    int depth = image.depth();
    if (depth < 24) {
        quality -= 0.2;  // Lower quality for reduced color depth
    }

    return qMax(0.0, qMin(1.0, quality));
}
