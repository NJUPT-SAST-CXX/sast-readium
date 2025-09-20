#include "ControllerTestMocks.h"
#include <QApplication>
#include <QTemporaryFile>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QTimer>
#include <QEventLoop>

// MockMainWindow implementation
MockMainWindow::MockMainWindow(QWidget* parent) : QMainWindow(parent) {
    resize(800, 600);
    setWindowTitle("Mock Main Window");
}

// MockDocumentModel implementation
MockDocumentModel::MockDocumentModel(QObject* parent) : QObject(parent) {
    // Initialize with default empty state
}

bool MockDocumentModel::openFromFile(const QString& filePath) {
    if (filePath.isEmpty() || !filePath.toLower().endsWith(".pdf")) {
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

bool MockDocumentModel::openFromFiles(const QStringList& filePaths) {
    if (filePaths.isEmpty()) {
        return false;
    }
    
    int validCount = 0;
    for (const QString& path : filePaths) {
        if (!path.isEmpty() && path.toLower().endsWith(".pdf")) {
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
        emit documentOpened(0, m_currentFileName);
    }
    
    return true;
}

bool MockDocumentModel::closeDocument(int index) {
    if (index < 0 || index >= m_documentCount) {
        return false;
    }
    
    m_documentCount--;
    if (m_documentCount == 0) {
        m_isEmpty = true;
        m_currentIndex = -1;
        m_currentFilePath.clear();
        m_currentFileName.clear();
    } else if (index == m_currentIndex) {
        // If closing current document, switch to first available
        m_currentIndex = 0;
    }
    
    emit documentClosed(index);
    return true;
}

bool MockDocumentModel::closeCurrentDocument() {
    return closeDocument(m_currentIndex);
}

void MockDocumentModel::switchToDocument(int index) {
    if (index >= 0 && index < m_documentCount) {
        m_currentIndex = index;
        emit currentDocumentChanged(index);
    }
}

// MockPageModel implementation
MockPageModel::MockPageModel(QObject* parent) : QObject(parent) {
    // Initialize with default values
}

void MockPageModel::setCurrentPage(int page) {
    if (page >= 1 && page <= m_totalPages) {
        m_currentPage = page;
        emit pageUpdate(m_currentPage, m_totalPages);
    }
}

void MockPageModel::setTotalPages(int total) {
    if (total > 0) {
        m_totalPages = total;
        if (m_currentPage > m_totalPages) {
            m_currentPage = m_totalPages;
        }
        emit pageUpdate(m_currentPage, m_totalPages);
    }
}

// MockRenderModel implementation
MockRenderModel::MockRenderModel(int dpiX, int dpiY, QObject* parent) 
    : QObject(parent), m_dpiX(dpiX), m_dpiY(dpiY) {
    // Initialize with provided DPI values
}

// MockRecentFilesManager implementation
MockRecentFilesManager::MockRecentFilesManager(QObject* parent) : QObject(parent) {
    // Initialize with empty recent files list
}

void MockRecentFilesManager::addRecentFile(const QString& filePath) {
    if (filePath.isEmpty()) {
        return;
    }
    
    // Remove if already exists to avoid duplicates
    m_recentFiles.removeAll(filePath);
    
    // Add to front
    m_recentFiles.prepend(filePath);
    
    // Limit size
    while (m_recentFiles.size() > m_maxRecentFiles) {
        m_recentFiles.removeLast();
    }
    
    emit recentFileAdded(filePath);
    emit recentFilesChanged();
}

void MockRecentFilesManager::clearRecentFiles() {
    m_recentFiles.clear();
    emit recentFilesCleared();
    emit recentFilesChanged();
}

// MockStyleManager implementation
MockStyleManager::MockStyleManager(QObject* parent) : QObject(parent) {
    // Initialize with default theme
}

void MockStyleManager::setTheme(const QString& theme) {
    if (m_availableThemes.contains(theme) && m_currentTheme != theme) {
        m_currentTheme = theme;
        emit themeChanged(theme);
    }
}

// MockWelcomeScreenManager implementation
MockWelcomeScreenManager::MockWelcomeScreenManager(QObject* parent) : QObject(parent) {
    // Initialize with default state
}

void MockWelcomeScreenManager::showWelcomeScreen() {
    emit welcomeScreenShown();
}

void MockWelcomeScreenManager::hideWelcomeScreen() {
    emit welcomeScreenHidden();
}

// MockUIComponent implementation
MockUIComponent::MockUIComponent(QObject* parent) : QObject(parent) {
    // Initialize with default state
}

void MockUIComponent::setVisible(bool visible) {
    if (m_visible != visible) {
        m_visible = visible;
        emit visibilityChanged(visible);
    }
}

// ControllerTestUtils implementation
QString ControllerTestUtils::createTempPdfFile(const QString& content) {
    QTemporaryFile* tempFile = new QTemporaryFile(
        QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/test_XXXXXX.pdf");
    
    if (tempFile->open()) {
        tempFile->write(content.toUtf8());
        tempFile->close();
        QString fileName = tempFile->fileName();
        tempFile->setAutoRemove(false); // Don't auto-remove so we can use it in tests
        return fileName;
    }
    
    delete tempFile;
    return QString();
}

QStringList ControllerTestUtils::createTempPdfFiles(int count) {
    QStringList files;
    for (int i = 0; i < count; ++i) {
        QString content = QString("Mock PDF Content %1").arg(i + 1);
        QString fileName = createTempPdfFile(content);
        if (!fileName.isEmpty()) {
            files.append(fileName);
        }
    }
    return files;
}

void ControllerTestUtils::cleanupTempFiles(const QStringList& files) {
    for (const QString& file : files) {
        QFile::remove(file);
    }
}

bool ControllerTestUtils::isValidPdfPath(const QString& path) {
    return !path.isEmpty() && path.toLower().endsWith(".pdf");
}

bool ControllerTestUtils::isValidPageNumber(int page, int totalPages) {
    return page >= 1 && page <= totalPages && totalPages > 0;
}

// MockObjectFactory implementation
MockMainWindow* MockObjectFactory::createMockMainWindow(QWidget* parent) {
    return new MockMainWindow(parent);
}

MockDocumentModel* MockObjectFactory::createMockDocumentModel(QObject* parent) {
    return new MockDocumentModel(parent);
}

MockPageModel* MockObjectFactory::createMockPageModel(int totalPages, QObject* parent) {
    MockPageModel* model = new MockPageModel(parent);
    model->setTotalPages(totalPages);
    return model;
}

MockRenderModel* MockObjectFactory::createMockRenderModel(int dpiX, int dpiY, QObject* parent) {
    return new MockRenderModel(dpiX, dpiY, parent);
}

MockRecentFilesManager* MockObjectFactory::createMockRecentFilesManager(QObject* parent) {
    return new MockRecentFilesManager(parent);
}

MockStyleManager* MockObjectFactory::createMockStyleManager(QObject* parent) {
    return new MockStyleManager(parent);
}

MockWelcomeScreenManager* MockObjectFactory::createMockWelcomeScreenManager(QObject* parent) {
    return new MockWelcomeScreenManager(parent);
}

MockUIComponent* MockObjectFactory::createMockUIComponent(QObject* parent) {
    return new MockUIComponent(parent);
}

// ControllerTestBase implementation
void ControllerTestBase::initTestCase() {
    // Ensure QApplication exists for widget testing
    if (!QApplication::instance()) {
        int argc = 0;
        char** argv = nullptr;
        new QApplication(argc, argv);
    }
}

void ControllerTestBase::cleanupTestCase() {
    // Cleanup will be handled by Qt
}

void ControllerTestBase::init() {
    setupMockObjects();
}

void ControllerTestBase::cleanup() {
    cleanupMockObjects();
}

void ControllerTestBase::setupMockObjects() {
    // Create fresh mock objects for each test
    m_mockMainWindow = MockObjectFactory::createMockMainWindow();
    m_mockDocumentModel = MockObjectFactory::createMockDocumentModel(this);
    m_mockPageModel = MockObjectFactory::createMockPageModel(10, this);
    m_mockRenderModel = MockObjectFactory::createMockRenderModel(96, 96, this);
    m_mockRecentFilesManager = MockObjectFactory::createMockRecentFilesManager(this);
    m_mockStyleManager = MockObjectFactory::createMockStyleManager(this);
    m_mockWelcomeScreenManager = MockObjectFactory::createMockWelcomeScreenManager(this);
}

void ControllerTestBase::cleanupMockObjects() {
    // Delete main window (not managed by Qt parent-child)
    delete m_mockMainWindow;
    m_mockMainWindow = nullptr;
    
    // Other objects will be cleaned up by Qt parent-child relationship
    m_mockDocumentModel = nullptr;
    m_mockPageModel = nullptr;
    m_mockRenderModel = nullptr;
    m_mockRecentFilesManager = nullptr;
    m_mockStyleManager = nullptr;
    m_mockWelcomeScreenManager = nullptr;
}

#include "ControllerTestMocks.moc"
