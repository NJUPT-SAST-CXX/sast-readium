#pragma once

#include <QObject>
#include <QPixmap>
#include <QIcon>
#include <QString>
#include <QFileInfo>
#include <memory>

// Forward declaration
class FileTypeIconManagerImpl;

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
    ~FileTypeIconManager();
    FileTypeIconManager(const FileTypeIconManager&) = delete;
    FileTypeIconManager& operator=(const FileTypeIconManager&) = delete;

    std::unique_ptr<FileTypeIconManagerImpl> pImpl;
};

// Convenience macro
#define FILE_ICON_MANAGER FileTypeIconManager::instance()
