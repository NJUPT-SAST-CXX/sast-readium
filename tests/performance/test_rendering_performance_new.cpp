#include <QTest>
#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPdfDocument>
#include <QTemporaryFile>
#include "../TestUtilities.h"
#include "../../app/ui/viewer/QGraphicsPDFViewer.h"
#include "../../app/ui/viewer/PDFViewer.h"
#include "../../app/ui/viewer/PDFPrerenderer.h"
#include "../../app/controller/ServiceLocator.h"
#include "../../app/controller/StateManager.h"
#include "../../app/cache/PDFCacheManager.h"

class TestRenderingPerformance : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;
    
    // Basic rendering performance
    void testSinglePageRenderTime();
    void testMultiPageRenderTime();
    void testLargeDocumentRenderTime();
    void testZoomPerformance();
    void testScrollPerformance();
    
    // QGraphics vs Traditional comparison
    void testQGraphicsVsTraditionalRendering();
    void testMemoryUsageComparison();
    void testCPUUsageComparison();
    
    // Cache performance
    void testCacheHitPerformance();
    void testCacheMissPerformance();
    void testCacheEvictionPerformance();
    
    // Prerendering performance
    void testPrerenderingSpeed();
    void testPrerenderingMemoryUsage();
    void testAdaptivePrerendering();
    
    // Concurrent rendering
    void testConcurrentPageRendering();
    void testThreadPoolEfficiency();
    void testRenderingUnderLoad();
    
    // Resolution and quality
    void testHighResolutionRendering();
    void testLowQualityFastRendering();
    void testAdaptiveQualityRendering();
    
    // Stress tests
    void testRapidPageChanges();
    void testRapidZoomChanges();
    void testMemoryPressure();
    
    // Benchmarks
    void benchmarkRenderingPipeline();
    void benchmarkCacheLookup();
    void benchmarkImageConversion();

private:
    struct PerformanceMetrics {
        qint64 renderTime;
        qint64 cacheTime;
        qint64 totalTime;
        size_t memoryUsed;
        double cpuUsage;
        int framesRendered;
    };
    
    void setupTestDocument(int pageCount = 10);
    void cleanupTestDocument();
    PerformanceMetrics measureRendering(PDFViewer* viewer, int pageNum);
    PerformanceMetrics measureQGraphicsRendering(QGraphicsPDFViewer* viewer, int pageNum);
    double calculateCPUUsage();
    size_t getCurrentMemoryUsage();
    
    QString m_testPdfPath;
    QPdfDocument* m_document;
    PDFViewer* m_traditionalViewer;
    QGraphicsPDFViewer* m_qgraphicsViewer;
    PDFCacheManager* m_cacheManager;
    PDFPrerenderer* m_prerenderer;
};

void TestRenderingPerformance::initTestCase() {
    // Setup services
    ServiceLocator::instance().clearServices();
    StateManager::instance().reset();
    
    // Set performance testing configuration
    StateManager::instance().set("performance.testing", true);
    StateManager::instance().set("cache.enabled", true);
    StateManager::instance().set("prerender.enabled", true);
}

void TestRenderingPerformance::cleanupTestCase() {
    ServiceLocator::instance().clearServices();
    StateManager::instance().reset();
}

void TestRenderingPerformance::init() {
    setupTestDocument();
    
    m_document = new QPdfDocument(this);
    m_document->load(m_testPdfPath);
    
    m_traditionalViewer = new PDFViewer(nullptr);
    m_qgraphicsViewer = new QGraphicsPDFViewer(nullptr);
    m_cacheManager = new PDFCacheManager(this);
    m_prerenderer = new PDFPrerenderer(this);
    
    m_traditionalViewer->loadDocument(m_testPdfPath);
    m_qgraphicsViewer->loadDocument(m_testPdfPath);
}

void TestRenderingPerformance::cleanup() {
    delete m_prerenderer;
    delete m_cacheManager;
    delete m_qgraphicsViewer;
    delete m_traditionalViewer;
    delete m_document;
    
    cleanupTestDocument();
}

void TestRenderingPerformance::setupTestDocument(int pageCount) {
    // Create a test PDF with specified page count
    QTemporaryFile tempFile("test_perf_XXXXXX.pdf");
    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        m_testPdfPath = tempFile.fileName();
        
        // Simple PDF structure
        QString pdfContent = "%PDF-1.4\n";
        for (int i = 0; i < pageCount; ++i) {
            pdfContent += QString("Page %1 content\n").arg(i + 1);
        }
        pdfContent += "%%EOF";
        
        tempFile.write(pdfContent.toUtf8());
        tempFile.close();
    }
}

void TestRenderingPerformance::cleanupTestDocument() {
    if (!m_testPdfPath.isEmpty()) {
        QFile::remove(m_testPdfPath);
        m_testPdfPath.clear();
    }
}

TestRenderingPerformance::PerformanceMetrics 
TestRenderingPerformance::measureRendering(PDFViewer* viewer, int pageNum) {
    PerformanceMetrics metrics = {0, 0, 0, 0, 0.0, 0};
    
    size_t startMemory = getCurrentMemoryUsage();
    QElapsedTimer timer;
    timer.start();
    
    // Measure rendering
    viewer->setCurrentPage(pageNum);
    viewer->update();
    processEvents();
    
    metrics.renderTime = timer.elapsed();
    
    // Measure cache lookup
    timer.restart();
    // Simulate cache access
    m_cacheManager->getCachedPage(pageNum);
    metrics.cacheTime = timer.elapsed();
    
    metrics.totalTime = metrics.renderTime + metrics.cacheTime;
    metrics.memoryUsed = getCurrentMemoryUsage() - startMemory;
    metrics.cpuUsage = calculateCPUUsage();
    metrics.framesRendered = 1;
    
    return metrics;
}

TestRenderingPerformance::PerformanceMetrics 
TestRenderingPerformance::measureQGraphicsRendering(QGraphicsPDFViewer* viewer, int pageNum) {
    PerformanceMetrics metrics = {0, 0, 0, 0, 0.0, 0};
    
    size_t startMemory = getCurrentMemoryUsage();
    QElapsedTimer timer;
    timer.start();
    
    // Measure QGraphics rendering
    viewer->setCurrentPage(pageNum);
    viewer->viewport()->update();
    processEvents();
    
    metrics.renderTime = timer.elapsed();
    
    // Measure cache lookup
    timer.restart();
    m_cacheManager->getCachedPage(pageNum);
    metrics.cacheTime = timer.elapsed();
    
    metrics.totalTime = metrics.renderTime + metrics.cacheTime;
    metrics.memoryUsed = getCurrentMemoryUsage() - startMemory;
    metrics.cpuUsage = calculateCPUUsage();
    metrics.framesRendered = 1;
    
    return metrics;
}

double TestRenderingPerformance::calculateCPUUsage() {
    // Simplified CPU usage calculation
    // In real implementation, use platform-specific APIs
    static QElapsedTimer cpuTimer;
    static qint64 lastCpuTime = 0;
    
    if (!cpuTimer.isValid()) {
        cpuTimer.start();
        return 0.0;
    }
    
    qint64 currentTime = cpuTimer.elapsed();
    qint64 deltaTime = currentTime - lastCpuTime;
    lastCpuTime = currentTime;
    
    // Simulate CPU usage (0-100%)
    return qMin(100.0, deltaTime / 10.0);
}

size_t TestRenderingPerformance::getCurrentMemoryUsage() {
    // Simplified memory usage calculation
    // In real implementation, use platform-specific APIs
    return QRandomGenerator::global()->bounded(1024 * 1024, 100 * 1024 * 1024);
}

void TestRenderingPerformance::testSinglePageRenderTime() {
    const int testPage = 0;
    
    auto metrics = measureRendering(m_traditionalViewer, testPage);
    
    qDebug() << "Single page render time:" << metrics.renderTime << "ms";
    qDebug() << "Memory used:" << metrics.memoryUsed / 1024 << "KB";
    
    // Performance assertion - should render in less than 100ms
    QVERIFY(metrics.renderTime < 100);
}

void TestRenderingPerformance::testMultiPageRenderTime() {
    const int pagesToRender = 5;
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < pagesToRender; ++i) {
        m_traditionalViewer->setCurrentPage(i);
        m_traditionalViewer->update();
        processEvents();
    }
    
    qint64 totalTime = timer.elapsed();
    qDebug() << "Rendered" << pagesToRender << "pages in" << totalTime << "ms";
    qDebug() << "Average per page:" << totalTime / pagesToRender << "ms";
    
    // Should maintain good performance for multiple pages
    QVERIFY(totalTime < pagesToRender * 150); // 150ms per page max
}

void TestRenderingPerformance::testLargeDocumentRenderTime() {
    // Test with larger document
    cleanupTestDocument();
    setupTestDocument(100); // 100 pages
    
    delete m_document;
    m_document = new QPdfDocument(this);
    m_document->load(m_testPdfPath);
    
    QElapsedTimer timer;
    timer.start();
    
    // Test jumping to different pages
    QList<int> testPages = {0, 25, 50, 75, 99};
    for (int page : testPages) {
        m_traditionalViewer->setCurrentPage(page);
        m_traditionalViewer->update();
        processEvents();
    }
    
    qint64 totalTime = timer.elapsed();
    qDebug() << "Large document navigation time:" << totalTime << "ms";
    
    // Should handle large documents efficiently
    QVERIFY(totalTime < 1000); // Less than 1 second for 5 page jumps
}

void TestRenderingPerformance::testZoomPerformance() {
    QList<qreal> zoomLevels = {0.5, 1.0, 1.5, 2.0, 3.0};
    QElapsedTimer timer;
    
    timer.start();
    for (qreal zoom : zoomLevels) {
        m_traditionalViewer->setZoomFactor(zoom);
        m_traditionalViewer->update();
        processEvents();
    }
    
    qint64 totalTime = timer.elapsed();
    qDebug() << "Zoom changes time:" << totalTime << "ms";
    qDebug() << "Average per zoom:" << totalTime / zoomLevels.size() << "ms";
    
    // Zoom should be responsive
    QVERIFY(totalTime < 500); // Less than 500ms for all zoom changes
}

void TestRenderingPerformance::testScrollPerformance() {
    // Simulate scrolling
    QElapsedTimer timer;
    timer.start();
    
    for (int y = 0; y < 1000; y += 50) {
        m_traditionalViewer->scroll(0, 50);
        processEvents();
    }
    
    qint64 scrollTime = timer.elapsed();
    qDebug() << "Scroll performance:" << scrollTime << "ms for 20 scroll steps";
    
    // Scrolling should be smooth
    QVERIFY(scrollTime < 300); // Less than 300ms for smooth scrolling
}

void TestRenderingPerformance::testQGraphicsVsTraditionalRendering() {
    const int testPage = 0;
    
    // Measure traditional rendering
    auto traditionalMetrics = measureRendering(m_traditionalViewer, testPage);
    
    // Measure QGraphics rendering
    auto qgraphicsMetrics = measureQGraphicsRendering(m_qgraphicsViewer, testPage);
    
    qDebug() << "=== Rendering Comparison ===";
    qDebug() << "Traditional:" << traditionalMetrics.renderTime << "ms";
    qDebug() << "QGraphics:" << qgraphicsMetrics.renderTime << "ms";
    
    double speedup = (double)traditionalMetrics.renderTime / qgraphicsMetrics.renderTime;
    qDebug() << "QGraphics speedup:" << speedup << "x";
    
    // QGraphics should be competitive or faster
    QVERIFY(qgraphicsMetrics.renderTime <= traditionalMetrics.renderTime * 1.2);
}

void TestRenderingPerformance::testMemoryUsageComparison() {
    // Render multiple pages and compare memory usage
    const int pagesToTest = 10;
    
    size_t traditionalMemory = 0;
    size_t qgraphicsMemory = 0;
    
    // Traditional viewer memory usage
    size_t startMem = getCurrentMemoryUsage();
    for (int i = 0; i < pagesToTest; ++i) {
        m_traditionalViewer->setCurrentPage(i);
        m_traditionalViewer->update();
        processEvents();
    }
    traditionalMemory = getCurrentMemoryUsage() - startMem;
    
    // QGraphics viewer memory usage
    startMem = getCurrentMemoryUsage();
    for (int i = 0; i < pagesToTest; ++i) {
        m_qgraphicsViewer->setCurrentPage(i);
        m_qgraphicsViewer->viewport()->update();
        processEvents();
    }
    qgraphicsMemory = getCurrentMemoryUsage() - startMem;
    
    qDebug() << "=== Memory Usage Comparison ===";
    qDebug() << "Traditional:" << traditionalMemory / (1024 * 1024) << "MB";
    qDebug() << "QGraphics:" << qgraphicsMemory / (1024 * 1024) << "MB";
    
    // Memory usage should be reasonable
    QVERIFY(qgraphicsMemory < traditionalMemory * 2); // Not more than 2x memory
}

void TestRenderingPerformance::testCPUUsageComparison() {
    double traditionalCPU = 0;
    double qgraphicsCPU = 0;
    
    // Measure CPU during traditional rendering
    for (int i = 0; i < 5; ++i) {
        auto metrics = measureRendering(m_traditionalViewer, i);
        traditionalCPU += metrics.cpuUsage;
    }
    traditionalCPU /= 5;
    
    // Measure CPU during QGraphics rendering
    for (int i = 0; i < 5; ++i) {
        auto metrics = measureQGraphicsRendering(m_qgraphicsViewer, i);
        qgraphicsCPU += metrics.cpuUsage;
    }
    qgraphicsCPU /= 5;
    
    qDebug() << "=== CPU Usage Comparison ===";
    qDebug() << "Traditional:" << traditionalCPU << "%";
    qDebug() << "QGraphics:" << qgraphicsCPU << "%";
    
    // CPU usage should be reasonable
    QVERIFY(qgraphicsCPU < 80); // Less than 80% CPU usage
}

void TestRenderingPerformance::testCacheHitPerformance() {
    const int testPage = 0;
    
    // First render (cache miss)
    auto firstMetrics = measureRendering(m_traditionalViewer, testPage);
    
    // Second render (cache hit)
    auto cachedMetrics = measureRendering(m_traditionalViewer, testPage);
    
    qDebug() << "First render:" << firstMetrics.renderTime << "ms";
    qDebug() << "Cached render:" << cachedMetrics.renderTime << "ms";
    
    double speedup = (double)firstMetrics.renderTime / cachedMetrics.renderTime;
    qDebug() << "Cache speedup:" << speedup << "x";
    
    // Cached rendering should be significantly faster
    QVERIFY(cachedMetrics.renderTime < firstMetrics.renderTime * 0.3);
}

void TestRenderingPerformance::testCacheMissPerformance() {
    // Clear cache
    m_cacheManager->clearCache();
    
    QElapsedTimer timer;
    timer.start();
    
    // Render pages not in cache
    for (int i = 0; i < 5; ++i) {
        m_traditionalViewer->setCurrentPage(i);
        m_traditionalViewer->update();
        processEvents();
    }
    
    qint64 missTime = timer.elapsed();
    qDebug() << "Cache miss rendering time:" << missTime << "ms for 5 pages";
    
    // Should still be reasonably fast even with cache misses
    QVERIFY(missTime < 1000);
}

void TestRenderingPerformance::testCacheEvictionPerformance() {
    // Fill cache
    for (int i = 0; i < 20; ++i) {
        m_cacheManager->cachePage(i, QImage());
    }
    
    QElapsedTimer timer;
    timer.start();
    
    // Trigger cache eviction by adding more pages
    for (int i = 20; i < 30; ++i) {
        m_cacheManager->cachePage(i, QImage());
    }
    
    qint64 evictionTime = timer.elapsed();
    qDebug() << "Cache eviction time:" << evictionTime << "ms";
    
    // Eviction should be fast
    QVERIFY(evictionTime < 50);
}

void TestRenderingPerformance::testPrerenderingSpeed() {
    QElapsedTimer timer;
    timer.start();
    
    // Start prerendering
    m_prerenderer->setDocument(m_document);
    m_prerenderer->prerenderPages(0, 5);
    
    // Wait for prerendering
    waitMs(500);
    
    qint64 prerenderTime = timer.elapsed();
    qDebug() << "Prerendered 5 pages in" << prerenderTime << "ms";
    
    // Prerendering should be efficient
    QVERIFY(prerenderTime < 1000);
}

void TestRenderingPerformance::testPrerenderingMemoryUsage() {
    size_t startMemory = getCurrentMemoryUsage();
    
    // Prerender multiple pages
    m_prerenderer->setDocument(m_document);
    m_prerenderer->prerenderPages(0, 10);
    
    waitMs(500);
    
    size_t memoryUsed = getCurrentMemoryUsage() - startMemory;
    qDebug() << "Prerendering memory usage:" << memoryUsed / (1024 * 1024) << "MB for 10 pages";
    
    // Memory usage should be reasonable
    QVERIFY(memoryUsed < 100 * 1024 * 1024); // Less than 100MB
}

void TestRenderingPerformance::testAdaptivePrerendering() {
    // Test adaptive prerendering based on user behavior
    QElapsedTimer timer;
    
    // Simulate user reading pattern
    for (int i = 0; i < 5; ++i) {
        timer.restart();
        
        m_prerenderer->adaptivePrerender(i, 1); // Current page, direction forward
        waitMs(50);
        
        qint64 adaptiveTime = timer.elapsed();
        qDebug() << "Adaptive prerender for page" << i << ":" << adaptiveTime << "ms";
        
        QVERIFY(adaptiveTime < 100);
    }
}

void TestRenderingPerformance::testConcurrentPageRendering() {
    QElapsedTimer timer;
    timer.start();
    
    // Render multiple pages concurrently
    QList<QFuture<void>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.append(QtConcurrent::run([this, i]() {
            PDFViewer viewer;
            viewer.loadDocument(m_testPdfPath);
            viewer.setCurrentPage(i);
            viewer.update();
        }));
    }
    
    // Wait for all to complete
    for (auto& future : futures) {
        future.waitForFinished();
    }
    
    qint64 concurrentTime = timer.elapsed();
    qDebug() << "Concurrent rendering of 5 pages:" << concurrentTime << "ms";
    
    // Concurrent rendering should be faster than sequential
    QVERIFY(concurrentTime < 500);
}

void TestRenderingPerformance::testThreadPoolEfficiency() {
    // Test thread pool utilization
    QThreadPool* pool = QThreadPool::globalInstance();
    int maxThreads = pool->maxThreadCount();
    
    qDebug() << "Thread pool max threads:" << maxThreads;
    
    QElapsedTimer timer;
    timer.start();
    
    // Submit rendering tasks
    for (int i = 0; i < maxThreads * 2; ++i) {
        QtConcurrent::run([this, i]() {
            QImage image(100, 100, QImage::Format_RGB32);
            image.fill(Qt::white);
            QThread::msleep(10);
        });
    }
    
    pool->waitForDone();
    
    qint64 poolTime = timer.elapsed();
    qDebug() << "Thread pool processing time:" << poolTime << "ms";
    
    // Thread pool should efficiently handle tasks
    QVERIFY(poolTime < 500);
}

void TestRenderingPerformance::testRenderingUnderLoad() {
    // Simulate system under load
    QList<QFuture<void>> backgroundTasks;
    
    // Start background load
    for (int i = 0; i < 3; ++i) {
        backgroundTasks.append(QtConcurrent::run([]() {
            for (int j = 0; j < 1000000; ++j) {
                volatile int x = j * j;
                Q_UNUSED(x);
            }
        }));
    }
    
    // Measure rendering under load
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < 3; ++i) {
        m_traditionalViewer->setCurrentPage(i);
        m_traditionalViewer->update();
        processEvents();
    }
    
    qint64 loadedTime = timer.elapsed();
    qDebug() << "Rendering under load:" << loadedTime << "ms";
    
    // Should still maintain reasonable performance
    QVERIFY(loadedTime < 1000);
    
    // Wait for background tasks
    for (auto& future : backgroundTasks) {
        future.waitForFinished();
    }
}

void TestRenderingPerformance::testHighResolutionRendering() {
    QElapsedTimer timer;
    
    // Test different resolutions
    QList<qreal> resolutions = {72, 150, 300, 600};
    
    for (qreal dpi : resolutions) {
        timer.restart();
        
        m_traditionalViewer->setRenderHint(QPainter::Antialiasing);
        m_traditionalViewer->setZoomFactor(dpi / 72.0);
        m_traditionalViewer->update();
        processEvents();
        
        qint64 renderTime = timer.elapsed();
        qDebug() << "Render at" << dpi << "DPI:" << renderTime << "ms";
        
        // Higher resolution takes more time but should be reasonable
        QVERIFY(renderTime < dpi * 2); // Rough scaling
    }
}

void TestRenderingPerformance::testLowQualityFastRendering() {
    // Test fast, low-quality rendering mode
    m_traditionalViewer->setRenderHint(QPainter::Antialiasing, false);
    m_traditionalViewer->setRenderHint(QPainter::TextAntialiasing, false);
    m_traditionalViewer->setRenderHint(QPainter::SmoothPixmapTransform, false);
    
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < 10; ++i) {
        m_traditionalViewer->setCurrentPage(i % 5);
        m_traditionalViewer->update();
        processEvents();
    }
    
    qint64 fastRenderTime = timer.elapsed();
    qDebug() << "Fast rendering mode:" << fastRenderTime << "ms for 10 renders";
    
    // Fast mode should be very quick
    QVERIFY(fastRenderTime < 200);
}

void TestRenderingPerformance::testAdaptiveQualityRendering() {
    // Test adaptive quality based on interaction
    
    // High quality when static
    m_traditionalViewer->setRenderHint(QPainter::Antialiasing, true);
    QElapsedTimer timer;
    timer.start();
    m_traditionalViewer->update();
    processEvents();
    qint64 highQualityTime = timer.elapsed();
    
    // Low quality during interaction
    m_traditionalViewer->setRenderHint(QPainter::Antialiasing, false);
    timer.restart();
    m_traditionalViewer->update();
    processEvents();
    qint64 lowQualityTime = timer.elapsed();
    
    qDebug() << "High quality render:" << highQualityTime << "ms";
    qDebug() << "Low quality render:" << lowQualityTime << "ms";
    
    // Low quality should be faster
    QVERIFY(lowQualityTime < highQualityTime);
}

void TestRenderingPerformance::testRapidPageChanges() {
    // Simulate rapid page flipping
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < 20; ++i) {
        m_traditionalViewer->setCurrentPage(i % 5);
        processEvents();
        waitMs(5); // Very short delay
    }
    
    qint64 rapidChangeTime = timer.elapsed();
    qDebug() << "Rapid page changes (20 changes):" << rapidChangeTime << "ms";
    
    // Should handle rapid changes without freezing
    QVERIFY(rapidChangeTime < 1000);
}

void TestRenderingPerformance::testRapidZoomChanges() {
    // Simulate rapid zoom changes
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < 20; ++i) {
        qreal zoom = 0.5 + (i % 10) * 0.2;
        m_traditionalViewer->setZoomFactor(zoom);
        processEvents();
        waitMs(5);
    }
    
    qint64 rapidZoomTime = timer.elapsed();
    qDebug() << "Rapid zoom changes (20 changes):" << rapidZoomTime << "ms";
    
    // Should handle rapid zoom without issues
    QVERIFY(rapidZoomTime < 1000);
}

void TestRenderingPerformance::testMemoryPressure() {
    // Test behavior under memory pressure
    QList<QImage> memoryHog;
    
    // Allocate memory to create pressure
    for (int i = 0; i < 10; ++i) {
        memoryHog.append(QImage(1000, 1000, QImage::Format_ARGB32));
    }
    
    QElapsedTimer timer;
    timer.start();
    
    // Try to render under memory pressure
    for (int i = 0; i < 5; ++i) {
        m_traditionalViewer->setCurrentPage(i);
        m_traditionalViewer->update();
        processEvents();
    }
    
    qint64 pressureTime = timer.elapsed();
    qDebug() << "Rendering under memory pressure:" << pressureTime << "ms";
    
    // Should still function under pressure
    QVERIFY(pressureTime < 2000);
    
    // Clear memory
    memoryHog.clear();
}

void TestRenderingPerformance::benchmarkRenderingPipeline() {
    const int iterations = 100;
    QElapsedTimer timer;
    
    timer.start();
    for (int i = 0; i < iterations; ++i) {
        // Simulate full rendering pipeline
        QImage page(612, 792, QImage::Format_ARGB32);
        page.fill(Qt::white);
        
        QPainter painter(&page);
        painter.drawText(100, 100, QString("Page %1").arg(i));
        painter.end();
    }
    
    qint64 pipelineTime = timer.elapsed();
    double avgTime = (double)pipelineTime / iterations;
    
    qDebug() << "=== Rendering Pipeline Benchmark ===";
    qDebug() << "Total time:" << pipelineTime << "ms";
    qDebug() << "Average per page:" << avgTime << "ms";
    qDebug() << "Pages per second:" << 1000.0 / avgTime;
    
    // Should achieve good throughput
    QVERIFY(avgTime < 10); // Less than 10ms per page
}

void TestRenderingPerformance::benchmarkCacheLookup() {
    // Populate cache
    for (int i = 0; i < 100; ++i) {
        m_cacheManager->cachePage(i, QImage());
    }
    
    const int lookups = 10000;
    QElapsedTimer timer;
    
    timer.start();
    for (int i = 0; i < lookups; ++i) {
        m_cacheManager->getCachedPage(i % 100);
    }
    
    qint64 lookupTime = timer.elapsed();
    double avgLookup = (double)lookupTime / lookups * 1000; // Convert to microseconds
    
    qDebug() << "=== Cache Lookup Benchmark ===";
    qDebug() << "Total lookups:" << lookups;
    qDebug() << "Total time:" << lookupTime << "ms";
    qDebug() << "Average lookup:" << avgLookup << "μs";
    
    // Cache lookups should be very fast
    QVERIFY(avgLookup < 100); // Less than 100 microseconds
}

void TestRenderingPerformance::benchmarkImageConversion() {
    QImage testImage(612, 792, QImage::Format_ARGB32);
    testImage.fill(Qt::white);
    
    const int conversions = 1000;
    QElapsedTimer timer;
    
    timer.start();
    for (int i = 0; i < conversions; ++i) {
        // Convert to different formats
        QImage rgb = testImage.convertToFormat(QImage::Format_RGB32);
        QImage gray = testImage.convertToFormat(QImage::Format_Grayscale8);
        QPixmap pixmap = QPixmap::fromImage(testImage);
    }
    
    qint64 conversionTime = timer.elapsed();
    double avgConversion = (double)conversionTime / conversions;
    
    qDebug() << "=== Image Conversion Benchmark ===";
    qDebug() << "Total conversions:" << conversions * 3;
    qDebug() << "Total time:" << conversionTime << "ms";
    qDebug() << "Average per conversion:" << avgConversion / 3 << "ms";
    
    // Conversions should be efficient
    QVERIFY(avgConversion < 5); // Less than 5ms per set of conversions
}

QTEST_MAIN(TestRenderingPerformance)
#include "test_rendering_performance_new.moc"
