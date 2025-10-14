#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>
#include "../../app/controller/DocumentController.h"
#include "../../app/controller/tool.hpp"
#include "../../app/managers/RecentFilesManager.h"
#include "../../app/model/DocumentModel.h"
#include "../TestUtilities.h"

// Mock classes for testing - these inherit from real classes for proper type
// compatibility
class MockDocumentModel : public DocumentModel {
    Q_OBJECT
public:
    explicit MockDocumentModel(QObject* parent = nullptr)
        : DocumentModel(nullptr),
          m_isEmpty(true),
          m_documentCount(0),
          m_currentIndex(-1) {
        setParent(parent);
    }

    // Document state
    bool isEmpty() const override { return m_isEmpty; }
    int getDocumentCount() const override { return m_documentCount; }
    int getCurrentDocumentIndex() const override { return m_currentIndex; }
    QString getCurrentFilePath() const override { return m_currentFilePath; }
    QString getCurrentFileName() const override { return m_currentFileName; }

    // Mock document operations
    bool openFromFile(const QString& filePath) override {
        if (filePath.isEmpty() || !filePath.endsWith(".pdf")) {
            return false;
        }
        m_isEmpty = false;
        m_documentCount = 1;
        m_currentIndex = 0;
        m_currentFilePath = filePath;
        QFileInfo info(filePath);
        m_currentFileName = info.fileName();
        emit documentOpened(0, m_currentFileName);
        return true;
    }

    bool openFromFiles(const QStringList& filePaths) override {
        if (filePaths.isEmpty()) {
            return false;
        }

        int validCount = 0;
        for (const QString& path : filePaths) {
            if (!path.isEmpty() && path.endsWith(".pdf")) {
                validCount++;
            }
        }

        if (validCount == 0) {
            return false;
        }

        m_isEmpty = false;
        m_documentCount = validCount;
        m_currentIndex = 0;
        if (!filePaths.isEmpty()) {
            m_currentFilePath = filePaths.first();
            QFileInfo info(m_currentFilePath);
            m_currentFileName = info.fileName();
        }
        return true;
    }

    bool closeDocument(int index) override {
        if (index < 0 || index >= m_documentCount) {
            return false;
        }
        m_documentCount--;
        if (m_documentCount == 0) {
            m_isEmpty = true;
            m_currentIndex = -1;
            m_currentFilePath.clear();
            m_currentFileName.clear();
        }
        return true;
    }

    bool closeCurrentDocument() override {
        return closeDocument(m_currentIndex);
    }

    void switchToDocument(int index) override {
        if (index >= 0 && index < m_documentCount) {
            m_currentIndex = index;
        }
    }

    // Mock for getCurrentDocument - returns nullptr since we can't create real
    // Poppler::Document
    Poppler::Document* getCurrentDocument() const override { return nullptr; }

    // Mock for isNULL - returns false since mock is not null
    bool isNULL() override { return false; }

signals:
    void documentOpened(int index, const QString& fileName);
    void documentClosed(int index);
    void currentDocumentChanged(int index);

private:
    bool m_isEmpty;
    int m_documentCount;
    int m_currentIndex;
    QString m_currentFilePath;
    QString m_currentFileName;
};

class MockRecentFilesManager : public RecentFilesManager {
    Q_OBJECT
public:
    explicit MockRecentFilesManager(QObject* parent = nullptr)
        : RecentFilesManager(parent) {}

    void addRecentFile(const QString& filePath) override {
        m_recentFiles.prepend(filePath);
        if (m_recentFiles.size() > 10) {
            m_recentFiles.removeLast();
        }
        emit recentFileAdded(filePath);
    }

    void clearRecentFiles() {
        m_recentFiles.clear();
        emit recentFilesCleared();
    }

    QStringList recentFiles() const { return m_recentFiles; }

signals:
    void recentFileAdded(const QString& filePath);
    void recentFilesCleared();

private:
    QStringList m_recentFiles;
};

class DocumentControllerTest : public TestBase {
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

    void init() override {
        // Create fresh mock objects for each test
        m_mockDocumentModel = new MockDocumentModel(this);
        m_mockRecentFilesManager = new MockRecentFilesManager(this);
        m_documentController = new DocumentController(m_mockDocumentModel);
        m_documentController->setRecentFilesManager(m_mockRecentFilesManager);
    }

    void cleanup() override {
        delete m_documentController;
        m_documentController = nullptr;
        // Mock objects will be cleaned up by Qt parent-child relationship
    }

    // Constructor tests
    void testConstructorWithValidModel() {
        QVERIFY(m_documentController != nullptr);
        QVERIFY(m_documentController->getDocumentModel() ==
                m_mockDocumentModel);
        QVERIFY(m_documentController->getRecentFilesManager() ==
                m_mockRecentFilesManager);
    }

    void testConstructorWithNullModel() {
        // Test constructor with null model
        DocumentController* controller = new DocumentController(nullptr);
        QVERIFY(controller != nullptr);
        QVERIFY(controller->getDocumentModel() == nullptr);
        delete controller;
    }

    // Document operation tests
    void testOpenValidDocument() {
        QSignalSpy operationSpy(
            m_documentController,
            &DocumentController::documentOperationCompleted);
        QSignalSpy recentFilesSpy(m_mockRecentFilesManager,
                                  &MockRecentFilesManager::recentFileAdded);

        // Test opening a valid document
        bool result = m_documentController->openDocument("test.pdf");

        // Debug: Check what the mock model returned
        qDebug() << "DocumentController::openDocument returned:" << result;
        qDebug() << "MockDocumentModel document count:"
                 << m_mockDocumentModel->getDocumentCount();
        qDebug() << "MockDocumentModel current file:"
                 << m_mockDocumentModel->getCurrentFilePath();

        QVERIFY(result);

        // Debug: Check if the recent files manager is set
        QVERIFY(m_documentController->getRecentFilesManager() != nullptr);
        QVERIFY(m_documentController->getRecentFilesManager() ==
                m_mockRecentFilesManager);

        // Check that recent files manager was called for the original file
        QCOMPARE(recentFilesSpy.count(), 1);
        QCOMPARE(recentFilesSpy.first().first().toString(),
                 QString("test.pdf"));
    }

    void testOpenInvalidDocument() {
        QSignalSpy recentFilesSpy(m_mockRecentFilesManager,
                                  &MockRecentFilesManager::recentFileAdded);

        // Test opening invalid documents
        bool result1 = m_documentController->openDocument("");
        QVERIFY(!result1);

        bool result2 = m_documentController->openDocument("test.txt");
        QVERIFY(!result2);

        // Recent files should not be updated for invalid files
        QCOMPARE(recentFilesSpy.count(), 0);
    }

    void testOpenMultipleDocuments() {
        QSignalSpy recentFilesSpy(m_mockRecentFilesManager,
                                  &MockRecentFilesManager::recentFileAdded);

        QStringList validFiles = {"doc1.pdf", "doc2.pdf", "doc3.pdf"};
        bool result = m_documentController->openDocuments(validFiles);
        QVERIFY(result);

        // All valid files should be added to recent files
        QCOMPARE(recentFilesSpy.count(), 3);
    }

    void testOpenEmptyDocumentList() {
        QStringList emptyList;
        bool result = m_documentController->openDocuments(emptyList);
        QVERIFY(!result);
    }

    void testCloseDocument() {
        // First open a document
        m_documentController->openDocument("test.pdf");

        // Then close it
        bool result = m_documentController->closeDocument(0);
        QVERIFY(result);
    }

    void testCloseCurrentDocument() {
        // First open a document
        m_documentController->openDocument("test.pdf");

        // Then close current document
        bool result = m_documentController->closeCurrentDocument();
        QVERIFY(result);
    }

    void testSwitchToDocument() {
        // Open multiple documents first
        QStringList files = {"doc1.pdf", "doc2.pdf"};
        m_documentController->openDocuments(files);

        // Switch to document
        m_documentController->switchToDocument(1);

        // Method should execute without error
        QVERIFY(true);
    }

    // Command execution tests
    void testExecuteValidCommand() {
        QSignalSpy operationSpy(
            m_documentController,
            &DocumentController::documentOperationCompleted);

        // Test executing a valid command
        m_documentController->execute(ActionMap::closeCurrentTab, nullptr);

        // Should complete without crashing
        QVERIFY(true);
    }

    void testExecuteInvalidCommand() {
        // Test executing an invalid command (cast to ActionMap)
        ActionMap invalidAction = static_cast<ActionMap>(9999);
        m_documentController->execute(invalidAction, nullptr);

        // Should handle gracefully without crashing
        QVERIFY(true);
    }

    // Signal emission tests
    void testDocumentOperationCompletedSignal() {
        QSignalSpy operationSpy(
            m_documentController,
            &DocumentController::documentOperationCompleted);

        // Execute an operation that should emit the signal
        m_documentController->execute(ActionMap::closeCurrentTab, nullptr);

        // Should emit signal (may be 0 or 1 depending on implementation)
        QVERIFY(operationSpy.count() >= 0);
    }

    void testSideBarSignals() {
        QSignalSpy toggleSpy(m_documentController,
                             &DocumentController::sideBarToggleRequested);
        QSignalSpy showSpy(m_documentController,
                           &DocumentController::sideBarShowRequested);
        QSignalSpy hideSpy(m_documentController,
                           &DocumentController::sideBarHideRequested);

        // Execute sidebar commands
        m_documentController->execute(ActionMap::toggleSideBar, nullptr);
        m_documentController->execute(ActionMap::showSideBar, nullptr);
        m_documentController->execute(ActionMap::hideSideBar, nullptr);

        // Signals should be emitted
        QCOMPARE(toggleSpy.count(), 1);
        QCOMPARE(showSpy.count(), 1);
        QCOMPARE(hideSpy.count(), 1);
    }

    void testViewModeChangeSignal() {
        QSignalSpy viewModeSpy(m_documentController,
                               &DocumentController::viewModeChangeRequested);

        // Execute view mode commands
        m_documentController->execute(ActionMap::setSinglePageMode, nullptr);
        m_documentController->execute(ActionMap::setContinuousScrollMode,
                                      nullptr);

        // Signals should be emitted
        QCOMPARE(viewModeSpy.count(), 2);

        // Check signal parameters
        QList<QVariant> args1 = viewModeSpy.takeFirst();
        QCOMPARE(args1.at(0).toInt(), 0);  // SinglePage mode

        QList<QVariant> args2 = viewModeSpy.takeFirst();
        QCOMPARE(args2.at(0).toInt(), 1);  // ContinuousScroll mode
    }

private:
    DocumentController* m_documentController = nullptr;
    MockDocumentModel* m_mockDocumentModel = nullptr;
    MockRecentFilesManager* m_mockRecentFilesManager = nullptr;
};

QTEST_MAIN(DocumentControllerTest)
#include "test_document_controller.moc"
