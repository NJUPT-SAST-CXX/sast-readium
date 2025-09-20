#include <QTest>
#include <QSignalSpy>
#include <QMainWindow>
#include <QStackedWidget>
#include <QApplication>
#include <QTimer>
#include "ControllerTestMocks.h"

class ControllerMockObjectTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override {
        // Ensure QApplication exists for widget testing
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            new QApplication(argc, argv);
        }
    }

    void cleanupTestCase() override {
        // Cleanup will be handled by Qt
    }

    void init() override {
        // Create fresh mock objects for each test
        m_mockMainWindow = new MockMainWindow();
    }

    void cleanup() override {
        delete m_mockMainWindow;
        m_mockMainWindow = nullptr;
    }
    
    // Basic mock object tests
    void testMockMainWindowCreation() {
        QVERIFY(m_mockMainWindow != nullptr);
        QCOMPARE(m_mockMainWindow->logicalDpiX(), 96);
        QCOMPARE(m_mockMainWindow->logicalDpiY(), 96);

        // Test DPI modification
        m_mockMainWindow->setDpi(120, 120);
        QCOMPARE(m_mockMainWindow->logicalDpiX(), 120);
        QCOMPARE(m_mockMainWindow->logicalDpiY(), 120);
    }

    void testMockObjectFactory() {
        // Test factory methods
        MockMainWindow* window = MockObjectFactory::createMockMainWindow();
        QVERIFY(window != nullptr);
        delete window;

        MockDocumentModel* docModel = MockObjectFactory::createMockDocumentModel(this);
        QVERIFY(docModel != nullptr);
        QVERIFY(docModel->isEmpty());

        MockPageModel* pageModel = MockObjectFactory::createMockPageModel(20, this);
        QVERIFY(pageModel != nullptr);
        QCOMPARE(pageModel->totalPages(), 20);
        QCOMPARE(pageModel->currentPage(), 1);
    }
    
    // Mock document model tests
    void testMockDocumentModel() {
        MockDocumentModel* docModel = new MockDocumentModel(this);

        // Test initial state
        QVERIFY(docModel->isEmpty());
        QCOMPARE(docModel->getDocumentCount(), 0);
        QCOMPARE(docModel->getCurrentDocumentIndex(), -1);

        QSignalSpy documentOpenedSpy(docModel, &MockDocumentModel::documentOpened);

        // Test opening a document
        bool result = docModel->openFromFile("test.pdf");
        QVERIFY(result);
        QVERIFY(!docModel->isEmpty());
        QCOMPARE(docModel->getDocumentCount(), 1);
        QCOMPARE(docModel->getCurrentDocumentIndex(), 0);
        QCOMPARE(documentOpenedSpy.count(), 1);

        // Test opening invalid document
        bool invalidResult = docModel->openFromFile("test.txt");
        QVERIFY(!invalidResult);

        // Test closing document
        bool closeResult = docModel->closeDocument(0);
        QVERIFY(closeResult);
        QVERIFY(docModel->isEmpty());
    }

    void testMockPageModel() {
        MockPageModel* pageModel = new MockPageModel(this);

        // Test initial state
        QCOMPARE(pageModel->currentPage(), 1);
        QCOMPARE(pageModel->totalPages(), 1);

        QSignalSpy pageUpdateSpy(pageModel, &MockPageModel::pageUpdate);

        // Test setting total pages
        pageModel->setTotalPages(10);
        QCOMPARE(pageModel->totalPages(), 10);
        QCOMPARE(pageUpdateSpy.count(), 1);

        // Test setting current page
        pageModel->setCurrentPage(5);
        QCOMPARE(pageModel->currentPage(), 5);
        QCOMPARE(pageUpdateSpy.count(), 2);

        // Test invalid page
        pageModel->setCurrentPage(15); // Should not change
        QCOMPARE(pageModel->currentPage(), 5);

        // Test reset
        pageModel->reset();
        QCOMPARE(pageModel->currentPage(), 1);
        QCOMPARE(pageModel->totalPages(), 1);
    }
    
    // Mock utilities tests
    void testControllerTestUtils() {
        // Test PDF path validation
        QVERIFY(ControllerTestUtils::isValidPdfPath("test.pdf"));
        QVERIFY(ControllerTestUtils::isValidPdfPath("document.PDF"));
        QVERIFY(!ControllerTestUtils::isValidPdfPath("test.txt"));
        QVERIFY(!ControllerTestUtils::isValidPdfPath(""));

        // Test page number validation
        QVERIFY(ControllerTestUtils::isValidPageNumber(1, 10));
        QVERIFY(ControllerTestUtils::isValidPageNumber(10, 10));
        QVERIFY(!ControllerTestUtils::isValidPageNumber(0, 10));
        QVERIFY(!ControllerTestUtils::isValidPageNumber(11, 10));
        QVERIFY(!ControllerTestUtils::isValidPageNumber(1, 0));
    }

    void testMockRecentFilesManager() {
        MockRecentFilesManager* manager = new MockRecentFilesManager(this);

        QSignalSpy addedSpy(manager, &MockRecentFilesManager::recentFileAdded);
        QSignalSpy changedSpy(manager, &MockRecentFilesManager::recentFilesChanged);

        // Test adding files
        manager->addRecentFile("file1.pdf");
        manager->addRecentFile("file2.pdf");

        QCOMPARE(manager->recentFiles().size(), 2);
        QCOMPARE(addedSpy.count(), 2);
        QCOMPARE(changedSpy.count(), 2);

        // Test clearing files
        QSignalSpy clearedSpy(manager, &MockRecentFilesManager::recentFilesCleared);
        manager->clearRecentFiles();

        QCOMPARE(manager->recentFiles().size(), 0);
        QCOMPARE(clearedSpy.count(), 1);
    }

    void testMockStyleManager() {
        MockStyleManager* styleManager = new MockStyleManager(this);

        // Test initial state
        QCOMPARE(styleManager->currentTheme(), QString("light"));

        QSignalSpy themeChangedSpy(styleManager, &MockStyleManager::themeChanged);

        // Test theme change
        styleManager->setTheme("dark");
        QCOMPARE(styleManager->currentTheme(), QString("dark"));
        QCOMPARE(themeChangedSpy.count(), 1);

        // Test invalid theme (should not change)
        styleManager->setTheme("invalid");
        QCOMPARE(styleManager->currentTheme(), QString("dark"));
        QCOMPARE(themeChangedSpy.count(), 1);
    }

private:
    MockMainWindow* m_mockMainWindow = nullptr;
};

QTEST_MAIN(ControllerMockObjectTest)
#include "application_controller_test.moc"
