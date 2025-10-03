#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QMainWindow>
#include <QObject>
#include <QStack>
#include <QString>
#include <QStringList>
#include <QVariant>
#include "../TestUtilities.h"

/**
 * @brief Mock objects for controller testing
 *
 * This file contains reusable mock objects that can be shared
 * across different controller tests to avoid code duplication.
 */

// Forward declarations
class MockDocumentModel;
class MockPageModel;
class MockRenderModel;
class MockRecentFilesManager;

/**
 * @brief Mock MainWindow for ApplicationController testing
 */
class MockMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MockMainWindow(QWidget* parent = nullptr);

    // DPI methods for consistent testing
    int logicalDpiX() const { return m_dpiX; }
    int logicalDpiY() const { return m_dpiY; }

    void setDpi(int dpiX, int dpiY) {
        m_dpiX = dpiX;
        m_dpiY = dpiY;
    }

private:
    int m_dpiX = 96;
    int m_dpiY = 96;
};

/**
 * @brief Mock DocumentModel for testing document operations
 */
class MockDocumentModel : public QObject {
    Q_OBJECT
public:
    explicit MockDocumentModel(QObject* parent = nullptr);

    // Document state
    bool isEmpty() const { return m_isEmpty; }
    int getDocumentCount() const { return m_documentCount; }
    int getCurrentDocumentIndex() const { return m_currentIndex; }
    QString getCurrentFilePath() const { return m_currentFilePath; }
    QString getCurrentFileName() const { return m_currentFileName; }

    // Mock document operations
    bool openFromFile(const QString& filePath);
    bool openFromFiles(const QStringList& filePaths);
    bool closeDocument(int index);
    bool closeCurrentDocument();
    void switchToDocument(int index);

    // Mock for getCurrentDocument - returns nullptr since we can't create real
    // Poppler::Document
    void* getCurrentDocument() const { return nullptr; }

    // Test helpers
    void setEmpty(bool empty) { m_isEmpty = empty; }
    void setDocumentCount(int count) { m_documentCount = count; }
    void setCurrentIndex(int index) { m_currentIndex = index; }

signals:
    void documentOpened(int index, const QString& fileName);
    void documentClosed(int index);
    void currentDocumentChanged(int index);

private:
    bool m_isEmpty = true;
    int m_documentCount = 0;
    int m_currentIndex = -1;
    QString m_currentFilePath;
    QString m_currentFileName;
};

/**
 * @brief Mock PageModel for testing page operations
 */
class MockPageModel : public QObject {
    Q_OBJECT
public:
    explicit MockPageModel(QObject* parent = nullptr);

    int currentPage() const { return m_currentPage; }
    int totalPages() const { return m_totalPages; }

    void setCurrentPage(int page);
    void setTotalPages(int total);

    // Test helpers
    void reset() {
        m_currentPage = 1;
        m_totalPages = 1;
    }

signals:
    void pageUpdate(int currentPage, int totalPages);

private:
    int m_currentPage = 1;
    int m_totalPages = 1;
};

/**
 * @brief Mock RenderModel for testing rendering operations
 */
class MockRenderModel : public QObject {
    Q_OBJECT
public:
    explicit MockRenderModel(int dpiX, int dpiY, QObject* parent = nullptr);

    int dpiX() const { return m_dpiX; }
    int dpiY() const { return m_dpiY; }

    void setDpi(int dpiX, int dpiY) {
        m_dpiX = dpiX;
        m_dpiY = dpiY;
    }

private:
    int m_dpiX;
    int m_dpiY;
};

/**
 * @brief Mock RecentFilesManager for testing recent files functionality
 */
class MockRecentFilesManager : public QObject {
    Q_OBJECT
public:
    explicit MockRecentFilesManager(QObject* parent = nullptr);

    void addRecentFile(const QString& filePath);
    void clearRecentFiles();
    QStringList recentFiles() const { return m_recentFiles; }

    // Test helpers
    void setMaxRecentFiles(int max) { m_maxRecentFiles = max; }
    int maxRecentFiles() const { return m_maxRecentFiles; }

signals:
    void recentFileAdded(const QString& filePath);
    void recentFilesCleared();
    void recentFilesChanged();

private:
    QStringList m_recentFiles;
    int m_maxRecentFiles = 10;
};

/**
 * @brief Mock StyleManager for testing theme operations
 */
class MockStyleManager : public QObject {
    Q_OBJECT
public:
    explicit MockStyleManager(QObject* parent = nullptr);

    QString currentTheme() const { return m_currentTheme; }
    void setTheme(const QString& theme);

    QStringList availableThemes() const { return m_availableThemes; }
    void setAvailableThemes(const QStringList& themes) {
        m_availableThemes = themes;
    }

signals:
    void themeChanged(const QString& theme);

private:
    QString m_currentTheme = "light";
    QStringList m_availableThemes = {"light", "dark"};
};

/**
 * @brief Mock WelcomeScreenManager for testing welcome screen functionality
 */
class MockWelcomeScreenManager : public QObject {
    Q_OBJECT
public:
    explicit MockWelcomeScreenManager(QObject* parent = nullptr);

    bool shouldShowWelcomeScreen() const { return m_shouldShow; }
    void setShouldShowWelcomeScreen(bool show) { m_shouldShow = show; }

    void showWelcomeScreen();
    void hideWelcomeScreen();

signals:
    void welcomeScreenShown();
    void welcomeScreenHidden();

private:
    bool m_shouldShow = true;
};

/**
 * @brief Mock UI Component for testing UI interactions
 */
class MockUIComponent : public QObject {
    Q_OBJECT
public:
    explicit MockUIComponent(QObject* parent = nullptr);

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible);

    int getPreferredWidth() const { return m_preferredWidth; }
    void setPreferredWidth(int width) { m_preferredWidth = width; }

    void applyTheme() { m_themeApplied = true; }
    bool wasThemeApplied() const { return m_themeApplied; }

signals:
    void visibilityChanged(bool visible);
    void themeApplied();

private:
    bool m_visible = true;
    int m_preferredWidth = 200;
    bool m_themeApplied = false;
};

/**
 * @brief Test utilities for controller testing
 */
class ControllerTestUtils {
public:
    // File system helpers
    static QString createTempPdfFile(
        const QString& content = "Mock PDF Content");
    static QStringList createTempPdfFiles(int count);
    static void cleanupTempFiles(const QStringList& files);

    // Validation helpers
    static bool isValidPdfPath(const QString& path);
    static bool isValidPageNumber(int page, int totalPages);

    // Signal testing helpers
    template <typename T>
    static bool waitForSignal(T* object, const char* signal,
                              int timeout = 5000) {
        QSignalSpy spy(object, signal);
        return spy.wait(timeout);
    }

    template <typename T>
    static int getSignalCount(T* object, const char* signal) {
        QSignalSpy spy(object, signal);
        QCoreApplication::processEvents();
        return spy.count();
    }
};

/**
 * @brief Factory for creating mock objects
 */
class MockObjectFactory {
public:
    static MockMainWindow* createMockMainWindow(QWidget* parent = nullptr);
    static MockDocumentModel* createMockDocumentModel(
        QObject* parent = nullptr);
    static MockPageModel* createMockPageModel(int totalPages = 10,
                                              QObject* parent = nullptr);
    static MockRenderModel* createMockRenderModel(int dpiX = 96, int dpiY = 96,
                                                  QObject* parent = nullptr);
    static MockRecentFilesManager* createMockRecentFilesManager(
        QObject* parent = nullptr);
    static MockStyleManager* createMockStyleManager(QObject* parent = nullptr);
    static MockWelcomeScreenManager* createMockWelcomeScreenManager(
        QObject* parent = nullptr);
    static MockUIComponent* createMockUIComponent(QObject* parent = nullptr);
};

/**
 * @brief Base class for controller tests with common setup
 */
class ControllerTestBase : public TestBase {
    Q_OBJECT

protected:
    void setupMockObjects();
    void cleanupMockObjects();

    // Common mock objects
    MockMainWindow* m_mockMainWindow = nullptr;
    MockDocumentModel* m_mockDocumentModel = nullptr;
    MockPageModel* m_mockPageModel = nullptr;
    MockRenderModel* m_mockRenderModel = nullptr;
    MockRecentFilesManager* m_mockRecentFilesManager = nullptr;
    MockStyleManager* m_mockStyleManager = nullptr;
    MockWelcomeScreenManager* m_mockWelcomeScreenManager = nullptr;

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;
};
