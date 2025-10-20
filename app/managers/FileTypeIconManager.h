#pragma once

#include <QFileInfo>
#include <QIcon>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <memory>

// Forward declaration
class FileTypeIconManagerImpl;

/**
 * File Type Icon Manager
 * Manages file type icons for the welcome interface and other components
 * Note: Protected destructor is intentional for singleton pattern with QObject
 * base
 */
// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class FileTypeIconManager : public QObject {
    Q_OBJECT

public:
    static FileTypeIconManager& instance();

    // Icon retrieval
    [[nodiscard]] QIcon getFileTypeIcon(const QString& filePath,
                                        int size = 24) const;
    [[nodiscard]] QIcon getFileTypeIcon(const QFileInfo& fileInfo,
                                        int size = 24) const;
    [[nodiscard]] QPixmap getFileTypePixmap(const QString& filePath,
                                            int size = 24) const;
    [[nodiscard]] QPixmap getFileTypePixmap(const QFileInfo& fileInfo,
                                            int size = 24) const;

    // Icon management
    void preloadIcons();
    void clearCache();
    void setIconSize(int size);

    // Supported file types
    [[nodiscard]] QStringList getSupportedExtensions() const;
    [[nodiscard]] bool isSupported(const QString& extension) const;

    // Deleted copy/move operations (public for better error messages)
    FileTypeIconManager(const FileTypeIconManager&) = delete;
    FileTypeIconManager& operator=(const FileTypeIconManager&) = delete;
    FileTypeIconManager(FileTypeIconManager&&) = delete;
    FileTypeIconManager& operator=(FileTypeIconManager&&) = delete;

protected:
    // Protected destructor for singleton pattern - prevents deletion through
    // base class pointer while allowing the static instance to be destroyed.
    // Must override QObject's virtual destructor.
    ~FileTypeIconManager() override;

private:
    explicit FileTypeIconManager(QObject* parent = nullptr);

    std::unique_ptr<FileTypeIconManagerImpl> m_pImpl;
};

// Convenience macro
#define FILE_ICON_MANAGER FileTypeIconManager::instance()
