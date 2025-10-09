#pragma once

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QFileDialog>
#include <QFont>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMutex>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <deque>
#include <memory>
#include "../../utils/Logger.h"
#include "../../utils/LoggingManager.h"

/**
 * @brief Comprehensive debug logging panel widget
 *
 * This widget provides a complete debug logging interface with:
 * - Real-time log message display
 * - Filtering by log level and category
 * - Search functionality with highlighting
 * - Log export capabilities
 * - Statistics display
 * - Configuration options
 */
class DebugLogPanel : public QWidget {
    Q_OBJECT

public:
    struct LogEntry {
        QDateTime timestamp;
        Logger::LogLevel level;
        QString category;
        QString message;
        QString threadId;
        QString sourceLocation;

        LogEntry() = default;
        LogEntry(const QDateTime& ts, Logger::LogLevel lvl, const QString& cat,
                 const QString& msg, const QString& thread = "",
                 const QString& source = "")
            : timestamp(ts),
              level(lvl),
              category(cat),
              message(msg),
              threadId(thread),
              sourceLocation(source) {}
    };

    struct PanelConfiguration {
        int maxLogEntries = 10000;
        bool autoScroll = true;
        bool showTimestamp = true;
        bool showLevel = true;
        bool showCategory = true;
        bool showThreadId = false;
        bool showSourceLocation = false;
        bool wordWrap = true;
        bool colorizeOutput = true;
        QString timestampFormat = "hh:mm:ss.zzz";
        QFont logFont = QFont("Consolas", 9);

        // Filter settings
        Logger::LogLevel minLogLevel = Logger::LogLevel::Debug;
        QStringList enabledCategories;
        QString searchFilter;
        bool caseSensitiveSearch = false;
        bool regexSearch = false;

        // Performance settings
        int updateIntervalMs = 100;
        int batchSize = 50;
        bool pauseOnHighFrequency = true;
        int highFrequencyThreshold = 1000;  // messages per second
    };

    explicit DebugLogPanel(QWidget* parent = nullptr);
    ~DebugLogPanel();

    // Configuration management
    void setConfiguration(const PanelConfiguration& config);
    const PanelConfiguration& getConfiguration() const { return m_config; }
    void saveConfiguration();
    void loadConfiguration();
    void resetToDefaults();

    // Log management
    void clearLogs();
    void pauseLogging(bool pause);
    bool isLoggingPaused() const { return m_paused; }

    // Statistics
    struct LogStatistics {
        int totalMessages = 0;
        int debugMessages = 0;
        int infoMessages = 0;
        int warningMessages = 0;
        int errorMessages = 0;
        int criticalMessages = 0;
        int filteredMessages = 0;
        QDateTime firstLogTime;
        QDateTime lastLogTime;
        double messagesPerSecond = 0.0;
    };

    LogStatistics getStatistics() const;
    void resetStatistics();

public slots:
    void onLogMessage(const QString& message, int level);
    void onLogMessageDetailed(const QDateTime& timestamp, int level,
                              const QString& category, const QString& message,
                              const QString& threadId = "",
                              const QString& sourceLocation = "");
    void showPanel();
    void hidePanel();
    void togglePanel();

private slots:
    void onFilterChanged();
    void onSearchTextChanged();
    void onSearchNext();
    void onSearchPrevious();
    void onClearLogs();
    void onExportLogs();
    void onCopySelected();
    void onCopyAll();
    void onPauseToggled(bool paused);
    void onAutoScrollToggled(bool enabled);
    void onConfigurationChanged();
    void onUpdateTimer();
    void onLogLevelFilterChanged();
    void onCategoryFilterChanged();
    void onShowSettingsDialog();
    void onContextMenuRequested(const QPoint& pos);

signals:
    void panelVisibilityChanged(bool visible);
    void configurationChanged();
    void logStatisticsUpdated(const LogStatistics& stats);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void setupUI();
    void setupLogDisplay();
    void setupFilterControls();
    void setupActionButtons();
    void setupStatisticsDisplay();
    void setupContextMenu();
    void connectSignals();
    void applyConfiguration();
    void updateLogDisplay();
    void addLogEntryToDisplay(const LogEntry& entry);
    void filterLogEntries();
    void highlightSearchResults();
    void updateStatistics();
    void updateStatisticsDisplay();
    QString formatLogEntry(const LogEntry& entry) const;
    QColor getLogLevelColor(Logger::LogLevel level) const;
    QString getLogLevelText(Logger::LogLevel level) const;
    bool passesFilter(const LogEntry& entry) const;
    void scrollToBottom();
    void exportToFile(const QString& filePath);
    void showSettingsDialog();
    void applyTheme();

    // UI Components
    QVBoxLayout* m_mainLayout;
    QSplitter* m_mainSplitter;

    // Log display area
    QTextEdit* m_logDisplay;
    QScrollBar* m_scrollBar;

    // Filter controls
    QGroupBox* m_filterGroup;
    QComboBox* m_logLevelFilter;
    QComboBox* m_categoryFilter;
    QLineEdit* m_searchEdit;
    QPushButton* m_searchNextBtn;
    QPushButton* m_searchPrevBtn;
    QCheckBox* m_caseSensitiveCheck;
    QCheckBox* m_regexCheck;

    // Action buttons
    QHBoxLayout* m_actionLayout;
    QPushButton* m_clearBtn;
    QPushButton* m_exportBtn;
    QPushButton* m_copyBtn;
    QPushButton* m_pauseBtn;
    QPushButton* m_settingsBtn;
    QCheckBox* m_autoScrollCheck;

    // Statistics display
    QGroupBox* m_statsGroup;
    QTableWidget* m_statsTable;
    QLabel* m_messagesPerSecLabel;
    QProgressBar* m_memoryUsageBar;

    // Context menu
    QMenu* m_contextMenu;
    QAction* m_copyAction;
    QAction* m_copyAllAction;
    QAction* m_clearAction;
    QAction* m_exportAction;
    QAction* m_pauseAction;

    // Data management
    std::deque<LogEntry> m_logEntries;
    std::deque<LogEntry> m_filteredEntries;
    mutable QMutex m_logMutex;
    QTimer* m_updateTimer;
    QTimer* m_statisticsTimer;

    // Configuration and state
    PanelConfiguration m_config;
    LogStatistics m_statistics;
    bool m_paused;
    bool m_autoScroll;
    int m_currentSearchIndex;
    QStringList m_searchResults;

    // Performance tracking
    QDateTime m_lastUpdateTime;
    int m_pendingMessages;
    std::deque<LogEntry> m_pendingEntries;

    // Settings
    QSettings* m_settings;

    static const QString SETTINGS_GROUP;
    static const int DEFAULT_MAX_ENTRIES;
    static const int UPDATE_INTERVAL_MS;
    static const int STATISTICS_UPDATE_INTERVAL_MS;
};
