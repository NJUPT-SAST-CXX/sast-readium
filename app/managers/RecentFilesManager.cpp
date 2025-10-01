#include "RecentFilesManager.h"
#include <QDir>
#include "../logging/Logger.h"
#include <QFileInfo>
#include <QMutexLocker>
#include <QTimer>
#include <QString>
#include <QObject>
#include <QSettings>
#include <QList>
#include <QStringList>
#include <QtCore/QObject>
#include <QMutex>
#include <algorithm>

// Private implementation class
class RecentFilesManagerImpl {
public:
    RecentFilesManagerImpl()
        : m_settings(nullptr)
        , m_maxRecentFiles(DEFAULT_MAX_RECENT_FILES)
    {
    }

    void loadSettings();
    void loadSettingsWithoutCleanup();
    void saveSettings();
    void enforceMaxSize();
    QVariantMap fileInfoToVariant(const RecentFileInfo& info) const;
    RecentFileInfo variantToFileInfo(const QVariantMap& variant) const;

    QSettings* m_settings;
    QList<RecentFileInfo> m_recentFiles;
    int m_maxRecentFiles;
    mutable QMutex m_mutex;

    static const int DEFAULT_MAX_RECENT_FILES = 10;
};

// Static constant definitions
const QString RecentFilesManager::SETTINGS_GROUP = "recentFiles";
const QString RecentFilesManager::SETTINGS_MAX_FILES_KEY = "maxFiles";
const QString RecentFilesManager::SETTINGS_FILES_KEY = "files";

RecentFilesManager::RecentFilesManager(QObject* parent)
    : QObject(parent)
    , pImpl(std::make_unique<RecentFilesManagerImpl>())
{
    // 初始化设置
    pImpl->m_settings = new QSettings("SAST", "Readium-RecentFiles", this);

    // 加载配置 (不执行文件清理以避免阻塞)
    pImpl->loadSettingsWithoutCleanup();

    Logger::instance().debug("RecentFilesManager: Initialized with max files: {}", pImpl->m_maxRecentFiles);
}

RecentFilesManager::~RecentFilesManager() { saveSettings(); }

void RecentFilesManager::addRecentFile(const QString& filePath) {
    if (filePath.isEmpty()) {
        return;
    }

    QMutexLocker locker(&pImpl->m_mutex);

    // 创建文件信息
    RecentFileInfo newFile(filePath);
    if (!newFile.isValid()) {
        Logger::instance().warning("[managers] File does not exist: {}", filePath.toStdString());
        return;
    }

    // 移除已存在的相同文件
    auto it = std::find_if(pImpl->m_recentFiles.begin(), pImpl->m_recentFiles.end(),
                           [&filePath](const RecentFileInfo& info) {
                               return info.filePath == filePath;
                           });
    if (it != pImpl->m_recentFiles.end()) {
        pImpl->m_recentFiles.erase(it);
    }

    // 添加到列表开头
    pImpl->m_recentFiles.prepend(newFile);

    // 强制执行最大数量限制
    pImpl->enforceMaxSize();

    // 保存设置
    saveSettings();

    emit recentFileAdded(filePath);
    emit recentFilesChanged();

    Logger::instance().info("[managers] Added recent file: {}", filePath.toStdString());
}

QList<RecentFileInfo> RecentFilesManager::getRecentFiles() const {
    QMutexLocker locker(&pImpl->m_mutex);
    return pImpl->m_recentFiles;
}

QStringList RecentFilesManager::getRecentFilePaths() const {
    QMutexLocker locker(&pImpl->m_mutex);
    QStringList paths;
    for (const RecentFileInfo& info : pImpl->m_recentFiles) {
        if (info.isValid()) {
            paths.append(info.filePath);
        }
    }
    return paths;
}

void RecentFilesManager::clearRecentFiles() {
    QMutexLocker locker(&pImpl->m_mutex);

    if (pImpl->m_recentFiles.isEmpty()) {
        return;
    }

    pImpl->m_recentFiles.clear();
    saveSettings();

    emit recentFilesCleared();
    emit recentFilesChanged();

    Logger::instance().info("[managers] Cleared all recent files");
}

void RecentFilesManager::removeRecentFile(const QString& filePath) {
    QMutexLocker locker(&pImpl->m_mutex);

    auto it = std::find_if(pImpl->m_recentFiles.begin(), pImpl->m_recentFiles.end(),
                           [&filePath](const RecentFileInfo& info) {
                               return info.filePath == filePath;
                           });

    if (it != pImpl->m_recentFiles.end()) {
        pImpl->m_recentFiles.erase(it);
        saveSettings();

        emit recentFileRemoved(filePath);
        emit recentFilesChanged();

        Logger::instance().info("[managers] Removed recent file: {}", filePath.toStdString());
    }
}

void RecentFilesManager::setMaxRecentFiles(int maxFiles) {
    if (maxFiles < 1 || maxFiles > 50) {
        Logger::instance().warning("[managers] Invalid max files count: {}", maxFiles);
        return;
    }

    QMutexLocker locker(&pImpl->m_mutex);

    if (pImpl->m_maxRecentFiles != maxFiles) {
        pImpl->m_maxRecentFiles = maxFiles;
        pImpl->enforceMaxSize();
        saveSettings();

        emit recentFilesChanged();

        Logger::instance().info("[managers] Max recent files changed to: {}", maxFiles);
    }
}

int RecentFilesManager::getMaxRecentFiles() const {
    QMutexLocker locker(&pImpl->m_mutex);
    return pImpl->m_maxRecentFiles;
}

bool RecentFilesManager::hasRecentFiles() const {
    QMutexLocker locker(&pImpl->m_mutex);
    return !pImpl->m_recentFiles.isEmpty();
}

int RecentFilesManager::getRecentFilesCount() const {
    QMutexLocker locker(&pImpl->m_mutex);
    return pImpl->m_recentFiles.size();
}

void RecentFilesManager::cleanupInvalidFiles() {
    QMutexLocker locker(&pImpl->m_mutex);

    bool changed = false;
    auto it = pImpl->m_recentFiles.begin();
    while (it != pImpl->m_recentFiles.end()) {
        if (!it->isValid()) {
            Logger::instance().debug("[managers] Removing invalid file: {}", it->filePath.toStdString());
            it = pImpl->m_recentFiles.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }

    if (changed) {
        saveSettings();
        emit recentFilesChanged();
    }
}

void RecentFilesManager::initializeAsync() {
    // 使用 QTimer::singleShot 在下一个事件循环中异步执行清理
    QTimer::singleShot(100, this, [this]() {
        try {
            Logger::instance().debug("[managers] Starting async cleanup");

            // 检查对象是否仍然有效
            if (!pImpl->m_settings) {
                Logger::instance().warning("[managers] Settings object is null during async cleanup");
                return;
            }

            cleanupInvalidFiles();
            Logger::instance().debug("[managers] Async cleanup completed successfully");

        } catch (const std::exception& e) {
            Logger::instance().error("[managers] Exception during async cleanup: {}", e.what());
        } catch (...) {
            Logger::instance().error("[managers] Unknown exception during async cleanup");
        }
    });
}

void RecentFilesManager::loadSettings() {
    pImpl->loadSettingsWithoutCleanup();

    // 清理无效文件
    cleanupInvalidFiles();

    Logger::instance().info("[managers] Loaded and cleaned {} recent files", pImpl->m_recentFiles.size());
}

void RecentFilesManagerImpl::loadSettingsWithoutCleanup() {
    if (!m_settings)
        return;

    QMutexLocker locker(&m_mutex);

    m_settings->beginGroup(RecentFilesManager::SETTINGS_GROUP);

    // 加载最大文件数量
    m_maxRecentFiles =
        m_settings->value(RecentFilesManager::SETTINGS_MAX_FILES_KEY, DEFAULT_MAX_RECENT_FILES)
            .toInt();

    // 加载文件列表
    int size = m_settings->beginReadArray(RecentFilesManager::SETTINGS_FILES_KEY);
    m_recentFiles.clear();
    m_recentFiles.reserve(size);

    int validCount = 0;
    for (int i = 0; i < size; ++i) {
        m_settings->setArrayIndex(i);
        QVariantMap data = m_settings->value("fileInfo").toMap();
        if (!data.isEmpty()) {
            RecentFileInfo info = variantToFileInfo(data);
            // Only add valid file info (variantToFileInfo now validates data)
            if (!info.filePath.isEmpty() && !info.fileName.isEmpty()) {
                m_recentFiles.append(info);
                validCount++;
            } else {
                Logger::instance().warning("[managers] Skipping invalid file entry at index {}", i);
            }
        }
    }

    m_settings->endArray();
    m_settings->endGroup();

    Logger::instance().debug("[managers] Loaded {} valid recent files out of {} total entries (without cleanup)",
              validCount, size);
}

void RecentFilesManager::saveSettings() {
    pImpl->saveSettings();
}

void RecentFilesManagerImpl::saveSettings() {
    if (!m_settings)
        return;

    // 注意：这里不需要加锁，因为调用此方法的地方已经加锁了

    m_settings->beginGroup(RecentFilesManager::SETTINGS_GROUP);

    // 保存最大文件数量
    m_settings->setValue(RecentFilesManager::SETTINGS_MAX_FILES_KEY, m_maxRecentFiles);

    // 保存文件列表
    m_settings->beginWriteArray(RecentFilesManager::SETTINGS_FILES_KEY);
    for (int i = 0; i < m_recentFiles.size(); ++i) {
        m_settings->setArrayIndex(i);
        QVariantMap data = fileInfoToVariant(m_recentFiles[i]);
        m_settings->setValue("fileInfo", data);
    }
    m_settings->endArray();

    m_settings->endGroup();
    m_settings->sync();
}

// Implementation methods moved to RecentFilesManagerImpl class
void RecentFilesManagerImpl::enforceMaxSize() {
    // 注意：调用此方法时应该已经加锁
    while (m_recentFiles.size() > m_maxRecentFiles) {
        m_recentFiles.removeLast();
    }
}

QVariantMap RecentFilesManagerImpl::fileInfoToVariant(
    const RecentFileInfo& info) const {
    QVariantMap data;
    data["filePath"] = info.filePath;
    data["fileName"] = info.fileName;
    data["lastOpened"] = info.lastOpened;
    data["fileSize"] = static_cast<qint64>(info.fileSize);
    return data;
}

RecentFileInfo RecentFilesManagerImpl::variantToFileInfo(
    const QVariantMap& variant) const {
    RecentFileInfo info;
    info.filePath = variant["filePath"].toString();
    info.fileName = variant["fileName"].toString();
    info.lastOpened = variant["lastOpened"].toDateTime();
    info.fileSize = variant["fileSize"].toLongLong();

    // Validate and fix corrupted data
    if (info.filePath.isEmpty() || info.fileName.isEmpty()) {
        Logger::instance().warning("[managers] Invalid file info detected, skipping");
        return RecentFileInfo(); // Return empty/invalid info
    }

    // Ensure fileName is properly extracted from filePath if missing
    if (info.fileName.isEmpty() && !info.filePath.isEmpty()) {
        QFileInfo fileInfo(info.filePath);
        info.fileName = fileInfo.fileName();
    }

    // Validate lastOpened date
    if (!info.lastOpened.isValid()) {
        info.lastOpened = QDateTime::currentDateTime();
    }

    return info;
}
