#pragma once

#include <QHash>
#include <QObject>
#include "plugin/PluginInterface.h"

/**
 * @brief Reading session data
 */
struct ReadingSession {
    QString documentPath;
    QDateTime startTime;
    QDateTime endTime;
    int startPage;
    int endPage;
    int pagesRead;
    qint64 durationSeconds;
};

/**
 * @brief Document progress data
 */
struct DocumentProgress {
    QString documentPath;
    QString documentTitle;
    int totalPages;
    int lastReadPage;
    double percentComplete;
    qint64 totalReadingTime;
    QDateTime lastAccessed;
    QDateTime firstAccessed;
    QList<ReadingSession> sessions;
};

/**
 * @brief ReadingProgressPlugin - Reading progress tracking plugin
 *
 * This plugin demonstrates:
 * - **Progress Tracking**: Track pages read and completion percentage
 * - **Reading Sessions**: Record reading time and sessions
 * - **Statistics**: Reading speed, estimated completion time
 * - **History**: Recently read documents with progress
 * - **Persistence**: Save/load progress data
 */
class ReadingProgressPlugin : public PluginBase, public IUIExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "reading_progress.json")
    Q_INTERFACES(IPluginInterface IUIExtension)

public:
    explicit ReadingProgressPlugin(QObject* parent = nullptr);
    ~ReadingProgressPlugin() override;

    void handleMessage(const QString& from, const QVariant& message) override;

    // IUIExtension
    QList<QAction*> menuActions() const override;
    QList<QAction*> toolbarActions() const override { return {}; }
    QList<QAction*> contextMenuActions() const override { return {}; }
    QString statusBarMessage() const override;
    QWidget* createDockWidget() override;
    QString menuPath() const override { return "View"; }
    QString toolbarId() const override { return QString(); }

    // Progress API
    DocumentProgress getProgress(const QString& documentPath) const;
    QList<DocumentProgress> getRecentDocuments(int limit = 10) const;
    void markPageRead(const QString& documentPath, int pageNumber);
    void resetProgress(const QString& documentPath);
    double getReadingSpeed() const;  // pages per minute
    int estimateTimeToComplete(const QString& documentPath) const;

protected:
    bool onInitialize() override;
    void onShutdown() override;

private slots:
    void onShowProgress();
    void onShowHistory();

private:
    void registerHooks();
    void setupEventSubscriptions();
    void loadProgress();
    void saveProgress();
    void startSession(const QString& documentPath, int startPage);
    void endSession();
    void updateStatistics();

    QVariant onPageViewed(const QVariantMap& context);

    // Progress data
    QHash<QString, DocumentProgress> m_progress;
    ReadingSession m_currentSession;
    bool m_sessionActive;

    // Statistics
    double m_averageReadingSpeed;
    qint64 m_totalReadingTime;
    int m_totalPagesRead;

    // UI
    QList<QAction*> m_menuActions;

    // Config
    QString m_storageFile;
    int m_sessionTimeoutMinutes;
};
