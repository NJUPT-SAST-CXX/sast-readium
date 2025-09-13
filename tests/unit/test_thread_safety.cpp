#include <gtest/gtest.h>
#include <QApplication>
#include <QThread>
#include <QTimer>
#include <QEventLoop>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <memory>
#include <vector>
#include <atomic>
#include <chrono>

#include "ui/thumbnail/ThumbnailGenerator.h"
#include "model/AsyncDocumentLoader.h"
#include "ui/viewer/PDFPrerenderer.h"

class ThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple test PDF document for testing
        createTestPDF();
    }

    void TearDown() override {
        if (testPdfFile.exists()) {
            testPdfFile.remove();
        }
    }

    void createTestPDF() {
        // Create a temporary PDF file for testing
        testPdfFile.setFileTemplate(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/test_XXXXXX.pdf");
        testPdfFile.open();
        
        // Write minimal PDF content
        QByteArray pdfContent = 
            "%PDF-1.4\n"
            "1 0 obj\n"
            "<<\n"
            "/Type /Catalog\n"
            "/Pages 2 0 R\n"
            ">>\n"
            "endobj\n"
            "2 0 obj\n"
            "<<\n"
            "/Type /Pages\n"
            "/Kids [3 0 R]\n"
            "/Count 1\n"
            ">>\n"
            "endobj\n"
            "3 0 obj\n"
            "<<\n"
            "/Type /Page\n"
            "/Parent 2 0 R\n"
            "/MediaBox [0 0 612 792]\n"
            ">>\n"
            "endobj\n"
            "xref\n"
            "0 4\n"
            "0000000000 65535 f \n"
            "0000000009 00000 n \n"
            "0000000074 00000 n \n"
            "0000000120 00000 n \n"
            "trailer\n"
            "<<\n"
            "/Size 4\n"
            "/Root 1 0 R\n"
            ">>\n"
            "startxref\n"
            "199\n"
            "%%EOF\n";
        
        testPdfFile.write(pdfContent);
        testPdfFile.close();
    }

    QTemporaryFile testPdfFile;
};

// Test ThumbnailGenerator thread safety under concurrent operations
TEST_F(ThreadSafetyTest, ThumbnailGeneratorConcurrentOperations) {
    auto document = Poppler::Document::load(testPdfFile.fileName());
    ASSERT_NE(document, nullptr);
    
    ThumbnailGenerator generator;
    generator.setDocument(std::shared_ptr<Poppler::Document>(document));
    generator.start();
    
    std::atomic<int> completedThumbnails{0};
    std::atomic<int> errorCount{0};
    
    QObject::connect(&generator, &ThumbnailGenerator::thumbnailGenerated,
                     [&completedThumbnails](int, const QPixmap&) {
                         completedThumbnails++;
                     });
    
    QObject::connect(&generator, &ThumbnailGenerator::thumbnailError,
                     [&errorCount](int, const QString&) {
                         errorCount++;
                     });
    
    // Stress test: Multiple threads requesting thumbnails concurrently
    const int numThreads = 8;
    const int requestsPerThread = 10;
    std::vector<std::unique_ptr<QThread>> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        auto thread = std::make_unique<QThread>();
        QObject::connect(thread.get(), &QThread::started, [&generator, requestsPerThread]() {
            for (int j = 0; j < requestsPerThread; ++j) {
                generator.generateThumbnail(0, QSize(100, 150), 1.0, j);
                QThread::msleep(1); // Small delay to increase contention
            }
        });
        threads.push_back(std::move(thread));
    }
    
    // Start all threads
    for (auto& thread : threads) {
        thread->start();
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        ASSERT_TRUE(thread->wait(5000)) << "Thread did not complete within timeout";
    }
    
    // Wait for thumbnail generation to complete
    QEventLoop loop;
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();
    
    generator.stop();
    
    // Verify no deadlocks occurred and some thumbnails were generated
    EXPECT_GT(completedThumbnails.load(), 0) << "No thumbnails were generated";
    EXPECT_LT(errorCount.load(), numThreads * requestsPerThread) << "Too many errors occurred";
}

// Test AsyncDocumentLoader thread safety during concurrent load/cancel operations
TEST_F(ThreadSafetyTest, AsyncDocumentLoaderConcurrentLoadCancel) {
    AsyncDocumentLoader loader;
    
    std::atomic<int> loadCompletedCount{0};
    std::atomic<int> loadCancelledCount{0};
    std::atomic<int> loadFailedCount{0};
    
    QObject::connect(&loader, &AsyncDocumentLoader::documentLoaded,
                     [&loadCompletedCount](Poppler::Document*, const QString&) {
                         loadCompletedCount++;
                     });
    
    QObject::connect(&loader, &AsyncDocumentLoader::loadingCancelled,
                     [&loadCancelledCount](const QString&) {
                         loadCancelledCount++;
                     });
    
    QObject::connect(&loader, &AsyncDocumentLoader::loadingFailed,
                     [&loadFailedCount](const QString&, const QString&) {
                         loadFailedCount++;
                     });
    
    // Stress test: Rapid load/cancel cycles
    const int cycles = 20;
    for (int i = 0; i < cycles; ++i) {
        loader.loadDocument(testPdfFile.fileName());
        QThread::msleep(10); // Brief delay
        loader.cancelLoading();
        QThread::msleep(5);
    }
    
    // Wait for all operations to complete
    QEventLoop loop;
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);
    loop.exec();
    
    // Verify no deadlocks occurred
    int totalOperations = loadCompletedCount + loadCancelledCount + loadFailedCount;
    EXPECT_GT(totalOperations, 0) << "No operations completed";
    EXPECT_LE(totalOperations, cycles) << "More operations than expected";
}

// Test PDFPrerenderer thread coordination
TEST_F(ThreadSafetyTest, PDFPrerendererThreadCoordination) {
    auto document = Poppler::Document::load(testPdfFile.fileName());
    ASSERT_NE(document, nullptr);
    
    PDFPrerenderer prerenderer;
    prerenderer.setDocument(document);
    prerenderer.setMaxWorkerThreads(4);
    prerenderer.startPrerendering();
    
    std::atomic<int> prerenderedPages{0};
    
    QObject::connect(&prerenderer, &PDFPrerenderer::pagePrerendered,
                     [&prerenderedPages](int, double, int) {
                         prerenderedPages++;
                     });
    
    // Request multiple pages concurrently
    const int numPages = 1; // Our test PDF only has 1 page
    const int numRequests = 50;
    
    for (int i = 0; i < numRequests; ++i) {
        prerenderer.requestPrerender(0, 1.0, 0, i);
        QThread::msleep(1);
    }
    
    // Wait for prerendering to complete
    QEventLoop loop;
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();
    
    prerenderer.stopPrerendering();
    
    // Verify prerendering worked without deadlocks
    EXPECT_GT(prerenderedPages.load(), 0) << "No pages were prerendered";
}

// Test document switching under load
TEST_F(ThreadSafetyTest, DocumentSwitchingUnderLoad) {
    auto document1 = Poppler::Document::load(testPdfFile.fileName());
    ASSERT_NE(document1, nullptr);
    
    ThumbnailGenerator generator;
    generator.setDocument(std::shared_ptr<Poppler::Document>(document1));
    generator.start();
    
    std::atomic<bool> keepRunning{true};
    std::atomic<int> operationCount{0};
    
    // Thread that continuously requests thumbnails
    QThread requestThread;
    QObject::connect(&requestThread, &QThread::started, [&]() {
        while (keepRunning.load()) {
            generator.generateThumbnail(0, QSize(100, 150));
            operationCount++;
            QThread::msleep(10);
        }
    });
    
    requestThread.start();
    
    // Rapidly switch documents
    for (int i = 0; i < 10; ++i) {
        QThread::msleep(50);
        auto newDocument = Poppler::Document::load(testPdfFile.fileName());
        if (newDocument) {
            generator.setDocument(std::shared_ptr<Poppler::Document>(newDocument));
        }
    }
    
    keepRunning = false;
    ASSERT_TRUE(requestThread.wait(3000)) << "Request thread did not complete";
    
    generator.stop();
    
    EXPECT_GT(operationCount.load(), 0) << "No operations were performed";
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
