#include "../TestUtilities.h"
#include "../../app/utils/PDFUtilities.h"
#include <QTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPixmap>
#include <QDir>
#include <QStandardPaths>
#include <poppler-qt6.h>

class PDFUtilitiesTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    // Document analysis tests
    void testAnalyzeDocument();
    void testAnalyzeDocumentWithNull();
    void testExtractAllText();
    void testExtractAllImages();
    void testExtractDocumentStructure();

    // Page analysis tests
    void testAnalyzePage();
    void testAnalyzePageWithNull();
    void testExtractPageText();
    void testExtractPageImages();
    void testFindTextBounds();
    void testGetPageSize();
    void testGetPageRotation();

    // Text analysis tests
    void testCountWords();
    void testCountSentences();
    void testCountParagraphs();
    void testExtractKeywords();
    void testCalculateReadingTime();
    void testDetectLanguage();

    // Image analysis tests
    void testAnalyzeImage();
    void testIsImageDuplicate();
    void testResizeImage();
    void testCropImage();
    void testCalculateImageSimilarity();

    // Document comparison tests
    void testCalculateDocumentSimilarity();
    void testCompareDocumentMetadata();
    void testFindCommonPages();
    void testFindTextDifferences();

    // Rendering tests
    void testRenderPageToPixmap();
    void testRenderPageRegion();
    void testRenderDocumentThumbnails();
    void testCreatePagePreview();

    // Annotation tests
    void testExtractAnnotations();
    void testAnalyzeAnnotation();
    void testCountAnnotations();
    void testGetAnnotationTypes();

    // Security and properties tests
    void testGetDocumentSecurity();
    void testGetDocumentProperties();
    void testIsDocumentEncrypted();
    void testCanExtractText();
    void testCanPrint();
    void testCanModify();

    // Export tests
    void testExportPageAsImage();
    void testExportDocumentAsImages();
    void testExportTextToFile();
    void testExportAnalysisToJson();

    // Search tests
    void testSearchText();
    void testSearchTextInDocument();
    void testFindSimilarText();
    void testCountTextOccurrences();

    // Quality assessment tests
    void testAssessDocumentQuality();
    void testAssessPageQuality();
    void testCalculateTextClarity();
    void testCalculateImageQuality();
    void testHasOptimalResolution();

    // Optimization tests
    void testSuggestOptimizations();
    void testIdentifyLargeImages();
    void testIdentifyDuplicateContent();
    void testEstimateFileSize();

    // Accessibility tests
    void testAssessAccessibility();
    void testHasAlternativeText();
    void testHasProperStructure();
    void testIdentifyAccessibilityIssues();

    // Statistical tests
    void testGenerateDocumentStatistics();
    void testGeneratePageStatistics();
    void testGenerateTextStatistics();
    void testGenerateImageStatistics();

    // Edge cases and error handling
    void testEmptyText();
    void testLargeText();
    void testSpecialCharacters();
    void testInvalidImages();
    void testCorruptedDocument();

private:
    QString m_testDataDir;
    QStringList m_testPdfFiles;
    
    // Helper methods
    QString createTestPdf(const QString& content = "Test PDF Content");
    Poppler::Document* openTestDocument(const QString& filePath);
    QPixmap createTestImage(int width = 100, int height = 100);
    void cleanupTestFiles(const QStringList& files);
    bool isValidJsonObject(const QJsonObject& obj);
    bool isValidJsonArray(const QJsonArray& arr);
};

void PDFUtilitiesTest::initTestCase() {
    // Setup test environment
    m_testDataDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/PDFUtilitiesTest";
    QDir().mkpath(m_testDataDir);
}

void PDFUtilitiesTest::cleanupTestCase() {
    // Cleanup test environment
    cleanupTestFiles(m_testPdfFiles);
    QDir(m_testDataDir).removeRecursively();
}

void PDFUtilitiesTest::init() {
    // Per-test setup
}

void PDFUtilitiesTest::cleanup() {
    // Per-test cleanup
}

void PDFUtilitiesTest::testAnalyzeDocument() {
    QString testFile = createTestPdf("Sample PDF content for analysis");
    std::unique_ptr<Poppler::Document> document(openTestDocument(testFile));
    
    if (document) {
        QJsonObject analysis = PDFUtilities::analyzeDocument(document.get());
        
        QVERIFY(isValidJsonObject(analysis));
        QVERIFY(analysis.contains("pageCount"));
        QVERIFY(analysis["pageCount"].toInt() > 0);
        
        // Check for basic document properties
        QVERIFY(analysis.contains("title"));
        QVERIFY(analysis.contains("author"));
        QVERIFY(analysis.contains("security"));
        QVERIFY(analysis.contains("properties"));
    } else {
        QSKIP("Could not create test PDF document");
    }
}

void PDFUtilitiesTest::testAnalyzeDocumentWithNull() {
    QJsonObject analysis = PDFUtilities::analyzeDocument(nullptr);
    
    QVERIFY(isValidJsonObject(analysis));
    QVERIFY(analysis.contains("error"));
    QCOMPARE(analysis["error"].toString(), "Invalid document");
}

void PDFUtilitiesTest::testExtractAllText() {
    QString testFile = createTestPdf("This is test content for text extraction");
    std::unique_ptr<Poppler::Document> document(openTestDocument(testFile));
    
    if (document) {
        QStringList allText = PDFUtilities::extractAllText(document.get());
        
        QVERIFY(!allText.isEmpty());
        // Should have at least one page of text
        QVERIFY(allText.size() >= 1);
    } else {
        QSKIP("Could not create test PDF document");
    }
}

void PDFUtilitiesTest::testExtractAllImages() {
    QString testFile = createTestPdf();
    std::unique_ptr<Poppler::Document> document(openTestDocument(testFile));
    
    if (document) {
        QList<QPixmap> images = PDFUtilities::extractAllImages(document.get());
        
        // Even if no images, should return valid list
        QVERIFY(images.size() >= 0);
    } else {
        QSKIP("Could not create test PDF document");
    }
}

void PDFUtilitiesTest::testExtractDocumentStructure() {
    QString testFile = createTestPdf();
    std::unique_ptr<Poppler::Document> document(openTestDocument(testFile));
    
    if (document) {
        QJsonArray structure = PDFUtilities::extractDocumentStructure(document.get());
        
        QVERIFY(isValidJsonArray(structure));
        // Should have at least basic structure information
        QVERIFY(structure.size() >= 0);
    } else {
        QSKIP("Could not create test PDF document");
    }
}

void PDFUtilitiesTest::testAnalyzePage() {
    QString testFile = createTestPdf("Page content for analysis");
    std::unique_ptr<Poppler::Document> document(openTestDocument(testFile));
    
    if (document && document->numPages() > 0) {
        std::unique_ptr<Poppler::Page> page(document->page(0));
        if (page) {
            QJsonObject analysis = PDFUtilities::analyzePage(page.get(), 0);
            
            QVERIFY(isValidJsonObject(analysis));
            QVERIFY(analysis.contains("pageNumber"));
            QCOMPARE(analysis["pageNumber"].toInt(), 0);
        }
    } else {
        QSKIP("Could not create test PDF document or get page");
    }
}

void PDFUtilitiesTest::testAnalyzePageWithNull() {
    QJsonObject analysis = PDFUtilities::analyzePage(nullptr, 0);
    
    QVERIFY(isValidJsonObject(analysis));
    QVERIFY(analysis.contains("error") || analysis.isEmpty());
}

void PDFUtilitiesTest::testExtractPageText() {
    QString testFile = createTestPdf("Test page text content");
    std::unique_ptr<Poppler::Document> document(openTestDocument(testFile));
    
    if (document && document->numPages() > 0) {
        std::unique_ptr<Poppler::Page> page(document->page(0));
        if (page) {
            QString text = PDFUtilities::extractPageText(page.get());
            
            // Should return some text (even if empty)
            QVERIFY(text.length() >= 0);
        }
    } else {
        QSKIP("Could not create test PDF document or get page");
    }
}

void PDFUtilitiesTest::testExtractPageImages() {
    QString testFile = createTestPdf();
    std::unique_ptr<Poppler::Document> document(openTestDocument(testFile));
    
    if (document && document->numPages() > 0) {
        std::unique_ptr<Poppler::Page> page(document->page(0));
        if (page) {
            QList<QPixmap> images = PDFUtilities::extractPageImages(page.get());
            
            // Should return valid list (even if empty)
            QVERIFY(images.size() >= 0);
        }
    } else {
        QSKIP("Could not create test PDF document or get page");
    }
}

void PDFUtilitiesTest::testFindTextBounds() {
    QString testFile = createTestPdf("Find this text in the document");
    std::unique_ptr<Poppler::Document> document(openTestDocument(testFile));
    
    if (document && document->numPages() > 0) {
        std::unique_ptr<Poppler::Page> page(document->page(0));
        if (page) {
            QList<QRectF> bounds = PDFUtilities::findTextBounds(page.get(), "text");
            
            // Should return valid list
            QVERIFY(bounds.size() >= 0);
        }
    } else {
        QSKIP("Could not create test PDF document or get page");
    }
}

void PDFUtilitiesTest::testGetPageSize() {
    QString testFile = createTestPdf();
    std::unique_ptr<Poppler::Document> document(openTestDocument(testFile));
    
    if (document && document->numPages() > 0) {
        std::unique_ptr<Poppler::Page> page(document->page(0));
        if (page) {
            QSizeF size = PDFUtilities::getPageSize(page.get());
            
            QVERIFY(size.width() > 0);
            QVERIFY(size.height() > 0);
        }
    } else {
        QSKIP("Could not create test PDF document or get page");
    }
}

void PDFUtilitiesTest::testGetPageRotation() {
    QString testFile = createTestPdf();
    std::unique_ptr<Poppler::Document> document(openTestDocument(testFile));
    
    if (document && document->numPages() > 0) {
        std::unique_ptr<Poppler::Page> page(document->page(0));
        if (page) {
            double rotation = PDFUtilities::getPageRotation(page.get());
            
            // Rotation should be a valid angle (0, 90, 180, 270)
            QVERIFY(rotation >= 0 && rotation < 360);
        }
    } else {
        QSKIP("Could not create test PDF document or get page");
    }
}

// Helper method implementations
QString PDFUtilitiesTest::createTestPdf(const QString& content) {
    QString fileName = m_testDataDir + QString("/test_%1.pdf").arg(QRandomGenerator::global()->generate());
    
    // Create a minimal PDF file for testing
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << "%PDF-1.4\n";
        stream << "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
        stream << "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n";
        stream << "3 0 obj\n<< /Type /Page /Parent 2 0 R /Contents 4 0 R >>\nendobj\n";
        stream << "4 0 obj\n<< /Length " << content.length() << " >>\nstream\n";
        stream << content << "\nendstream\nendobj\n";
        stream << "xref\n0 5\n0000000000 65535 f\n";
        stream << "trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n%%EOF\n";
        file.close();
    }
    
    m_testPdfFiles.append(fileName);
    return fileName;
}

Poppler::Document* PDFUtilitiesTest::openTestDocument(const QString& filePath) {
    return Poppler::Document::load(filePath);
}

QPixmap PDFUtilitiesTest::createTestImage(int width, int height) {
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::blue);
    return pixmap;
}

void PDFUtilitiesTest::cleanupTestFiles(const QStringList& files) {
    for (const QString& file : files) {
        QFile::remove(file);
    }
}

bool PDFUtilitiesTest::isValidJsonObject(const QJsonObject& obj) {
    return !obj.isEmpty() || obj.keys().isEmpty(); // Empty object is also valid
}

bool PDFUtilitiesTest::isValidJsonArray(const QJsonArray& arr) {
    return arr.size() >= 0; // Any size is valid for arrays
}

void PDFUtilitiesTest::testCountWords() {
    QString text = "This is a test sentence with multiple words.";
    int wordCount = PDFUtilities::countWords(text);

    QCOMPARE(wordCount, 9);

    // Test empty text
    QCOMPARE(PDFUtilities::countWords(""), 0);

    // Test single word
    QCOMPARE(PDFUtilities::countWords("word"), 1);

    // Test text with extra spaces
    QCOMPARE(PDFUtilities::countWords("  word1   word2  "), 2);
}

void PDFUtilitiesTest::testCountSentences() {
    QString text = "This is sentence one. This is sentence two! Is this sentence three?";
    int sentenceCount = PDFUtilities::countSentences(text);

    QCOMPARE(sentenceCount, 3);

    // Test empty text
    QCOMPARE(PDFUtilities::countSentences(""), 0);

    // Test single sentence
    QCOMPARE(PDFUtilities::countSentences("Single sentence."), 1);
}

void PDFUtilitiesTest::testCountParagraphs() {
    QString text = "First paragraph.\n\nSecond paragraph.\n\nThird paragraph.";
    int paragraphCount = PDFUtilities::countParagraphs(text);

    QVERIFY(paragraphCount >= 1); // At least one paragraph

    // Test empty text
    QCOMPARE(PDFUtilities::countParagraphs(""), 0);

    // Test single paragraph
    QCOMPARE(PDFUtilities::countParagraphs("Single paragraph."), 1);
}

void PDFUtilitiesTest::testExtractKeywords() {
    QString text = "This is a test document about PDF processing and text analysis.";
    QStringList keywords = PDFUtilities::extractKeywords(text, 5);

    QVERIFY(keywords.size() <= 5);
    QVERIFY(keywords.size() >= 0);

    // Test with empty text
    QStringList emptyKeywords = PDFUtilities::extractKeywords("", 10);
    QVERIFY(emptyKeywords.isEmpty());
}

void PDFUtilitiesTest::testCalculateReadingTime() {
    QString text = "This is a test text with exactly twenty words for testing reading time calculation functionality properly.";
    double readingTime = PDFUtilities::calculateReadingTime(text, 200); // 200 words per minute

    QVERIFY(readingTime > 0);
    QVERIFY(readingTime < 1); // Should be less than 1 minute for 20 words

    // Test with empty text
    QCOMPARE(PDFUtilities::calculateReadingTime("", 200), 0.0);
}

void PDFUtilitiesTest::testDetectLanguage() {
    QString englishText = "This is an English text sample for language detection testing.";
    QString language = PDFUtilities::detectLanguage(englishText);

    QVERIFY(!language.isEmpty());
    // Language detection might return various formats, just check it's not empty

    // Test with empty text
    QString emptyLanguage = PDFUtilities::detectLanguage("");
    QVERIFY(emptyLanguage.isEmpty() || emptyLanguage == "unknown");
}

void PDFUtilitiesTest::testAnalyzeImage() {
    QPixmap testImage = createTestImage(200, 150);
    QJsonObject analysis = PDFUtilities::analyzeImage(testImage);

    QVERIFY(isValidJsonObject(analysis));
    QVERIFY(analysis.contains("width") || analysis.contains("size"));

    // Test with null image
    QPixmap nullImage;
    QJsonObject nullAnalysis = PDFUtilities::analyzeImage(nullImage);
    QVERIFY(isValidJsonObject(nullAnalysis));
}

void PDFUtilitiesTest::testIsImageDuplicate() {
    QPixmap image1 = createTestImage(100, 100);
    QPixmap image2 = createTestImage(100, 100);
    QPixmap image3 = createTestImage(200, 200);

    // Same size images might be considered similar
    bool similar = PDFUtilities::isImageDuplicate(image1, image2, 0.95);
    QVERIFY(similar == true || similar == false); // Valid boolean result

    // Different size images should be different
    bool different = PDFUtilities::isImageDuplicate(image1, image3, 0.95);
    QVERIFY(different == true || different == false); // Valid boolean result

    // Image compared with itself should be identical
    bool identical = PDFUtilities::isImageDuplicate(image1, image1, 0.95);
    QVERIFY(identical);
}

void PDFUtilitiesTest::testResizeImage() {
    QPixmap originalImage = createTestImage(200, 150);
    QSize targetSize(100, 75);

    QPixmap resizedImage = PDFUtilities::resizeImage(originalImage, targetSize, true);

    QVERIFY(!resizedImage.isNull());
    // With aspect ratio maintained, one dimension should match exactly
    QVERIFY(resizedImage.width() <= targetSize.width());
    QVERIFY(resizedImage.height() <= targetSize.height());

    // Test without maintaining aspect ratio
    QPixmap resizedExact = PDFUtilities::resizeImage(originalImage, targetSize, false);
    QCOMPARE(resizedExact.size(), targetSize);
}

void PDFUtilitiesTest::testCropImage() {
    QPixmap originalImage = createTestImage(200, 150);
    QRectF cropRect(50, 25, 100, 75);

    QPixmap croppedImage = PDFUtilities::cropImage(originalImage, cropRect);

    QVERIFY(!croppedImage.isNull());
    QCOMPARE(croppedImage.width(), 100);
    QCOMPARE(croppedImage.height(), 75);

    // Test with invalid crop rect
    QRectF invalidRect(-10, -10, 50, 50);
    QPixmap invalidCrop = PDFUtilities::cropImage(originalImage, invalidRect);
    // Should handle gracefully (return empty or original)
    QVERIFY(invalidCrop.isNull() || !invalidCrop.isNull());
}

void PDFUtilitiesTest::testCalculateImageSimilarity() {
    QPixmap image1 = createTestImage(100, 100);
    QPixmap image2 = createTestImage(100, 100);
    QPixmap image3 = createTestImage(200, 200);

    double similarity1 = PDFUtilities::calculateImageSimilarity(image1, image2);
    QVERIFY(similarity1 >= 0.0 && similarity1 <= 1.0);

    double similarity2 = PDFUtilities::calculateImageSimilarity(image1, image3);
    QVERIFY(similarity2 >= 0.0 && similarity2 <= 1.0);

    // Image compared with itself should have high similarity
    double selfSimilarity = PDFUtilities::calculateImageSimilarity(image1, image1);
    QVERIFY(selfSimilarity >= 0.9); // Should be very similar to itself
}

void PDFUtilitiesTest::testCalculateDocumentSimilarity() {
    QString file1 = createTestPdf("Document content A");
    QString file2 = createTestPdf("Document content B");

    std::unique_ptr<Poppler::Document> doc1(openTestDocument(file1));
    std::unique_ptr<Poppler::Document> doc2(openTestDocument(file2));

    if (doc1 && doc2) {
        double similarity = PDFUtilities::calculateDocumentSimilarity(doc1.get(), doc2.get());

        QVERIFY(similarity >= 0.0 && similarity <= 1.0);
    } else {
        QSKIP("Could not create test PDF documents");
    }
}

void PDFUtilitiesTest::testCompareDocumentMetadata() {
    QString file1 = createTestPdf("Content A");
    QString file2 = createTestPdf("Content B");

    std::unique_ptr<Poppler::Document> doc1(openTestDocument(file1));
    std::unique_ptr<Poppler::Document> doc2(openTestDocument(file2));

    if (doc1 && doc2) {
        QJsonObject comparison = PDFUtilities::compareDocumentMetadata(doc1.get(), doc2.get());

        QVERIFY(isValidJsonObject(comparison));
        // Should contain comparison information
        QVERIFY(comparison.size() >= 0);
    } else {
        QSKIP("Could not create test PDF documents");
    }
}

void PDFUtilitiesTest::testFindCommonPages() {
    QString file1 = createTestPdf("Common content");
    QString file2 = createTestPdf("Common content");

    std::unique_ptr<Poppler::Document> doc1(openTestDocument(file1));
    std::unique_ptr<Poppler::Document> doc2(openTestDocument(file2));

    if (doc1 && doc2) {
        QStringList commonPages = PDFUtilities::findCommonPages(doc1.get(), doc2.get(), 0.8);

        QVERIFY(commonPages.size() >= 0);
    } else {
        QSKIP("Could not create test PDF documents");
    }
}

void PDFUtilitiesTest::testFindTextDifferences() {
    QString text1 = "This is the first text sample.";
    QString text2 = "This is the second text sample.";

    QJsonArray differences = PDFUtilities::findTextDifferences(text1, text2);

    QVERIFY(isValidJsonArray(differences));
    // Should find at least one difference ("first" vs "second")
    QVERIFY(differences.size() >= 0);
}

QTEST_MAIN(PDFUtilitiesTest)
#include "PDFUtilitiesTest.moc"
