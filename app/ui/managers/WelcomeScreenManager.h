#pragma once

#include <QObject>
#include <QSettings>
#include <QTimer>

class MainWindow;
class WelcomeWidget;
class DocumentModel;

/**
 * 欢迎界面管理器
 * 负责管理欢迎界面的显示状态、设置持久化和应用程序生命周期集成
 */
class WelcomeScreenManager : public QObject {
    Q_OBJECT

public:
    explicit WelcomeScreenManager(QObject* parent = nullptr);
    ~WelcomeScreenManager();

    // 设置相关组件
    void setMainWindow(MainWindow* mainWindow);
    void setWelcomeWidget(WelcomeWidget* welcomeWidget);
    void setDocumentModel(DocumentModel* documentModel);

    // 欢迎界面控制
    bool isWelcomeScreenEnabled() const;
    void setWelcomeScreenEnabled(bool enabled);

    bool shouldShowWelcomeScreen() const;
    void showWelcomeScreen();
    void hideWelcomeScreen();

    // 状态查询
    bool isWelcomeScreenVisible() const;
    bool hasOpenDocuments() const;

    // 设置管理
    void loadSettings();
    void saveSettings();
    void resetToDefaults();

    // 应用程序生命周期集成
    void onApplicationStartup();
    void onApplicationShutdown();
    void onDocumentOpened();
    void onDocumentClosed();
    void onAllDocumentsClosed();

public slots:
    void onDocumentModelChanged();
    void onWelcomeScreenToggleRequested();
    void checkWelcomeScreenVisibility();

signals:
    void welcomeScreenVisibilityChanged(bool visible);
    void welcomeScreenEnabledChanged(bool enabled);
    void showWelcomeScreenRequested();
    void hideWelcomeScreenRequested();

private slots:
    void onDelayedVisibilityCheck();

private:
    void initializeSettings();
    void setupConnections();
    void updateWelcomeScreenVisibility();
    void scheduleVisibilityCheck();

    // 组件引用
    MainWindow* m_mainWindow;
    WelcomeWidget* m_welcomeWidget;
    DocumentModel* m_documentModel;

    // 设置
    QSettings* m_settings;

    // 状态
    bool m_welcomeScreenEnabled;
    bool m_welcomeScreenVisible;
    bool m_isInitialized;

    // 延迟检查定时器
    QTimer* m_visibilityCheckTimer;

    // 设置键
    static const QString SETTINGS_GROUP;
    static const QString SETTINGS_ENABLED_KEY;
    static const QString SETTINGS_SHOW_ON_STARTUP_KEY;

    // 默认值
    static const bool DEFAULT_ENABLED = true;
    static const bool DEFAULT_SHOW_ON_STARTUP = true;
    static const int VISIBILITY_CHECK_DELAY = 100;  // ms
};
