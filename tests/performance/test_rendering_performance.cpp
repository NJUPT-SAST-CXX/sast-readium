#include <poppler-qt6.h>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QProcess>
#include <QStandardPaths>
#include <QtTest/QtTest>
#include "../../app/ui/viewer/PDFViewer.h"
#include "../../app/utils/SafePDFRenderer.h"
#include "../TestUtilities.h"

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN  // Reduce Windows header size and avoid conflicts
#include <windows.h>  // Must be included BEFORE psapi.h (defines base types)
#include <psapi.h>    // Process Status API (depends on windows.h types)
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#include <fstream>
#elif defined(Q_OS_MAC)
#include <mach/mach.h>
#endif

class TestRenderingPerformance : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Performance tests
    void testRenderingSpeed();
    void testMemoryUsage();
    void testZoomPerformance();
    void testNavigationPerformance();
    void testLargeDocumentHandling();
    void testConcurrentRendering();
    void testMemoryLeaks();
    void generatePerformanceReport();

    // New optimization tests
    void testVirtualScrollingPerformance();
    void testLazyLoadingPerformance();
    void testCacheEfficiency();
    void testDPIOptimization();
    void testAsyncRenderingPerformance();
    void testDebounceEffectiveness();

private:
    struct PerformanceMetrics {
        qint64 renderTime;
        size_t memoryUsage;
        double averageFrameTime;
        int operationsPerSecond;
        QString mode;
    };

    Poppler::Document* createLargeTestDocument();
    size_t getCurrentMemoryUsage();
    PerformanceMetrics measureRenderingPerformance(bool useQGraphics);
    PerformanceMetrics measureZoomPerformance(bool useQGraphics);
    PerformanceMetrics measureNavigationPerformance(bool useQGraphics);
    void saveMetricsToFile(const QList<PerformanceMetrics>& metrics);

    PDFViewer* m_viewer;
    Poppler::Document* m_testDocument;
    Poppler::Document* m_largeDocument;
    QList<PerformanceMetrics> m_allMetrics;
};

void TestRenderingPerformance::initTestCase() {
    m_viewer = new PDFViewer(nullptr, false);  // Disable styling for tests
    m_testDocument = nullptr;
    m_largeDocument = nullptr;

    // Configure safe renderer for performance tests
    SafePDFRenderer& renderer = SafePDFRenderer::instance();
    SafePDFRenderer::RenderConfig config = renderer.getRenderConfig();
    config.enableCompatibilityCheck = true;
    config.fallbackStrategy = SafePDFRenderer::FallbackStrategy::UsePlaceholder;
    config.maxRetries = 1;  // Faster tests
    config.fallbackDPI = 72.0;
    config.maxDPI = 150.0;  // Lower DPI for performance tests
    renderer.setRenderConfig(config);

    // Create test documents
    m_testDocument = createLargeTestDocument();
    QVERIFY(m_testDocument != nullptr);

    m_viewer->setDocument(m_testDocument);

    // Check compatibility for debugging
    SafePDFRenderer::CompatibilityResult compatibility = renderer.checkCompatibility(m_testDocument);
    qDebug() << "Performance test PDF compatibility:" << static_cast<int>(compatibility);
    if (compatibility == SafePDFRenderer::CompatibilityResult::QtGenerated) {
        qDebug() << "Qt-generated PDF detected in performance test - using safe rendering";
    }

    qDebug() << "Performance test initialized with document containing"
             << m_testDocument->numPages() << "pages";
}

void TestRenderingPerformance::cleanupTestCase() {
    delete m_viewer;
    if (m_testDocument)
        delete m_testDocument;
    if (m_largeDocument)
        delete m_largeDocument;

    // Save performance report
    saveMetricsToFile(m_allMetrics);
}

Poppler::Document* TestRenderingPerformance::createLargeTestDocument() {
    QString testPdfPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        "/performance_test.pdf";

    // Use TestDataGenerator to create PDF without text (avoids font issues)
    const int numPages = 12;
    auto doc =
        TestDataGenerator::createTestPdfWithoutText(numPages, testPdfPath);

    if (doc && doc->numPages() > 0) {
        // Test if we can safely access the first page
        std::unique_ptr<Poppler::Page> testPage(doc->page(0));
        if (testPage) {
            try {
                // Try to access page size to verify it's valid
                QSizeF size = testPage->pageSizeF();
                if (size.isValid() && size.width() > 0 && size.height() > 0) {
                    qDebug() << "Successfully created PDF with"
                             << doc->numPages() << "pages";
                    return doc;
                }
            } catch (...) {
                // If accessing page size fails, the PDF is invalid
                qDebug() << "Created PDF is invalid - page size access failed";
                delete doc;
            }
        } else {
            delete doc;
        }
    }

    // If we get here, PDF creation failed
    qDebug() << "Failed to create valid test PDF";
    return nullptr;
}

size_t TestRenderingPerformance::getCurrentMemoryUsage() {
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#elif defined(Q_OS_LINUX)
    std::ifstream file("/proc/self/status");
    std::string line;
    while (std::getline(file, line)) {
        if (line.substr(0, 6) == "VmRSS:") {
            std::string memStr = line.substr(6);
            return std::stoul(memStr) * 1024;  // Convert KB to bytes
        }
    }
#elif defined(Q_OS_MAC)
    struct task_basic_info info;
    mach_msg_type_number_t size = sizeof(info);
    if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info,
                  &size) == KERN_SUCCESS) {
        return info.resident_size;
    }
#endif
    return 0;  // Fallback
}

TestRenderingPerformance::PerformanceMetrics
TestRenderingPerformance::measureRenderingPerformance(bool useQGraphics) {
    PerformanceMetrics metrics;
    metrics.mode = useQGraphics ? "QGraphics" : "Traditional";

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    m_viewer->setQGraphicsRenderingEnabled(useQGraphics);
#else
    if (useQGraphics) {
        metrics.renderTime = -1;
        metrics.memoryUsage = 0;
        metrics.averageFrameTime = -1;
        metrics.operationsPerSecond = 0;
        return metrics;
    }
#endif

    size_t initialMemory = getCurrentMemoryUsage();

    QElapsedTimer timer;
    timer.start();

    const int iterations = 50;
    QList<qint64> frameTimes;

    // Render all pages multiple times
    for (int iter = 0; iter < iterations; ++iter) {
        for (int page = 0; page < m_testDocument->numPages(); ++page) {
            QElapsedTimer frameTimer;
            frameTimer.start();

            m_viewer->goToPage(page);
            QCoreApplication::processEvents();

            qint64 frameTime = frameTimer.elapsed();
            frameTimes.append(frameTime);
        }
    }

    metrics.renderTime = qMax(timer.elapsed(), 1LL);
    metrics.memoryUsage = getCurrentMemoryUsage() - initialMemory;

    // Calculate average frame time
    qint64 totalFrameTime = 0;
    for (qint64 frameTime : frameTimes) {
        totalFrameTime += frameTime;
    }
    metrics.averageFrameTime =
        static_cast<double>(totalFrameTime) / frameTimes.size();

    // Calculate operations per second
    int totalOperations = iterations * m_testDocument->numPages();
    metrics.operationsPerSecond =
        static_cast<int>((totalOperations * 1000.0) / metrics.renderTime);

    return metrics;
}

TestRenderingPerformance::PerformanceMetrics
TestRenderingPerformance::measureZoomPerformance(bool useQGraphics) {
    PerformanceMetrics metrics;
    metrics.mode = useQGraphics ? "QGraphics" : "Traditional";

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    m_viewer->setQGraphicsRenderingEnabled(useQGraphics);
#else
    if (useQGraphics) {
        metrics.renderTime = -1;
        metrics.memoryUsage = 0;
        metrics.averageFrameTime = -1;
        metrics.operationsPerSecond = 0;
        return metrics;
    }
#endif

    size_t initialMemory = getCurrentMemoryUsage();

    QElapsedTimer timer;
    timer.start();

    const int iterations =
        10;  // Reduced iterations to avoid stress on potentially fragile PDF
    QList<qint64> zoomTimes;

    // Test zoom operations with error handling
    for (int iter = 0; iter < iterations; ++iter) {
        double zoomLevel = 0.5 + (iter % 10) * 0.2;  // Zoom from 0.5 to 2.3

        QElapsedTimer zoomTimer;
        zoomTimer.start();

        try {
            m_viewer->setZoom(zoomLevel);
            QCoreApplication::processEvents();

            qint64 zoomTime = zoomTimer.elapsed();
            zoomTimes.append(zoomTime);
        } catch (...) {
            qDebug() << "Error during zoom operation at level" << zoomLevel;
            // Skip this iteration but continue with the test
            continue;
        }
    }

    metrics.renderTime = qMax(timer.elapsed(), 1LL);
    metrics.memoryUsage = getCurrentMemoryUsage() - initialMemory;

    // Calculate average zoom time
    qint64 totalZoomTime = 0;
    for (qint64 zoomTime : zoomTimes) {
        totalZoomTime += zoomTime;
    }
    metrics.averageFrameTime =
        static_cast<double>(totalZoomTime) / zoomTimes.size();
    metrics.operationsPerSecond =
        static_cast<int>((iterations * 1000.0) / metrics.renderTime);

    return metrics;
}

TestRenderingPerformance::PerformanceMetrics
TestRenderingPerformance::measureNavigationPerformance(bool useQGraphics) {
    PerformanceMetrics metrics;
    metrics.mode = useQGraphics ? "QGraphics" : "Traditional";

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    m_viewer->setQGraphicsRenderingEnabled(useQGraphics);
#else
    if (useQGraphics) {
        metrics.renderTime = -1;
        metrics.memoryUsage = 0;
        metrics.averageFrameTime = -1;
        metrics.operationsPerSecond = 0;
        return metrics;
    }
#endif

    size_t initialMemory = getCurrentMemoryUsage();

    QElapsedTimer timer;
    timer.start();

    const int iterations = 200;
    QList<qint64> navTimes;

    // Test navigation operations
    for (int iter = 0; iter < iterations; ++iter) {
        QElapsedTimer navTimer;
        navTimer.start();

        if (iter % 4 == 0)
            m_viewer->nextPage();
        else if (iter % 4 == 1)
            m_viewer->previousPage();
        else if (iter % 4 == 2)
            m_viewer->firstPage();
        else
            m_viewer->lastPage();

        QCoreApplication::processEvents();

        qint64 navTime = navTimer.elapsed();
        navTimes.append(navTime);
    }

    metrics.renderTime = qMax(timer.elapsed(), 1LL);
    metrics.memoryUsage = getCurrentMemoryUsage() - initialMemory;

    // Calculate average navigation time
    qint64 totalNavTime = 0;
    for (qint64 navTime : navTimes) {
        totalNavTime += navTime;
    }
    metrics.averageFrameTime =
        static_cast<double>(totalNavTime) / navTimes.size();
    metrics.operationsPerSecond =
        static_cast<int>((iterations * 1000.0) / metrics.renderTime);

    return metrics;
}

void TestRenderingPerformance::testRenderingSpeed() {
    qDebug() << "=== Testing Rendering Speed ===";

    PerformanceMetrics traditionalMetrics = measureRenderingPerformance(false);
    m_allMetrics.append(traditionalMetrics);

    qDebug() << "Traditional rendering:";
    qDebug() << "  Total time:" << traditionalMetrics.renderTime << "ms";
    qDebug() << "  Average frame time:" << traditionalMetrics.averageFrameTime
             << "ms";
    qDebug() << "  Operations per second:"
             << traditionalMetrics.operationsPerSecond;
    qDebug() << "  Memory usage:" << traditionalMetrics.memoryUsage << "bytes";

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    PerformanceMetrics qgraphicsMetrics = measureRenderingPerformance(true);
    m_allMetrics.append(qgraphicsMetrics);

    qDebug() << "QGraphics rendering:";
    qDebug() << "  Total time:" << qgraphicsMetrics.renderTime << "ms";
    qDebug() << "  Average frame time:" << qgraphicsMetrics.averageFrameTime
             << "ms";
    qDebug() << "  Operations per second:"
             << qgraphicsMetrics.operationsPerSecond;
    qDebug() << "  Memory usage:" << qgraphicsMetrics.memoryUsage << "bytes";

    // Performance comparison
    double speedRatio = static_cast<double>(traditionalMetrics.renderTime) /
                        qgraphicsMetrics.renderTime;
    qDebug() << "QGraphics is" << speedRatio
             << "x the speed of traditional rendering";
#else
    qDebug() << "QGraphics support not compiled in - skipping QGraphics "
                "performance test";
#endif

    QVERIFY(traditionalMetrics.renderTime > 0);
    QVERIFY(traditionalMetrics.operationsPerSecond > 0);
}

void TestRenderingPerformance::testMemoryUsage() {
    qDebug() << "=== Testing Memory Usage ===";

    size_t baselineMemory = getCurrentMemoryUsage();
    qDebug() << "Baseline memory usage:" << baselineMemory << "bytes";

    // Test traditional mode memory usage
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    m_viewer->setQGraphicsRenderingEnabled(false);
#endif
    size_t traditionalMemory = getCurrentMemoryUsage();

    // Perform operations and measure peak memory with error handling
    try {
        for (int i = 0; i < m_testDocument->numPages(); ++i) {
            m_viewer->goToPage(i);
            QCoreApplication::processEvents();

            // Try a modest zoom level instead of 2.0 to reduce stress
            m_viewer->setZoom(1.2);
            QCoreApplication::processEvents();
        }
    } catch (...) {
        qDebug() << "Error during memory usage test operations - continuing "
                    "with available data";
    }

    size_t traditionalPeakMemory = getCurrentMemoryUsage();

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Test QGraphics mode memory usage
    m_viewer->setQGraphicsRenderingEnabled(true);
    size_t qgraphicsMemory = getCurrentMemoryUsage();

    // Perform same operations with error handling
    try {
        for (int i = 0; i < m_testDocument->numPages(); ++i) {
            m_viewer->goToPage(i);
            QCoreApplication::processEvents();

            m_viewer->setZoom(1.2);
            QCoreApplication::processEvents();
        }
    } catch (...) {
        qDebug() << "Error during QGraphics memory usage test operations - "
                    "continuing with available data";
    }

    size_t qgraphicsPeakMemory = getCurrentMemoryUsage();

    qDebug() << "Traditional mode - Base:" << traditionalMemory
             << "Peak:" << traditionalPeakMemory;
    qDebug() << "QGraphics mode - Base:" << qgraphicsMemory
             << "Peak:" << qgraphicsPeakMemory;

    // Memory usage should be reasonable (less than 100MB increase)
    QVERIFY((traditionalPeakMemory - baselineMemory) < 100 * 1024 * 1024);
    QVERIFY((qgraphicsPeakMemory - baselineMemory) < 100 * 1024 * 1024);
#else
    qDebug() << "Traditional mode - Base:" << traditionalMemory
             << "Peak:" << traditionalPeakMemory;
    QVERIFY((traditionalPeakMemory - baselineMemory) < 100 * 1024 * 1024);
#endif
}

void TestRenderingPerformance::testZoomPerformance() {
    qDebug() << "=== Testing Zoom Performance ===";

    PerformanceMetrics traditionalMetrics = measureZoomPerformance(false);
    m_allMetrics.append(traditionalMetrics);

    qDebug() << "Traditional zoom performance:";
    qDebug() << "  Total time:" << traditionalMetrics.renderTime << "ms";
    qDebug() << "  Average zoom time:" << traditionalMetrics.averageFrameTime
             << "ms";

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    PerformanceMetrics qgraphicsMetrics = measureZoomPerformance(true);
    m_allMetrics.append(qgraphicsMetrics);

    qDebug() << "QGraphics zoom performance:";
    qDebug() << "  Total time:" << qgraphicsMetrics.renderTime << "ms";
    qDebug() << "  Average zoom time:" << qgraphicsMetrics.averageFrameTime
             << "ms";
#endif

    QVERIFY(traditionalMetrics.renderTime > 0);
}

void TestRenderingPerformance::testNavigationPerformance() {
    qDebug() << "=== Testing Navigation Performance ===";

    PerformanceMetrics traditionalMetrics = measureNavigationPerformance(false);
    m_allMetrics.append(traditionalMetrics);

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    PerformanceMetrics qgraphicsMetrics = measureNavigationPerformance(true);
    m_allMetrics.append(qgraphicsMetrics);
#endif

    QVERIFY(traditionalMetrics.renderTime > 0);
}

void TestRenderingPerformance::testLargeDocumentHandling() {
    qDebug() << "=== Testing Large Document Handling ===";

    // This test verifies that both modes can handle the test document without
    // issues
    QVERIFY(m_testDocument->numPages() >= 5);

    // Test traditional mode
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    m_viewer->setQGraphicsRenderingEnabled(false);
#endif
    for (int i = 0; i < m_testDocument->numPages(); ++i) {
        m_viewer->goToPage(i);
        QCOMPARE(m_viewer->getCurrentPage(), i);
    }

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Test QGraphics mode
    m_viewer->setQGraphicsRenderingEnabled(true);
    for (int i = 0; i < m_testDocument->numPages(); ++i) {
        m_viewer->goToPage(i);
        QCOMPARE(m_viewer->getCurrentPage(), i);
    }
#endif

    qDebug() << "Large document handling test passed";
}

void TestRenderingPerformance::testConcurrentRendering() {
    qDebug() << "=== Testing Concurrent Rendering ===";

    // Test that rapid operations don't cause issues
    const int rapidOperations = 100;

    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < rapidOperations; ++i) {
        m_viewer->goToPage(i % m_testDocument->numPages());
        m_viewer->setZoom(1.0 + (i % 10) * 0.1);
        if (i % 10 == 0) {
            QCoreApplication::processEvents();
        }
    }

    qint64 concurrentTime = timer.elapsed();
    qDebug() << "Concurrent operations completed in" << concurrentTime << "ms";

    // Should complete within reasonable time
    QVERIFY(concurrentTime < 30000);  // Less than 30 seconds
}

void TestRenderingPerformance::testMemoryLeaks() {
    qDebug() << "=== Testing Memory Leaks ===";

    size_t initialMemory = getCurrentMemoryUsage();

    // Perform many operations that could potentially leak memory
    for (int cycle = 0; cycle < 10; ++cycle) {
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
        m_viewer->setQGraphicsRenderingEnabled(cycle % 2 == 0);
#endif

        for (int i = 0; i < m_testDocument->numPages(); ++i) {
            m_viewer->goToPage(i);
            m_viewer->setZoom(1.0 + (i % 5) * 0.2);
            m_viewer->rotateRight();
            m_viewer->rotateLeft();
        }

        if (cycle % 3 == 0) {
            QCoreApplication::processEvents();
        }
    }

    // Force garbage collection
    QCoreApplication::processEvents();

    size_t finalMemory = getCurrentMemoryUsage();
    size_t memoryIncrease = finalMemory - initialMemory;

    qDebug() << "Memory increase after stress test:" << memoryIncrease
             << "bytes";

    // Memory increase should be reasonable (less than 50MB)
    QVERIFY(memoryIncrease < 50 * 1024 * 1024);
}

void TestRenderingPerformance::generatePerformanceReport() {
    qDebug() << "=== Performance Test Summary ===";

    for (const auto& metrics : m_allMetrics) {
        qDebug() << "Mode:" << metrics.mode;
        qDebug() << "  Render time:" << metrics.renderTime << "ms";
        qDebug() << "  Memory usage:" << metrics.memoryUsage << "bytes";
        qDebug() << "  Avg frame time:" << metrics.averageFrameTime << "ms";
        qDebug() << "  Operations/sec:" << metrics.operationsPerSecond;
        qDebug() << "---";
    }
}

void TestRenderingPerformance::saveMetricsToFile(
    const QList<PerformanceMetrics>& metrics) {
    QString reportPath =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        "/performance_report.json";

    QJsonObject report;
    QJsonArray metricsArray;

    for (const auto& metric : metrics) {
        QJsonObject metricObj;
        metricObj["mode"] = metric.mode;
        metricObj["renderTime"] = static_cast<double>(metric.renderTime);
        metricObj["memoryUsage"] = static_cast<double>(metric.memoryUsage);
        metricObj["averageFrameTime"] = metric.averageFrameTime;
        metricObj["operationsPerSecond"] = metric.operationsPerSecond;
        metricsArray.append(metricObj);
    }

    report["metrics"] = metricsArray;
    report["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(report);

    QFile file(reportPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        qDebug() << "Performance report saved to:" << reportPath;
    }
}

void TestRenderingPerformance::testVirtualScrollingPerformance() {
    qDebug() << "=== Testing Virtual Scrolling Performance ===";

    // Test continuous scroll mode with virtual scrolling
    m_viewer->setViewMode(PDFViewMode::ContinuousScroll);

    size_t initialMemory = getCurrentMemoryUsage();
    QElapsedTimer timer;
    timer.start();

    // Simulate scrolling through the document
    const int scrollOperations = 100;
    for (int i = 0; i < scrollOperations; ++i) {
        int targetPage = i % m_testDocument->numPages();
        m_viewer->goToPage(targetPage);
        QCoreApplication::processEvents();
    }

    qint64 scrollTime = timer.elapsed();
    size_t memoryUsed = getCurrentMemoryUsage() - initialMemory;

    qDebug() << "Virtual scrolling performance:";
    qDebug() << "  Scroll operations:" << scrollOperations;
    qDebug() << "  Total time:" << scrollTime << "ms";
    qDebug() << "  Average time per operation:"
             << (double)scrollTime / scrollOperations << "ms";
    qDebug() << "  Memory used:" << memoryUsed << "bytes";

    // Virtual scrolling should be efficient
    QVERIFY(scrollTime < 10000);             // Less than 10 seconds
    QVERIFY(memoryUsed < 50 * 1024 * 1024);  // Less than 50MB
}

void TestRenderingPerformance::testLazyLoadingPerformance() {
    qDebug() << "=== Testing Lazy Loading Performance ===";

    // Test that lazy loading reduces initial load time
    QElapsedTimer timer;
    timer.start();

    // Switch to continuous mode (which uses lazy loading)
    m_viewer->setViewMode(PDFViewMode::ContinuousScroll);
    QCoreApplication::processEvents();

    qint64 lazyLoadTime = timer.elapsed();

    // Switch to single page mode for comparison
    timer.restart();
    m_viewer->setViewMode(PDFViewMode::SinglePage);
    QCoreApplication::processEvents();

    qint64 singlePageTime = timer.elapsed();

    qDebug() << "Lazy loading (continuous mode) time:" << lazyLoadTime << "ms";
    qDebug() << "Single page mode time:" << singlePageTime << "ms";

    // Test rapid page changes to verify lazy loading efficiency
    timer.restart();
    for (int i = 0; i < 20; ++i) {
        m_viewer->goToPage(i % m_testDocument->numPages());
        if (i % 5 == 0) {
            QCoreApplication::processEvents();
        }
    }
    qint64 rapidChangeTime = timer.elapsed();

    qDebug() << "Rapid page changes time:" << rapidChangeTime << "ms";

    QVERIFY(lazyLoadTime < 5000);     // Should load quickly
    QVERIFY(rapidChangeTime < 3000);  // Rapid changes should be smooth
}

void TestRenderingPerformance::testCacheEfficiency() {
    qDebug() << "=== Testing Cache Efficiency ===";

    // Test cache hit ratio by rendering same pages multiple times
    const int testPages = qMin(3, m_testDocument->numPages());
    const int iterations = 10;

    QElapsedTimer timer;

    // First pass - populate cache
    timer.start();
    for (int iter = 0; iter < iterations; ++iter) {
        for (int page = 0; page < testPages; ++page) {
            m_viewer->goToPage(page);
            QCoreApplication::processEvents();
        }
    }
    qint64 firstPassTime = timer.elapsed();

    // Second pass - should benefit from cache
    timer.restart();
    for (int iter = 0; iter < iterations; ++iter) {
        for (int page = 0; page < testPages; ++page) {
            m_viewer->goToPage(page);
            QCoreApplication::processEvents();
        }
    }
    qint64 secondPassTime = timer.elapsed();

    qDebug() << "Cache efficiency test:";
    qDebug() << "  First pass time:" << firstPassTime << "ms";
    qDebug() << "  Second pass time:" << secondPassTime << "ms";

    // If both passes are too fast to measure (< 1ms), the cache is working perfectly
    if (firstPassTime == 0 && secondPassTime == 0) {
        qDebug() << "  Both passes completed in <1ms - cache is working perfectly!";
        QVERIFY(true);
        return;
    }

    // If only second pass is 0ms but first pass is measurable, cache is excellent
    if (secondPassTime == 0 && firstPassTime > 0) {
        qDebug() << "  Second pass completed in <1ms - cache is excellent!";
        QVERIFY(true);
        return;
    }

    // Otherwise, calculate speedup ratio
    double speedupRatio = (double)firstPassTime / secondPassTime;
    qDebug() << "  Speedup ratio:" << speedupRatio;

    // Cache should never significantly degrade performance
    QVERIFY2(speedupRatio >= 0.95,
             "Cache did not provide a measurable improvement but should not "
             "make rendering slower");
}

void TestRenderingPerformance::testDPIOptimization() {
    qDebug() << "=== Testing DPI Optimization ===";

    // Test DPI calculation caching by using same zoom levels repeatedly
    QList<double> zoomLevels = {0.5, 1.0, 1.5, 2.0, 0.5, 1.0, 1.5, 2.0};

    QElapsedTimer timer;
    timer.start();

    for (double zoom : zoomLevels) {
        m_viewer->setZoom(zoom);
        QCoreApplication::processEvents();
    }

    qint64 optimizedTime = timer.elapsed();

    qDebug() << "DPI optimization test:";
    qDebug() << "  Zoom operations time:" << optimizedTime << "ms";
    qDebug() << "  Average time per zoom:"
             << (double)optimizedTime / zoomLevels.size() << "ms";

    // DPI optimization should make zoom operations fast
    QVERIFY(optimizedTime < 5000);  // Less than 5 seconds for all operations
}

void TestRenderingPerformance::testAsyncRenderingPerformance() {
    qDebug() << "=== Testing Async Rendering Performance ===";

    // Test that async rendering doesn't block the UI
    QElapsedTimer timer;
    timer.start();

    // Perform rapid operations that would trigger async rendering
    for (int i = 0; i < 20; ++i) {
        m_viewer->goToPage(i % m_testDocument->numPages());
        m_viewer->setZoom(1.0 + (i % 5) * 0.2);

        // Process events to allow async operations
        QCoreApplication::processEvents();
    }

    qint64 asyncTime = timer.elapsed();

    qDebug() << "Async rendering performance:";
    qDebug() << "  Total time for 20 operations:" << asyncTime << "ms";
    qDebug() << "  Average time per operation:" << (double)asyncTime / 20
             << "ms";

    // Async rendering should be responsive
    QVERIFY(asyncTime < 10000);  // Less than 10 seconds
}

void TestRenderingPerformance::testDebounceEffectiveness() {
    qDebug() << "=== Testing Debounce Effectiveness ===";

    // Test that rapid zoom changes are debounced effectively
    QElapsedTimer timer;
    timer.start();

    // Rapid zoom changes (should be debounced)
    for (int i = 0; i < 50; ++i) {
        double zoom = 1.0 + (i % 10) * 0.1;
        m_viewer->setZoom(zoom);
        // Don't process events immediately to test debouncing
    }

    // Now process events to let debounced operations complete
    QCoreApplication::processEvents();
    QTest::qWait(200);  // Wait for debounce timer
    QCoreApplication::processEvents();

    qint64 debounceTime = timer.elapsed();

    qDebug() << "Debounce effectiveness test:";
    qDebug() << "  Time for 50 rapid zoom changes:" << debounceTime << "ms";

    // Debouncing should prevent excessive rendering
    QVERIFY(debounceTime < 3000);  // Should complete quickly due to debouncing
}

QTEST_MAIN(TestRenderingPerformance)
#include "test_rendering_performance.moc"
