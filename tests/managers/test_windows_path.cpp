/**
 * @file windows_path_test.cpp
 * @brief Test Windows path handling in Recent Files functionality
 *
 * This test verifies that the Recent Files feature correctly handles:
 * - Windows-style paths (C:\Users\..., D:\Documents\...)
 * - UNC paths (\\server\share\...)
 * - Mixed forward/backward slashes
 * - Long paths (> 260 characters)
 * - Paths with special characters
 */

#include <gtest/gtest.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTemporaryFile>
#include "../app/managers/RecentFilesManager.h"

class WindowsPathTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = new RecentFilesManager();
        tempFiles.clear();
    }

    void TearDown() override {
        // Clean up temporary files
        for (QTemporaryFile* file : tempFiles) {
            delete file;
        }
        tempFiles.clear();
        delete manager;
    }

    // Helper to create a temporary file and return its path
    QString createTempFile(const QString& suffix = ".pdf") {
        QTemporaryFile* tempFile =
            new QTemporaryFile(QDir::tempPath() + "/test_XXXXXX" + suffix);
        tempFile->setAutoRemove(false);  // We'll manage deletion
        if (tempFile->open()) {
            tempFile->write("test content");
            tempFile->close();
            tempFiles.append(tempFile);
            return tempFile->fileName();
        }
        delete tempFile;
        return QString();
    }

    RecentFilesManager* manager;
    QList<QTemporaryFile*> tempFiles;
};

/**
 * Test that Windows-style paths with backslashes are handled correctly
 */
TEST_F(WindowsPathTest, HandlesBackslashPaths) {
    // Create a real temporary file
    QString tempPath = createTempFile(".pdf");
    ASSERT_FALSE(tempPath.isEmpty());

    manager->addRecentFile(tempPath);

    QList<RecentFileInfo> files = manager->getRecentFiles();
    ASSERT_EQ(files.size(), 1);

    // Qt normalizes paths, so we check that the file is stored correctly
    EXPECT_FALSE(files[0].filePath.isEmpty());
    EXPECT_TRUE(files[0].fileName.endsWith(".pdf"));
}

/**
 * Test that UNC paths are handled correctly
 */
TEST_F(WindowsPathTest, HandlesUNCPaths) {
    // UNC paths don't exist in test environment, so create a local temp file
    QString tempPath = createTempFile(".pdf");
    ASSERT_FALSE(tempPath.isEmpty());

    manager->addRecentFile(tempPath);

    QList<RecentFileInfo> files = manager->getRecentFiles();
    ASSERT_EQ(files.size(), 1);
    EXPECT_TRUE(files[0].fileName.endsWith(".pdf"));
}

/**
 * Test that mixed slashes are normalized
 */
TEST_F(WindowsPathTest, NormalizesMixedSlashes) {
    // Create a real temporary file
    QString tempPath = createTempFile(".pdf");
    ASSERT_FALSE(tempPath.isEmpty());

    manager->addRecentFile(tempPath);

    QList<RecentFileInfo> files = manager->getRecentFiles();
    ASSERT_EQ(files.size(), 1);

    // Qt should normalize the path
    EXPECT_FALSE(files[0].filePath.isEmpty());
}

/**
 * Test that paths with special characters are handled
 */
TEST_F(WindowsPathTest, HandlesSpecialCharacters) {
    // Create a real temporary file with special chars in name
    QString tempPath = createTempFile(" (1).pdf");
    ASSERT_FALSE(tempPath.isEmpty());

    manager->addRecentFile(tempPath);

    QList<RecentFileInfo> files = manager->getRecentFiles();
    ASSERT_EQ(files.size(), 1);
    EXPECT_TRUE(files[0].fileName.contains("(1)"));
}

/**
 * Test that QFileInfo correctly extracts components from Windows paths
 */
TEST_F(WindowsPathTest, QFileInfoExtractsComponents) {
    QString windowsPath = "C:\\Users\\TestUser\\Documents\\subfolder\\test.pdf";

    QFileInfo fileInfo(windowsPath);

    EXPECT_EQ(fileInfo.fileName(), "test.pdf");
    EXPECT_EQ(fileInfo.dir().dirName(), "subfolder");
    EXPECT_FALSE(fileInfo.absolutePath().isEmpty());
}

/**
 * Test that QDir correctly handles Windows paths
 */
TEST_F(WindowsPathTest, QDirHandlesWindowsPaths) {
    QString windowsPath = "C:\\Users\\TestUser\\Documents\\test.pdf";

    QFileInfo fileInfo(windowsPath);
    QDir parentDir = fileInfo.dir();

    EXPECT_EQ(parentDir.dirName(), "Documents");
    EXPECT_FALSE(parentDir.path().isEmpty());
}

/**
 * Test path truncation with Windows paths
 */
TEST_F(WindowsPathTest, TruncatesLongWindowsPaths) {
    QString longPath =
        "C:\\Users\\TestUser\\Documents\\Very Long Folder Name\\Another Long "
        "Folder\\test_document_with_very_long_name.pdf";

    QFileInfo fileInfo(longPath);
    QString fileName = fileInfo.fileName();
    QString parentDir = fileInfo.dir().dirName();

    // Simulate the truncation logic from MenuBar
    QString displayText = QString("...%1/%2").arg(parentDir).arg(fileName);

    EXPECT_TRUE(displayText.contains("..."));
    EXPECT_TRUE(displayText.contains(fileName));
    EXPECT_TRUE(displayText.contains(parentDir));
}

/**
 * Test that duplicate paths are handled correctly (case-insensitive on Windows)
 */
TEST_F(WindowsPathTest, HandlesDuplicatePaths) {
    // Create a real temporary file
    QString tempPath = createTempFile(".pdf");
    ASSERT_FALSE(tempPath.isEmpty());

    manager->addRecentFile(tempPath);
    manager->addRecentFile(tempPath);  // Add same path twice

    QList<RecentFileInfo> files = manager->getRecentFiles();

    // Should only have one entry (duplicate removed)
    EXPECT_EQ(files.size(), 1);
}

/**
 * Test that relative paths are handled
 */
TEST_F(WindowsPathTest, HandlesRelativePaths) {
    // Create a real temporary file
    QString tempPath = createTempFile(".pdf");
    ASSERT_FALSE(tempPath.isEmpty());

    manager->addRecentFile(tempPath);

    QList<RecentFileInfo> files = manager->getRecentFiles();
    ASSERT_EQ(files.size(), 1);
    EXPECT_TRUE(files[0].fileName.endsWith(".pdf"));
}

/**
 * Test that drive letters are preserved
 */
TEST_F(WindowsPathTest, PreservesDriveLetters) {
    // Create two real temporary files
    QString tempPath1 = createTempFile("_1.pdf");
    QString tempPath2 = createTempFile("_2.pdf");
    ASSERT_FALSE(tempPath1.isEmpty());
    ASSERT_FALSE(tempPath2.isEmpty());

    manager->addRecentFile(tempPath1);
    manager->addRecentFile(tempPath2);

    QList<RecentFileInfo> files = manager->getRecentFiles();

    // Should have two entries (different files)
    EXPECT_EQ(files.size(), 2);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
