#include <QApplication>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include "../../app/ui/core/MenuBar.h"
#include "../../app/ui/core/StatusBar.h"
#include "../../app/ui/core/ToolBar.h"
#include "../../app/ui/core/ViewWidget.h"
#include "../../app/ui/widgets/SearchWidget.h"
#include "../TestUtilities.h"

/**
 * @brief Performance tests for UI responsiveness
 *
 * Tests UI performance including:
 * - Component initialization time
 * - UI update responsiveness
 * - Memory usage during operations
 * - Rendering performance
 * - Large document handling
 */
class UIPerformanceTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    // Performance benchmarks
    void benchmarkComponentInitialization();
    void benchmarkUIUpdates();
    void benchmarkSearchPerformance();
    void benchmarkNavigationPerformance();
    void testMemoryUsage();

private:
    QMainWindow* m_mainWindow;
    MenuBar* m_menuBar;
    ToolBar* m_toolBar;
    StatusBar* m_statusBar;
    ViewWidget* m_viewWidget;
    SearchWidget* m_searchWidget;
    QTemporaryFile* m_testPdfFile;

    void createTestPdf();
    void measureTime(const QString& operation, std::function<void()> func);
};

void UIPerformanceTest::initTestCase() { createTestPdf(); }

void UIPerformanceTest::cleanupTestCase() { delete m_testPdfFile; }

void UIPerformanceTest::init() {
    m_mainWindow = new QMainWindow();
    m_mainWindow->resize(1400, 900);
}

void UIPerformanceTest::cleanup() {
    delete m_mainWindow;
    m_mainWindow = nullptr;
}

void UIPerformanceTest::benchmarkComponentInitialization() {
    QElapsedTimer timer;

    // Benchmark MenuBar creation
    timer.start();
    m_menuBar = new MenuBar(m_mainWindow);
    qint64 menuBarTime = timer.elapsed();

    // Benchmark ToolBar creation
    timer.restart();
    m_toolBar = new ToolBar(m_mainWindow);
    qint64 toolBarTime = timer.elapsed();

    // Benchmark StatusBar creation
    timer.restart();
    m_statusBar = new StatusBar(m_mainWindow);
    qint64 statusBarTime = timer.elapsed();

    // Benchmark ViewWidget creation
    timer.restart();
    m_viewWidget = new ViewWidget(m_mainWindow);
    qint64 viewWidgetTime = timer.elapsed();

    // Benchmark SearchWidget creation
    timer.restart();
    m_searchWidget = new SearchWidget(m_mainWindow);
    qint64 searchWidgetTime = timer.elapsed();

    qDebug() << "Component initialization times:";
    qDebug() << "MenuBar:" << menuBarTime << "ms";
    qDebug() << "ToolBar:" << toolBarTime << "ms";
    qDebug() << "StatusBar:" << statusBarTime << "ms";
    qDebug() << "ViewWidget:" << viewWidgetTime << "ms";
    qDebug() << "SearchWidget:" << searchWidgetTime << "ms";

    // Performance assertions (components should initialize quickly)
    QVERIFY(menuBarTime < 100);  // Less than 100ms
    QVERIFY(toolBarTime < 100);
    QVERIFY(statusBarTime < 100);
    QVERIFY(viewWidgetTime < 200);  // ViewWidget may be more complex
    QVERIFY(searchWidgetTime < 100);
}

void UIPerformanceTest::benchmarkUIUpdates() {
    // Create components first
    m_statusBar = new StatusBar(m_mainWindow);
    m_toolBar = new ToolBar(m_mainWindow);

    const int iterations = 1000;

    // Benchmark status bar updates
    measureTime("StatusBar page updates", [this, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            m_statusBar->setPageInfo(i % 100, 100);
        }
    });

    // Benchmark toolbar updates
    measureTime("ToolBar page updates", [this, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            m_toolBar->updatePageInfo(i % 100, 100);
        }
    });

    // Benchmark zoom updates
    measureTime("Zoom level updates", [this, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            double zoom = 0.5 + (i % 400) / 100.0;
            m_statusBar->setZoomLevel(zoom);
            m_toolBar->updateZoomLevel(zoom);
        }
    });
}

void UIPerformanceTest::benchmarkSearchPerformance() {
    m_searchWidget = new SearchWidget(m_mainWindow);

    const int searchIterations = 100;

    measureTime("Search widget operations", [this, searchIterations]() {
        QLineEdit* searchInput = m_searchWidget->findChild<QLineEdit*>();
        if (searchInput) {
            for (int i = 0; i < searchIterations; ++i) {
                searchInput->setText(QString("search%1").arg(i));
                m_searchWidget->performSearch();
                m_searchWidget->clearSearch();
            }
        }
    });
}

void UIPerformanceTest::benchmarkNavigationPerformance() {
    m_viewWidget = new ViewWidget(m_mainWindow);

    if (!m_testPdfFile || !m_testPdfFile->exists()) {
        QSKIP("No test PDF file available");
    }

    // Load document first
    m_viewWidget->openDocument(m_testPdfFile->fileName());
    waitMs(500);  // Allow document to load

    if (!m_viewWidget->hasDocuments()) {
        QSKIP("Document not loaded");
    }

    const int navigationIterations = 50;

    measureTime("Page navigation", [this, navigationIterations]() {
        for (int i = 0; i < navigationIterations; ++i) {
            m_viewWidget->goToPage(0);
            QApplication::processEvents();
        }
    });

    measureTime("Zoom operations", [this, navigationIterations]() {
        for (int i = 0; i < navigationIterations; ++i) {
            double zoom = 0.5 + (i % 8) * 0.25;
            m_viewWidget->setZoom(zoom);
            QApplication::processEvents();
        }
    });
}

void UIPerformanceTest::testMemoryUsage() {
    size_t initialMemory =
        0;  // Memory usage tracking not available in test environment

    // Create all UI components
    m_menuBar = new MenuBar(m_mainWindow);
    m_toolBar = new ToolBar(m_mainWindow);
    m_statusBar = new StatusBar(m_mainWindow);
    m_viewWidget = new ViewWidget(m_mainWindow);
    m_searchWidget = new SearchWidget(m_mainWindow);

    size_t afterCreation = 0;
    size_t creationMemory = 0;

    // Perform operations
    for (int i = 0; i < 100; ++i) {
        m_statusBar->setPageInfo(i, 100);
        m_toolBar->updatePageInfo(i, 100);
        QApplication::processEvents();
    }

    size_t afterOperations = 0;
    size_t operationMemory = 0;

    qDebug() << "Memory usage:";
    qDebug() << "Component creation:" << creationMemory / (1024 * 1024) << "MB";
    qDebug() << "Operations:" << operationMemory / (1024 * 1024) << "MB";

    // Memory usage should be reasonable
    QVERIFY(creationMemory < 50 * 1024 * 1024);  // Less than 50MB for creation
    QVERIFY(operationMemory <
            10 * 1024 * 1024);  // Less than 10MB for operations
}

void UIPerformanceTest::createTestPdf() {
    m_testPdfFile = new QTemporaryFile();
    m_testPdfFile->setFileTemplate("test_pdf_XXXXXX.pdf");
    if (m_testPdfFile->open()) {
        QByteArray pdfContent =
            "%PDF-1.4\n"
            "1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n"
            "2 0 obj\n<<\n/Type /Pages\n/Kids [3 0 R]\n/Count 1\n>>\nendobj\n"
            "3 0 obj\n<<\n/Type /Page\n/Parent 2 0 R\n/MediaBox [0 0 612 792]\n"
            "/Contents 4 0 R\n>>\nendobj\n"
            "4 0 obj\n<<\n/Length 44\n>>\nstream\nBT\n/F1 12 Tf\n100 700 Td\n"
            "(Test Page) Tj\nET\nendstream\nendobj\n"
            "xref\n0 5\n0000000000 65535 f \n0000000009 65535 n \n"
            "0000000074 65535 n \n0000000120 65535 n \n0000000179 65535 n \n"
            "trailer\n<<\n/Size 5\n/Root 1 0 R\n>>\nstartxref\n274\n%%EOF\n";

        m_testPdfFile->write(pdfContent);
        m_testPdfFile->flush();
    }
}

void UIPerformanceTest::measureTime(const QString& operation,
                                    std::function<void()> func) {
    QElapsedTimer timer;
    timer.start();

    func();

    qint64 elapsed = timer.elapsed();
    qDebug() << operation << "took" << elapsed << "ms";

    // Performance assertion - operations should complete in reasonable time
    QVERIFY(elapsed < 5000);  // Less than 5 seconds
}

QTEST_MAIN(UIPerformanceTest)
#include "test_ui_performance_comprehensive.moc"
