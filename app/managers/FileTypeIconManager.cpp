#include "FileTypeIconManager.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QPainter>
#include <QSvgRenderer>
#include <QTimer>
#include "../logging/Logger.h"

// Private implementation class
class FileTypeIconManagerImpl {
public:
    FileTypeIconManagerImpl() = default;

    // Helper methods
    QString getIconPath(const QString& extension) const;
    QPixmap loadSvgIcon(const QString& path, int size) const;
    QPixmap createColoredIcon(const QPixmap& base, const QColor& color) const;
    QString normalizeExtension(const QString& extension) const;
    void initializeExtensionMapping();

    // Cache management
    mutable QHash<QString, QPixmap> m_iconCache;
    mutable QHash<QString, QString> m_extensionToIconMap;

    // Settings
    int m_defaultIconSize{24};
    QString m_iconBasePath{":/images/filetypes/"};

    // Supported file types mapping
    QHash<QString, QString> m_fileTypeMapping;
};

FileTypeIconManager& FileTypeIconManager::instance() {
    static FileTypeIconManager instance;
    return instance;
}

FileTypeIconManager::FileTypeIconManager(QObject* parent)
    : QObject(parent), m_pImpl(std::make_unique<FileTypeIconManagerImpl>()) {
    Logger::instance().info(
        "[managers] Initializing FileTypeIconManager with base path: {}",
        m_pImpl->m_iconBasePath.toStdString());
    m_pImpl->initializeExtensionMapping();

    // CRITICAL FIX: Defer icon preloading until after Qt event loop starts
    // Preloading SVG icons synchronously during initialization (before
    // QApplication::exec()) can cause the application to hang. Instead, we
    // defer this to the event loop.
    Logger::instance().info(
        "[managers] Deferring icon preloading to avoid initialization hang");
    QTimer::singleShot(0, this, [this]() {
        Logger::instance().info("[managers] Starting deferred icon preloading");
        preloadIcons();
        Logger::instance().info(
            "[managers] Deferred icon preloading completed");
    });

    Logger::instance().debug(
        "[managers] FileTypeIconManager initialized with {} file type mappings",
        m_pImpl->m_fileTypeMapping.size());
}

FileTypeIconManager::~FileTypeIconManager() = default;

void FileTypeIconManagerImpl::initializeExtensionMapping() {
    // PDF files
    m_fileTypeMapping["pdf"] = "pdf";

    // EPUB files
    m_fileTypeMapping["epub"] = "epub";
    m_fileTypeMapping["epub3"] = "epub";

    // Text files
    m_fileTypeMapping["txt"] = "txt";
    m_fileTypeMapping["text"] = "txt";
    m_fileTypeMapping["log"] = "txt";
    m_fileTypeMapping["md"] = "txt";
    m_fileTypeMapping["markdown"] = "txt";

    // Document files
    m_fileTypeMapping["doc"] = "doc";
    m_fileTypeMapping["docx"] = "doc";
    m_fileTypeMapping["rtf"] = "doc";
    m_fileTypeMapping["odt"] = "doc";

    Logger::instance().debug(
        "[managers] FileTypeIconManager extension mapping initialized with {} "
        "file types",
        m_fileTypeMapping.size());
}

QIcon FileTypeIconManager::getFileTypeIcon(const QString& filePath,
                                           int size) const {
    QFileInfo fileInfo(filePath);
    return getFileTypeIcon(fileInfo, size);
}

QIcon FileTypeIconManager::getFileTypeIcon(const QFileInfo& fileInfo,
                                           int size) const {
    QPixmap pixmap = getFileTypePixmap(fileInfo, size);
    return QIcon(pixmap);
}

QPixmap FileTypeIconManager::getFileTypePixmap(const QString& filePath,
                                               int size) const {
    QFileInfo fileInfo(filePath);
    return getFileTypePixmap(fileInfo, size);
}

QPixmap FileTypeIconManager::getFileTypePixmap(const QFileInfo& fileInfo,
                                               int size) const {
    QString extension = m_pImpl->normalizeExtension(fileInfo.suffix());
    QString cacheKey = QString("%1_%2").arg(extension).arg(size);

    // Check cache first
    if (m_pImpl->m_iconCache.contains(cacheKey)) {
        Logger::instance().trace(
            "[managers] Icon cache hit for extension '{}' size {}",
            extension.toStdString(), size);
        return m_pImpl->m_iconCache[cacheKey];
    }

    // Load icon
    QString iconPath = m_pImpl->getIconPath(extension);
    Logger::instance().debug(
        "[managers] Loading icon for extension '{}' from path: {}",
        extension.toStdString(), iconPath.toStdString());
    QPixmap pixmap = m_pImpl->loadSvgIcon(iconPath, size);

    // Cache the result
    m_pImpl->m_iconCache[cacheKey] = pixmap;
    Logger::instance().trace("[managers] Cached icon for key: {}",
                             cacheKey.toStdString());

    return pixmap;
}

QString FileTypeIconManagerImpl::getIconPath(const QString& extension) const {
    QString iconName = m_fileTypeMapping.value(extension, "default");
    return QString("%1%2.svg").arg(m_iconBasePath).arg(iconName);
}

QPixmap FileTypeIconManagerImpl::loadSvgIcon(const QString& path,
                                             int size) const {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QSvgRenderer renderer;

    // Try to load from resources first
    if (renderer.load(path)) {
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        renderer.render(&painter);
        return pixmap;
    }

    // Try to load from file system
    QString filePath = path;
    if (filePath.startsWith(":/")) {
        filePath = filePath.mid(2);  // Remove ":/" prefix
        filePath = QApplication::applicationDirPath() + "/../" + filePath;
    }

    if (QFile::exists(filePath) && renderer.load(filePath)) {
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        renderer.render(&painter);
        return pixmap;
    }

    // Fallback: create a simple colored rectangle
    pixmap.fill(QColor(113, 128, 150));  // Default gray color
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "?");

    return pixmap;
}

QString FileTypeIconManagerImpl::normalizeExtension(
    const QString& extension) const {
    return extension.toLower().trimmed();
}

QPixmap FileTypeIconManagerImpl::createColoredIcon(const QPixmap& base,
                                                   const QColor& color) const {
    QPixmap coloredPixmap = base;
    QPainter painter(&coloredPixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(coloredPixmap.rect(), color);
    return coloredPixmap;
}

void FileTypeIconManager::preloadIcons() {
    Logger::instance().debug("[managers] Starting icon preloading process");

    QStringList iconTypes = {"pdf", "epub", "txt", "doc", "default"};
    QList<int> sizes = {16, 24, 32, 48};

    for (const QString& iconType : iconTypes) {
        for (int size : sizes) {
            QString iconPath =
                QString("%1%2.svg").arg(m_pImpl->m_iconBasePath).arg(iconType);
            QPixmap pixmap = m_pImpl->loadSvgIcon(iconPath, size);
            QString cacheKey = QString("%1_%2").arg(iconType).arg(size);
            m_pImpl->m_iconCache[cacheKey] = pixmap;
        }
    }

    Logger::instance().info(
        "[managers] Icon preloading completed - cached {} icons",
        m_pImpl->m_iconCache.size());
}

void FileTypeIconManager::clearCache() {
    const auto cacheSize = m_pImpl->m_iconCache.size();
    m_pImpl->m_iconCache.clear();
    Logger::instance().info(
        "[managers] Icon cache cleared - removed {} cached icons", cacheSize);
}

void FileTypeIconManager::setIconSize(int size) {
    if (m_pImpl->m_defaultIconSize != size) {
        m_pImpl->m_defaultIconSize = size;
        clearCache();  // Clear cache to force reload with new size
    }
}

QStringList FileTypeIconManager::getSupportedExtensions() const {
    return m_pImpl->m_fileTypeMapping.keys();
}

bool FileTypeIconManager::isSupported(const QString& extension) const {
    return m_pImpl->m_fileTypeMapping.contains(
        m_pImpl->normalizeExtension(extension));
}
