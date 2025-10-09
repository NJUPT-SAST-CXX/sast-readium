#pragma once

#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class RecentFilesManager;
class RecentFileListWidget;
class WelcomeScreenManager;

/**
 * VSCode风格的欢迎界面组件
 * 显示应用程序logo和最近打开的文件列表
 */
class WelcomeWidget : public QWidget {
    Q_OBJECT

public:
    explicit WelcomeWidget(QWidget* parent = nullptr);
    ~WelcomeWidget();

    // 设置管理器
    void setRecentFilesManager(RecentFilesManager* manager);
    void setWelcomeScreenManager(WelcomeScreenManager* manager);

    // 主题支持
    void applyTheme();

    // 刷新内容
    void refreshContent();

public slots:
    void onRecentFilesChanged();
    void onThemeChanged();

signals:
    void fileOpenRequested(const QString& filePath);
    void newFileRequested();
    void openFileRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onNewFileClicked();
    void onOpenFileClicked();
    void onRecentFileClicked(const QString& filePath);
    void onFadeInFinished();

private:
    void initializeUI();
    void setupLayout();
    void setupLogo();
    void setupActions();
    void setupRecentFiles();
    void setupConnections();
    void updateLayout();
    void updateLogo();
    void startFadeInAnimation();

    // UI组件
    QVBoxLayout* m_mainLayout;
    QWidget* m_contentWidget;
    QScrollArea* m_scrollArea;

    // Logo区域
    QWidget* m_logoWidget;
    QVBoxLayout* m_logoLayout;
    QLabel* m_logoLabel;
    QLabel* m_titleLabel;
    QLabel* m_versionLabel;

    // 操作区域
    QWidget* m_actionsWidget;
    QHBoxLayout* m_actionsLayout;
    QPushButton* m_newFileButton;
    QPushButton* m_openFileButton;

    // 最近文件区域
    QWidget* m_recentFilesWidget;
    QVBoxLayout* m_recentFilesLayout;
    QLabel* m_recentFilesTitle;
    RecentFileListWidget* m_recentFilesList;
    QLabel* m_noRecentFilesLabel;

    // 分隔线
    QFrame* m_separatorLine;

    // 管理器
    RecentFilesManager* m_recentFilesManager;
    WelcomeScreenManager* m_welcomeScreenManager;

    // 动画效果
    QGraphicsOpacityEffect* m_opacityEffect;
    QPropertyAnimation* m_fadeAnimation;
    QTimer* m_refreshTimer;

    // 状态
    bool m_isInitialized;
    bool m_isVisible;

    // Enhanced 样式常量 for improved typography and spacing
    static const int LOGO_SIZE = 80;  // Larger logo for better presence
    static const int CONTENT_MAX_WIDTH = 900;  // Wider content area
    static const int SPACING_XLARGE =
        48;  // Extra large spacing for major sections
    static const int SPACING_LARGE = 32;   // Large spacing between components
    static const int SPACING_MEDIUM = 20;  // Medium spacing within components
    static const int SPACING_SMALL = 12;   // Small spacing for related elements
    static const int SPACING_XSMALL =
        8;  // Extra small spacing for tight elements
};
