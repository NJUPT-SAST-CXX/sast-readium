#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QMutex>
#include <QObject>
#include <QSettings>
#include <QStringList>

/**
 * 最近文件信息结构
 */
struct RecentFileInfo {
    QString filePath;
    QString fileName;
    QDateTime lastOpened;
    qint64 fileSize;

    RecentFileInfo() : fileSize(0) {}

    RecentFileInfo(const QString& path) : filePath(path), fileSize(0) {
        QFileInfo info(path);
        fileName = info.fileName();
        lastOpened = QDateTime::currentDateTime();
        if (info.exists()) {
            fileSize = info.size();
        }
    }

    bool isValid() const {
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
    ~RecentFilesManager();

    // 文件操作
    virtual void addRecentFile(const QString& filePath);
    QList<RecentFileInfo> getRecentFiles() const;
    QStringList getRecentFilePaths() const;
    void clearRecentFiles();
    void removeRecentFile(const QString& filePath);

    // 配置管理
    void setMaxRecentFiles(int maxFiles);
    int getMaxRecentFiles() const;

    // 实用功能
    bool hasRecentFiles() const;
    int getRecentFilesCount() const;
    void cleanupInvalidFiles();

    // 异步初始化
    void initializeAsync();

signals:
    void recentFilesChanged();
    void recentFileAdded(const QString& filePath);
    void recentFileRemoved(const QString& filePath);
    void recentFilesCleared();

private slots:
    void saveSettings();

private:
    void loadSettings();
    void loadSettingsWithoutCleanup();
    void enforceMaxSize();
    QVariantMap fileInfoToVariant(const RecentFileInfo& info) const;
    RecentFileInfo variantToFileInfo(const QVariantMap& variant) const;

    QSettings* m_settings;
    QList<RecentFileInfo> m_recentFiles;
    int m_maxRecentFiles;
    mutable QMutex m_mutex;

    static const int DEFAULT_MAX_RECENT_FILES = 10;
    static const QString SETTINGS_GROUP;
    static const QString SETTINGS_MAX_FILES_KEY;
    static const QString SETTINGS_FILES_KEY;
};
