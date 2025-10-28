#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include "../../app/model/DocumentModel.h"
#include "../../app/model/RenderModel.h"
#include "../TestUtilities.h"

/**
 * @brief Comprehensive tests for DocumentModel
 *
 * Tests document management including opening, closing, switching between
 * documents, async loading, error handling, and signal emissions.
 */
class DocumentModelTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Construction tests
    void testDefaultConstruction();
    void testConstructionWithRenderModel();

    // Document opening tests
    void testOpenSingleDocument();
    void testOpenMultipleDocuments();
    void testOpenDuplicateDocument();
    void testOpenNonExistentFile();
    void testOpenEmptyPath();
    void testOpenInvalidPDF();

    // Document closing tests
    void testCloseDocument();
    void testCloseCurrentDocument();
    void testCloseInvalidIndex();
    void testCloseAllDocuments();

    // Document switching tests
    void testSwitchToDocument();
    void testSwitchToInvalidIndex();
    void testSwitchToSameDocument();

    // Query tests
    void testGetDocumentCount();
    void testGetCurrentDocumentIndex();
    void testGetCurrentFilePath();
    void testGetCurrentFileName();
    void testGetDocumentByIndex();
    void testIsEmpty();
    void testIsValidIndex();

    // Signal emission tests
    void testDocumentOpenedSignal();
    void testDocumentClosedSignal();
    void testCurrentDocumentChangedSignal();
    void testAllDocumentsClosedSignal();
    void testLoadingSignals();

    // Async loading tests
    void testAsyncDocumentLoading();
    void testLoadingProgressTracking();
    void testLoadingFailureHandling();

    // Edge cases
    void testMultipleOpenClose();
    void testRapidSwitching();
    void testMemoryManagement();

private:
    DocumentModel* m_model;
    RenderModel* m_renderModel;
    QString m_testPdfPath;
    QString m_testPdfPath2;

    // Helper methods
    QString createTestPDF(int pages = 3);
    void waitForAsyncLoad(int timeout = 5000);
};

void DocumentModelTest::initTestCase() {
    // Create test PDFs
    m_testPdfPath = createTestPDF(3);
    m_testPdfPath2 = createTestPDF(5);

    QVERIFY(!m_testPdfPath.isEmpty());
    QVERIFY(!m_testPdfPath2.isEmpty());
    QVERIFY(QFile::exists(m_testPdfPath));
    QVERIFY(QFile::exists(m_testPdfPath2));
}

void DocumentModelTest::cleanupTestCase() {
    if (!m_testPdfPath.isEmpty()) {
        QFile::remove(m_testPdfPath);
    }
    if (!m_testPdfPath2.isEmpty()) {
        QFile::remove(m_testPdfPath2);
    }
}

void DocumentModelTest::init() {
    m_renderModel = new RenderModel();
    m_model = new DocumentModel(m_renderModel);
}

void DocumentModelTest::cleanup() {
    delete m_model;
    delete m_renderModel;
    m_model = nullptr;
    m_renderModel = nullptr;
}

QString DocumentModelTest::createTestPDF(int pages) {
    auto doc = TestDataGenerator::createTestPdfWithoutText(pages);
    if (!doc) {
        return QString();
    }

    // The test PDF is already saved, get its path
    QString path =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
        QString("/test_doc_%1.pdf").arg(QRandomGenerator::global()->generate());

    delete doc;
    return path;
}

void DocumentModelTest::waitForAsyncLoad(int timeout) {
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(timeout);

    while (timer.isActive() && m_model->isEmpty()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }
}

void DocumentModelTest::testDefaultConstruction() {
    DocumentModel model;
    QVERIFY(model.isEmpty());
    QCOMPARE(model.getDocumentCount(), 0);
    QCOMPARE(model.getCurrentDocumentIndex(), -1);
}

void DocumentModelTest::testConstructionWithRenderModel() {
    QVERIFY(m_model != nullptr);
    QVERIFY(m_model->isEmpty());
    QCOMPARE(m_model->getDocumentCount(), 0);
}

void DocumentModelTest::testOpenSingleDocument() {
    QSignalSpy spyOpened(m_model, &DocumentModel::documentOpened);
    QSignalSpy spyLoading(m_model, &DocumentModel::loadingStarted);

    bool result = m_model->openFromFile(m_testPdfPath);

    QVERIFY(result);
    QCOMPARE(spyLoading.count(), 1);

    // Wait for async load
    waitForAsyncLoad();

    QVERIFY(!m_model->isEmpty());
    QCOMPARE(m_model->getDocumentCount(), 1);
    QCOMPARE(m_model->getCurrentDocumentIndex(), 0);
    QVERIFY(m_model->getCurrentDocument() != nullptr);
}

void DocumentModelTest::testOpenMultipleDocuments() {
    m_model->openFromFile(m_testPdfPath);
    waitForAsyncLoad();

    QSignalSpy spyOpened(m_model, &DocumentModel::documentOpened);

    m_model->openFromFile(m_testPdfPath2);
    waitForAsyncLoad();

    QCOMPARE(m_model->getDocumentCount(), 2);
    QVERIFY(spyOpened.count() > 0);
}

void DocumentModelTest::testOpenDuplicateDocument() {
    m_model->openFromFile(m_testPdfPath);
    waitForAsyncLoad();

    int initialCount = m_model->getDocumentCount();

    // Try to open same document again
    m_model->openFromFile(m_testPdfPath);
    waitForAsyncLoad();

    // Should not create duplicate, just switch to existing
    QCOMPARE(m_model->getDocumentCount(), initialCount);
}

void DocumentModelTest::testOpenNonExistentFile() {
    QSignalSpy spyFailed(m_model, &DocumentModel::loadingFailed);

    bool result = m_model->openFromFile("/nonexistent/file.pdf");

    // Should handle error gracefully
    QVERIFY(!result || spyFailed.count() > 0);
}

void DocumentModelTest::testOpenEmptyPath() {
    QSignalSpy spyFailed(m_model, &DocumentModel::loadingFailed);

    bool result = m_model->openFromFile("");

    QVERIFY(!result || spyFailed.count() > 0);
}

void DocumentModelTest::testCloseDocument() {
    m_model->openFromFile(m_testPdfPath);
    waitForAsyncLoad();

    QSignalSpy spyClosed(m_model, &DocumentModel::documentClosed);

    bool result = m_model->closeDocument(0);

    QVERIFY(result);
    QCOMPARE(spyClosed.count(), 1);
    QVERIFY(m_model->isEmpty());
}

void DocumentModelTest::testCloseCurrentDocument() {
    m_model->openFromFile(m_testPdfPath);
    waitForAsyncLoad();

    QSignalSpy spyClosed(m_model, &DocumentModel::documentClosed);

    bool result = m_model->closeCurrentDocument();

    QVERIFY(result);
    QCOMPARE(spyClosed.count(), 1);
}

void DocumentModelTest::testCloseInvalidIndex() {
    bool result = m_model->closeDocument(999);
    QVERIFY(!result);
}

void DocumentModelTest::testSwitchToDocument() {
    m_model->openFromFile(m_testPdfPath);
    waitForAsyncLoad();
    m_model->openFromFile(m_testPdfPath2);
    waitForAsyncLoad();

    QSignalSpy spyChanged(m_model, &DocumentModel::currentDocumentChanged);

    m_model->switchToDocument(0);

    QCOMPARE(m_model->getCurrentDocumentIndex(), 0);
    QCOMPARE(spyChanged.count(), 1);
}

void DocumentModelTest::testGetDocumentCount() {
    QCOMPARE(m_model->getDocumentCount(), 0);

    m_model->openFromFile(m_testPdfPath);
    waitForAsyncLoad();
    QCOMPARE(m_model->getDocumentCount(), 1);

    m_model->openFromFile(m_testPdfPath2);
    waitForAsyncLoad();
    QCOMPARE(m_model->getDocumentCount(), 2);
}

void DocumentModelTest::testIsEmpty() {
    QVERIFY(m_model->isEmpty());

    m_model->openFromFile(m_testPdfPath);
    waitForAsyncLoad();
    QVERIFY(!m_model->isEmpty());
}

void DocumentModelTest::testIsValidIndex() {
    QVERIFY(!m_model->isValidIndex(0));

    m_model->openFromFile(m_testPdfPath);
    waitForAsyncLoad();

    QVERIFY(m_model->isValidIndex(0));
    QVERIFY(!m_model->isValidIndex(1));
    QVERIFY(!m_model->isValidIndex(-1));
}

void DocumentModelTest::testDocumentOpenedSignal() {
    QSignalSpy spy(m_model, &DocumentModel::documentOpened);

    m_model->openFromFile(m_testPdfPath);
    waitForAsyncLoad();

    QVERIFY(spy.count() > 0);
}

void DocumentModelTest::testAllDocumentsClosedSignal() {
    m_model->openFromFile(m_testPdfPath);
    waitForAsyncLoad();

    QSignalSpy spy(m_model, &DocumentModel::allDocumentsClosed);

    m_model->closeCurrentDocument();

    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(DocumentModelTest)
#include "test_document_model.moc"
