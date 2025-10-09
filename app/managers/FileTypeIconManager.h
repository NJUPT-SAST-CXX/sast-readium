#pragma once

#include <QFileInfo>
#include <QHash>
#include <QIcon>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QSvgRenderer>

/**
 * File Type Icon Manager
 * Manages file type icons for the welcome interface and other components
 */
class FileTypeIconManager : public QObject {
    Q_OBJECT

public:
    static FileTypeIconManager& instance();

    // Icon retrieval
    QIcon getFileTypeIcon(const QString& filePath, int size = 24) const;
    QIcon getFileTypeIcon(const QFileInfo& fileInfo, int size = 24) const;
    QPixmap getFileTypePixmap(const QString& filePath, int size = 24) const;
    QPixmap getFileTypePixmap(const QFileInfo& fileInfo, int size = 24) const;

    // Icon management
    void preloadIcons();
    void clearCache();
    void setIconSize(int size);

    // Supported file types
    QStringList getSupportedExtensions() const;
    bool isSupported(const QString& extension) const;

private:
    FileTypeIconManager(QObject* parent = nullptr);
    ~FileTypeIconManager() = default;
    FileTypeIconManager(const FileTypeIconManager&) = delete;
    FileTypeIconManager& operator=(const FileTypeIconManager&) = delete;

    // Helper methods
    QString getIconPath(const QString& extension) const;
    QPixmap loadSvgIcon(const QString& path, int size) const;
    QPixmap createColoredIcon(const QPixmap& base, const QColor& color) const;
    QString normalizeExtension(const QString& extension) const;

    // Cache management
    mutable QHash<QString, QPixmap> m_iconCache;
    mutable QHash<QString, QString> m_extensionToIconMap;

    // Settings
    int m_defaultIconSize;
    QString m_iconBasePath;

    // Supported file types mapping
    void initializeExtensionMapping();
    QHash<QString, QString> m_fileTypeMapping;
};

// Convenience macro
#define FILE_ICON_MANAGER FileTypeIconManager::instance()
