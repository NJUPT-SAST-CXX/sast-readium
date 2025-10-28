#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest/QtTest>
#include "../../app/managers/RecentFilesManager.h"
#include "../TestUtilities.h"

/**
 * @brief Comprehensive tests for RecentFilesManager
 *
 * Tests recent file management including adding, removing, clearing files,
 * max file limits, persistence, and signal emissions.
 */
class RecentFilesManagerTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic operations tests
    void testAddRecentFile();
    void testAddMultipleFiles();
    void testAddDuplicateFile();
    void testAddNonExistentFile();
    void testRemoveRecentFile();
    void testClearRecentFiles();

    // Query tests
    void testGetRecentFiles();
    void testGetRecentFilePaths();
    void testHasRecentFiles();
    void testGetRecentFilesCount();

    // Configuration tests
    void testSetMaxRecentFiles();
    void testGetMaxRecentFiles();
    void testMaxFilesLimit();

    // Utility tests
    void testCleanupInvalidFiles();
    void testRecentFileInfo();

    // Signal emission tests
    void testRecentFileAddedSignal();
    void testRecentFileRemovedSignal();
    void testRecentFilesClearedSignal();
    void testRecentFilesChangedSignal();

    // Edge cases
    void testAddEmptyPath();
    void testRemoveNonExistentFile();
    void testMRUOrdering();
    void testLargeNumberOfFiles();

private:
    RecentFilesManager* m_manager;
    QStringList m_testFiles;

    // Helper methods
    QString createTestFile(const QString& content = "test");
    void createMultipleTestFiles(int count);
};

void RecentFilesManagerTest::initTestCase() {
    // Test files will be created in each test
}

void RecentFilesManagerTest::cleanupTestCase() {
    // Clean up test files
    for (const QString& file : m_testFiles) {
        QFile::remove(file);
    }
}

void RecentFilesManagerTest::init() {
    m_manager = new RecentFilesManager();
    m_manager->clearRecentFiles();
}

void RecentFilesManagerTest::cleanup() {
    delete m_manager;
    m_manager = nullptr;
}

QString RecentFilesManagerTest::createTestFile(const QString& content) {
    QTemporaryFile* tempFile = new QTemporaryFile();
    tempFile->setAutoRemove(false);

    if (tempFile->open()) {
        tempFile->write(content.toUtf8());
        QString path = tempFile->fileName();
        tempFile->close();
        delete tempFile;

        m_testFiles.append(path);
        return path;
    }

    delete tempFile;
    return QString();
}

void RecentFilesManagerTest::createMultipleTestFiles(int count) {
    for (int i = 0; i < count; ++i) {
        createTestFile(QString("test content %1").arg(i));
    }
}

void RecentFilesManagerTest::testAddRecentFile() {
    QString testFile = createTestFile();
    QVERIFY(!testFile.isEmpty());

    QSignalSpy spy(m_manager, &RecentFilesManager::recentFileAdded);

    m_manager->addRecentFile(testFile);

    QCOMPARE(spy.count(), 1);
    QVERIFY(m_manager->hasRecentFiles());
    QCOMPARE(m_manager->getRecentFilesCount(), 1);
}

void RecentFilesManagerTest::testAddMultipleFiles() {
    createMultipleTestFiles(3);

    for (const QString& file : m_testFiles) {
        m_manager->addRecentFile(file);
    }

    QCOMPARE(m_manager->getRecentFilesCount(), 3);
}

void RecentFilesManagerTest::testAddDuplicateFile() {
    QString testFile = createTestFile();

    m_manager->addRecentFile(testFile);
    int initialCount = m_manager->getRecentFilesCount();

    // Add same file again
    m_manager->addRecentFile(testFile);

    // Should not create duplicate, just update timestamp
    QCOMPARE(m_manager->getRecentFilesCount(), initialCount);
}

void RecentFilesManagerTest::testAddNonExistentFile() {
    QString nonExistent = "/nonexistent/file.pdf";

    m_manager->addRecentFile(nonExistent);

    // Manager should handle gracefully (may or may not add)
    // Just verify no crash
    QVERIFY(true);
}

void RecentFilesManagerTest::testRemoveRecentFile() {
    QString testFile = createTestFile();
    m_manager->addRecentFile(testFile);

    QSignalSpy spy(m_manager, &RecentFilesManager::recentFileRemoved);

    m_manager->removeRecentFile(testFile);

    QCOMPARE(spy.count(), 1);
    QVERIFY(!m_manager->hasRecentFiles());
}

void RecentFilesManagerTest::testClearRecentFiles() {
    createMultipleTestFiles(3);
    for (const QString& file : m_testFiles) {
        m_manager->addRecentFile(file);
    }

    QSignalSpy spy(m_manager, &RecentFilesManager::recentFilesCleared);

    m_manager->clearRecentFiles();

    QCOMPARE(spy.count(), 1);
    QVERIFY(!m_manager->hasRecentFiles());
    QCOMPARE(m_manager->getRecentFilesCount(), 0);
}

void RecentFilesManagerTest::testGetRecentFiles() {
    createMultipleTestFiles(2);

    for (const QString& file : m_testFiles) {
        m_manager->addRecentFile(file);
    }

    QList<RecentFileInfo> files = m_manager->getRecentFiles();

    QCOMPARE(files.size(), 2);
    for (const auto& info : files) {
        QVERIFY(!info.filePath.isEmpty());
        QVERIFY(!info.fileName.isEmpty());
        QVERIFY(info.lastOpened.isValid());
    }
}

void RecentFilesManagerTest::testGetRecentFilePaths() {
    createMultipleTestFiles(2);

    for (const QString& file : m_testFiles) {
        m_manager->addRecentFile(file);
    }

    QStringList paths = m_manager->getRecentFilePaths();

    QCOMPARE(paths.size(), 2);
}

void RecentFilesManagerTest::testHasRecentFiles() {
    QVERIFY(!m_manager->hasRecentFiles());

    QString testFile = createTestFile();
    m_manager->addRecentFile(testFile);

    QVERIFY(m_manager->hasRecentFiles());
}

void RecentFilesManagerTest::testGetRecentFilesCount() {
    QCOMPARE(m_manager->getRecentFilesCount(), 0);

    createMultipleTestFiles(3);
    for (const QString& file : m_testFiles) {
        m_manager->addRecentFile(file);
    }

    QCOMPARE(m_manager->getRecentFilesCount(), 3);
}

void RecentFilesManagerTest::testSetMaxRecentFiles() {
    m_manager->setMaxRecentFiles(5);
    QCOMPARE(m_manager->getMaxRecentFiles(), 5);

    m_manager->setMaxRecentFiles(10);
    QCOMPARE(m_manager->getMaxRecentFiles(), 10);
}

void RecentFilesManagerTest::testGetMaxRecentFiles() {
    int maxFiles = m_manager->getMaxRecentFiles();
    QVERIFY(maxFiles > 0);
}

void RecentFilesManagerTest::testMaxFilesLimit() {
    m_manager->setMaxRecentFiles(3);

    createMultipleTestFiles(5);
    for (const QString& file : m_testFiles) {
        m_manager->addRecentFile(file);
    }

    // Should only keep max files
    QVERIFY(m_manager->getRecentFilesCount() <= 3);
}

void RecentFilesManagerTest::testCleanupInvalidFiles() {
    QString validFile = createTestFile();
    QString invalidFile = "/nonexistent/file.pdf";

    m_manager->addRecentFile(validFile);
    m_manager->addRecentFile(invalidFile);

    m_manager->cleanupInvalidFiles();

    // Should remove invalid files
    QStringList paths = m_manager->getRecentFilePaths();
    QVERIFY(!paths.contains(invalidFile));
}

void RecentFilesManagerTest::testRecentFileInfo() {
    QString testFile = createTestFile();
    RecentFileInfo info(testFile);

    QVERIFY(!info.filePath.isEmpty());
    QVERIFY(!info.fileName.isEmpty());
    QVERIFY(info.lastOpened.isValid());
    QVERIFY(info.isValid());
}

void RecentFilesManagerTest::testRecentFileAddedSignal() {
    QSignalSpy spy(m_manager, &RecentFilesManager::recentFileAdded);

    QString testFile = createTestFile();
    m_manager->addRecentFile(testFile);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), testFile);
}

void RecentFilesManagerTest::testRecentFileRemovedSignal() {
    QString testFile = createTestFile();
    m_manager->addRecentFile(testFile);

    QSignalSpy spy(m_manager, &RecentFilesManager::recentFileRemoved);

    m_manager->removeRecentFile(testFile);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), testFile);
}

void RecentFilesManagerTest::testRecentFilesClearedSignal() {
    QSignalSpy spy(m_manager, &RecentFilesManager::recentFilesCleared);

    m_manager->clearRecentFiles();

    QCOMPARE(spy.count(), 1);
}

void RecentFilesManagerTest::testRecentFilesChangedSignal() {
    QSignalSpy spy(m_manager, &RecentFilesManager::recentFilesChanged);

    QString testFile = createTestFile();
    m_manager->addRecentFile(testFile);

    QVERIFY(spy.count() > 0);
}

void RecentFilesManagerTest::testAddEmptyPath() {
    int initialCount = m_manager->getRecentFilesCount();

    m_manager->addRecentFile("");

    // Should not add empty path
    QCOMPARE(m_manager->getRecentFilesCount(), initialCount);
}

void RecentFilesManagerTest::testRemoveNonExistentFile() {
    // Should handle gracefully without crash
    m_manager->removeRecentFile("/nonexistent/file.pdf");
    QVERIFY(true);
}

void RecentFilesManagerTest::testMRUOrdering() {
    createMultipleTestFiles(3);

    // Add files in order
    for (const QString& file : m_testFiles) {
        m_manager->addRecentFile(file);
        waitMs(10);  // Small delay to ensure different timestamps
    }

    QStringList paths = m_manager->getRecentFilePaths();

    // Most recently added should be first
    QCOMPARE(paths.first(), m_testFiles.last());
}

void RecentFilesManagerTest::testLargeNumberOfFiles() {
    m_manager->setMaxRecentFiles(100);

    createMultipleTestFiles(50);
    for (const QString& file : m_testFiles) {
        m_manager->addRecentFile(file);
    }

    QCOMPARE(m_manager->getRecentFilesCount(), 50);
}

QTEST_MAIN(RecentFilesManagerTest)
#include "test_recent_files_manager.moc"
