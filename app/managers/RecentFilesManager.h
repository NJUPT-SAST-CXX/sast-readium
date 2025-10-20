#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QObject>
#include <QStringList>
#include <memory>

// Forward declaration
class RecentFilesManagerImpl;

/**
 * 最近文件信息结构
 */
struct RecentFileInfo {
    QString filePath;
    QString fileName;
    QDateTime lastOpened;
    qint64 fileSize;

    RecentFileInfo() : fileSize(0) {}

    explicit RecentFileInfo(const QString& path) : filePath(path), fileSize(0) {
        QFileInfo info(path);
        fileName = info.fileName();
        lastOpened = QDateTime::currentDateTime();
        if (info.exists()) {
            fileSize = info.size();
        }
    }

    [[nodiscard]] bool isValid() const {
        return !filePath.isEmpty() && QFileInfo(filePath).exists();
    }

    bool operator==(const RecentFileInfo& other) const {
        return filePath == other.filePath;
    }
};

/**
 * 最近文件管理器
 * 负责管理最近打开的文件列表，提供添加、获取、清空等功能
 */
class RecentFilesManager : public QObject {
    Q_OBJECT

public:
    explicit RecentFilesManager(QObject* parent = nullptr);
    ~RecentFilesManager() override;

    // 文件操作
    virtual void addRecentFile(const QString& filePath);
    [[nodiscard]] QList<RecentFileInfo> getRecentFiles() const;
    [[nodiscard]] QStringList getRecentFilePaths() const;
    void clearRecentFiles();
    void removeRecentFile(const QString& filePath);

    // 配置管理
    void setMaxRecentFiles(int maxFiles);
    [[nodiscard]] int getMaxRecentFiles() const;

    // 实用功能
    [[nodiscard]] bool hasRecentFiles() const;
    [[nodiscard]] int getRecentFilesCount() const;
    void cleanupInvalidFiles();

    // 异步初始化
    void initializeAsync();

    // Deleted copy/move operations (public for better error messages)
    RecentFilesManager(const RecentFilesManager&) = delete;
    RecentFilesManager& operator=(const RecentFilesManager&) = delete;
    RecentFilesManager(RecentFilesManager&&) = delete;
    RecentFilesManager& operator=(RecentFilesManager&&) = delete;

signals:
    void recentFilesChanged();
    void recentFileAdded(const QString& filePath);
    void recentFileRemoved(const QString& filePath);
    void recentFilesCleared();

private slots:
    void saveSettings();

private:
    void loadSettings();
    std::unique_ptr<RecentFilesManagerImpl> m_pImpl;

public:
    // Static constants - made public for implementation class access
    // Using constexpr to avoid cert-err58-cpp (static storage duration
    // exceptions)
    static constexpr QLatin1StringView SETTINGS_GROUP{"recentFiles"};
    static constexpr QLatin1StringView SETTINGS_MAX_FILES_KEY{"maxFiles"};
    static constexpr QLatin1StringView SETTINGS_FILES_KEY{"files"};
};
