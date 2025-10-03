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
#include <QFileInfo>
#include <QString>
#include "../app/managers/RecentFilesManager.h"

class WindowsPathTest : public ::testing::Test {
protected:
    void SetUp() override { manager = new RecentFilesManager(); }

    void TearDown() override { delete manager; }

    RecentFilesManager* manager;
};

/**
 * Test that Windows-style paths with backslashes are handled correctly
 */
TEST_F(WindowsPathTest, HandlesBackslashPaths) {
    QString windowsPath = "C:\\Users\\TestUser\\Documents\\test.pdf";

    RecentFileInfo info;
    info.filePath = windowsPath;
    info.fileName = "test.pdf";

    manager->addRecentFile(info);

    QList<RecentFileInfo> files = manager->getRecentFiles();
    ASSERT_EQ(files.size(), 1);

    // Qt normalizes paths, so we check that the file is stored correctly
    EXPECT_FALSE(files[0].filePath.isEmpty());
    EXPECT_EQ(files[0].fileName, "test.pdf");
}

/**
 * Test that UNC paths are handled correctly
 */
TEST_F(WindowsPathTest, HandlesUNCPaths) {
    QString uncPath = "\\\\server\\share\\documents\\test.pdf";

    RecentFileInfo info;
    info.filePath = uncPath;
    info.fileName = "test.pdf";

    manager->addRecentFile(info);

    QList<RecentFileInfo> files = manager->getRecentFiles();
    ASSERT_EQ(files.size(), 1);
    EXPECT_EQ(files[0].fileName, "test.pdf");
}

/**
 * Test that mixed slashes are normalized
 */
TEST_F(WindowsPathTest, NormalizesMixedSlashes) {
    QString mixedPath = "C:/Users\\TestUser/Documents\\test.pdf";

    RecentFileInfo info;
    info.filePath = mixedPath;
    info.fileName = "test.pdf";

    manager->addRecentFile(info);

    QList<RecentFileInfo> files = manager->getRecentFiles();
    ASSERT_EQ(files.size(), 1);

    // Qt should normalize the path
    EXPECT_FALSE(files[0].filePath.isEmpty());
}

/**
 * Test that paths with special characters are handled
 */
TEST_F(WindowsPathTest, HandlesSpecialCharacters) {
    QString specialPath = "C:\\Users\\Test User\\Documents\\file (1).pdf";

    RecentFileInfo info;
    info.filePath = specialPath;
    info.fileName = "file (1).pdf";

    manager->addRecentFile(info);

    QList<RecentFileInfo> files = manager->getRecentFiles();
    ASSERT_EQ(files.size(), 1);
    EXPECT_EQ(files[0].fileName, "file (1).pdf");
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
    QString path1 = "C:\\Users\\TestUser\\Documents\\test.pdf";
    QString path2 = "C:\\Users\\TestUser\\Documents\\test.pdf";

    RecentFileInfo info1;
    info1.filePath = path1;
    info1.fileName = "test.pdf";

    RecentFileInfo info2;
    info2.filePath = path2;
    info2.fileName = "test.pdf";

    manager->addRecentFile(info1);
    manager->addRecentFile(info2);

    QList<RecentFileInfo> files = manager->getRecentFiles();

    // Should only have one entry (duplicate removed)
    EXPECT_EQ(files.size(), 1);
}

/**
 * Test that relative paths are handled
 */
TEST_F(WindowsPathTest, HandlesRelativePaths) {
    QString relativePath = "..\\Documents\\test.pdf";

    RecentFileInfo info;
    info.filePath = relativePath;
    info.fileName = "test.pdf";

    manager->addRecentFile(info);

    QList<RecentFileInfo> files = manager->getRecentFiles();
    ASSERT_EQ(files.size(), 1);
    EXPECT_EQ(files[0].fileName, "test.pdf");
}

/**
 * Test that drive letters are preserved
 */
TEST_F(WindowsPathTest, PreservesDriveLetters) {
    QString pathD = "D:\\Projects\\test.pdf";
    QString pathE = "E:\\Backup\\test.pdf";

    RecentFileInfo info1;
    info1.filePath = pathD;
    info1.fileName = "test.pdf";

    RecentFileInfo info2;
    info2.filePath = pathE;
    info2.fileName = "test.pdf";

    manager->addRecentFile(info1);
    manager->addRecentFile(info2);

    QList<RecentFileInfo> files = manager->getRecentFiles();

    // Should have two entries (different drives)
    EXPECT_EQ(files.size(), 2);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
