#include <QSignalSpy>
#include <QTest>
#include "../../app/adapters/DocumentAdapter.h"
#include "../TestUtilities.h"

class TestDocumentAdapter : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_adapter = new DocumentAdapter(); }

    void cleanup() {
        delete m_adapter;
        m_adapter = nullptr;
    }

    void testConstruction() { QVERIFY(m_adapter != nullptr); }

    void testSetDocumentController() {
        m_adapter->setDocumentController(nullptr);
    }

    void testSetPDFViewerPage() { m_adapter->setPDFViewerPage(nullptr); }

    void testOpenFileWithoutController() {
        QSignalSpy errorSpy(m_adapter, &DocumentAdapter::errorOccurred);

        m_adapter->openFile("/nonexistent/path/file.pdf");
    }

    void testCloseDocumentWithoutController() { m_adapter->closeDocument(); }

    void testSaveDocumentCopyWithoutController() {
        m_adapter->saveDocumentCopy("/tmp/test_copy.pdf");
    }

    void testPrintDocumentWithoutController() { m_adapter->printDocument(); }

    void testExportDocumentWithoutController() {
        m_adapter->exportDocument("/tmp/export.pdf", "PDF");
    }

    void testShowDocumentPropertiesWithoutController() {
        m_adapter->showDocumentProperties();
    }

    void testDocumentOpenedSignal() {
        QSignalSpy spy(m_adapter, &DocumentAdapter::documentOpened);
        QVERIFY(spy.isValid());
    }

    void testDocumentClosedSignal() {
        QSignalSpy spy(m_adapter, &DocumentAdapter::documentClosed);
        QVERIFY(spy.isValid());
    }

    void testDocumentSavedSignal() {
        QSignalSpy spy(m_adapter, &DocumentAdapter::documentSaved);
        QVERIFY(spy.isValid());
    }

    void testErrorOccurredSignal() {
        QSignalSpy spy(m_adapter, &DocumentAdapter::errorOccurred);
        QVERIFY(spy.isValid());
    }

    void testOpenFileWithEmptyPath() { m_adapter->openFile(QString()); }

    void testSaveDocumentCopyWithEmptyPath() {
        m_adapter->saveDocumentCopy(QString());
    }

    void testExportDocumentWithEmptyPath() {
        m_adapter->exportDocument(QString(), "PDF");
    }

    void testExportDocumentWithEmptyFormat() {
        m_adapter->exportDocument("/tmp/test.pdf", QString());
    }

    void testMultipleOperationsSequence() {
        m_adapter->openFile("/test/file1.pdf");
        m_adapter->openFile("/test/file2.pdf");
        m_adapter->closeDocument();
        m_adapter->openFile("/test/file3.pdf");
        m_adapter->saveDocumentCopy("/tmp/copy.pdf");
        m_adapter->closeDocument();
    }

private:
    DocumentAdapter* m_adapter = nullptr;
};

QTEST_MAIN(TestDocumentAdapter)
#include "test_document_adapter.moc"
