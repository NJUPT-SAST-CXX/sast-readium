#include <poppler-qt6.h>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QtTest/QtTest>
#include "../../app/ui/viewer/PDFViewer.h"

#ifdef Q_OS_WIN
#include <psapi.h>
#include <windows.h>
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

    // Create test documents
    m_testDocument = createLargeTestDocument();
    QVERIFY(m_testDocument != nullptr);

    m_viewer->setDocument(m_testDocument);

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

    QFile file(testPdfPath);
    if (file.open(QIODevice::WriteOnly)) {
        // Create a PDF with multiple pages for performance testing
        QByteArray pdfContent = "%PDF-1.4\n";

        // Catalog
        pdfContent += "1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n";

        // Pages object
        const int numPages = 10;  // Create 10 pages for testing
        pdfContent += "2 0 obj\n<<\n/Type /Pages\n/Kids [";
        for (int i = 0; i < numPages; ++i) {
            pdfContent += QString("%1 0 R ").arg(3 + i * 2).toUtf8();
        }
        pdfContent +=
            QString("]\n/Count %1\n>>\nendobj\n").arg(numPages).toUtf8();

        // Create pages and content
        int objNum = 3;
        for (int page = 0; page < numPages; ++page) {
            // Page object
            pdfContent +=
                QString(
                    "%1 0 obj\n<<\n/Type /Page\n/Parent 2 0 R\n/MediaBox [0 0 "
                    "612 792]\n/Contents %2 0 R\n>>\nendobj\n")
                    .arg(objNum)
                    .arg(objNum + 1)
                    .toUtf8();

            // Content stream with more complex content for performance testing
            QString content = QString("BT\n/F1 12 Tf\n");
            for (int line = 0; line < 20; ++line) {
                content += QString(
                               "50 %1 Td\n(Page %2 Line %3 - Performance Test "
                               "Content) Tj\n")
                               .arg(750 - line * 30)
                               .arg(page + 1)
                               .arg(line + 1);
            }
            content += "ET\n";

            pdfContent += QString(
                              "%1 0 obj\n<<\n/Length "
                              "%2\n>>\nstream\n%3endstream\nendobj\n")
                              .arg(objNum + 1)
                              .arg(content.length())
                              .arg(content)
                              .toUtf8();

            objNum += 2;
        }

        // Add xref and trailer
        int xrefPos = pdfContent.length();
        pdfContent += QString("xref\n0 %1\n").arg(objNum).toUtf8();
        pdfContent += "0000000000 65535 f \n";

        // Calculate positions (simplified)
        for (int i = 1; i < objNum; ++i) {
            pdfContent += QString("%1 00000 n \n")
                              .arg(i * 100, 10, 10, QChar('0'))
                              .toUtf8();
        }

        pdfContent += QString(
                          "trailer\n<<\n/Size %1\n/Root 1 0 "
                          "R\n>>\nstartxref\n%2\n%%EOF\n")
                          .arg(objNum)
                          .arg(xrefPos)
                          .toUtf8();

        file.write(pdfContent);
        file.close();

        return Poppler::Document::load(testPdfPath).release();
    }

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

    metrics.renderTime = timer.elapsed();
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

    const int iterations = 100;
    QList<qint64> zoomTimes;

    // Test zoom operations
    for (int iter = 0; iter < iterations; ++iter) {
        double zoomLevel = 0.5 + (iter % 10) * 0.2;  // Zoom from 0.5 to 2.3

        QElapsedTimer zoomTimer;
        zoomTimer.start();

        m_viewer->setZoom(zoomLevel);
        QCoreApplication::processEvents();

        qint64 zoomTime = zoomTimer.elapsed();
        zoomTimes.append(zoomTime);
    }

    metrics.renderTime = timer.elapsed();
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

    metrics.renderTime = timer.elapsed();
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

    // Perform operations and measure peak memory
    for (int i = 0; i < m_testDocument->numPages(); ++i) {
        m_viewer->goToPage(i);
        m_viewer->setZoom(2.0);
        QCoreApplication::processEvents();
    }

    size_t traditionalPeakMemory = getCurrentMemoryUsage();

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    // Test QGraphics mode memory usage
    m_viewer->setQGraphicsRenderingEnabled(true);
    size_t qgraphicsMemory = getCurrentMemoryUsage();

    // Perform same operations
    for (int i = 0; i < m_testDocument->numPages(); ++i) {
        m_viewer->goToPage(i);
        m_viewer->setZoom(2.0);
        QCoreApplication::processEvents();
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
    QVERIFY(m_testDocument->numPages() > 5);

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

QTEST_MAIN(TestRenderingPerformance)
#include "test_rendering_performance.moc"
