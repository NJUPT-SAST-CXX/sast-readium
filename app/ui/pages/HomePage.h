#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>
#include "ElaScrollPage.h"

// Forward declarations
class ElaImageCard;
class ElaPopularCard;
class ElaAcrylicUrlCard;
class ElaText;
class ElaScrollArea;
class ElaScrollPageArea;
class ElaPushButton;
class ElaProgressRing;
class QVBoxLayout;
class QHBoxLayout;
class RecentFilesManager;
class WelcomeWidget;
class OnboardingWidget;
class OnboardingManager;
class WelcomeScreenManager;
class CommandManager;
class SkeletonWidget;

/**
 * @brief HomePage - 主页/欢迎页面
 *
 * 显示欢迎信息、最近文件、快速操作等
 * Inherits from ElaScrollPage following ElaWidgetTools example pattern
 *
 * Features:
 * - Application branding and introduction
 * - Quick action buttons (Open File, Recent Files, Settings)
 * - Recent files list with thumbnails (RecentFileListWidget)
 * - Application version information
 * - Links to GitHub and documentation
 * - Tutorial cards for new users
 * - Onboarding support for first-time users
 * - Loading skeleton for async content
 * - Daily tips and keyboard shortcuts
 */
class HomePage : public ElaScrollPage {
    Q_OBJECT

public:
    explicit HomePage(QWidget* parent = nullptr);
    ~HomePage() override;

    // Manager setters
    void setRecentFilesManager(RecentFilesManager* manager);
    void setOnboardingManager(OnboardingManager* manager);
    void setWelcomeScreenManager(WelcomeScreenManager* manager);
    void setCommandManager(CommandManager* manager);

    // Onboarding control
    void startOnboarding();
    void stopOnboarding();
    bool isOnboardingActive() const;

public slots:
    void refreshRecentFiles();

signals:
    void openFileRequested();
    void openRecentFileRequested(const QString& filePath);
    void showSettingsRequested();
    void showAboutRequested();
    void tutorialRequested(const QString& tutorialId);
    void showDocumentationRequested();
    void newFileRequested();
    void openFolderRequested();

protected:
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void initUI();
    void retranslateUi();
    void setupTitleSection();
    void setupQuickActionsSection();
    void setupRecentFilesSection();
    void setupInfoSection();
    void setupTutorialSection();
    void setupTipsSection();
    void initializeTips();
    void setupWelcomeWidget();
    void createRecentFileItem(const QString& filePath, const QString& fileName,
                              const QDateTime& lastOpened);
    void clearRecentFilesList();
    void updateEmptyRecentFilesState();
    void showLoadingSkeleton();
    void hideLoadingSkeleton();

private slots:
    void onRecentFileClicked(const QString& filePath);
    void onClearRecentFilesClicked();

private:
    // Managers
    RecentFilesManager* m_recentFilesManager;
    OnboardingManager* m_onboardingManager;
    WelcomeScreenManager* m_welcomeScreenManager;
    CommandManager* m_commandManager;

    // Enhanced widgets
    WelcomeWidget* m_welcomeWidget;
    OnboardingWidget* m_onboardingWidget;
    SkeletonWidget* m_loadingSkeleton;

    // UI Components - Title Section
    ElaImageCard* m_backgroundCard;
    ElaText* m_titleText;
    ElaText* m_subtitleText;
    ElaAcrylicUrlCard* m_githubCard;
    ElaAcrylicUrlCard* m_documentationCard;
    ElaScrollArea* m_urlScrollArea;

    // UI Components - Quick Actions Section
    ElaText* m_quickActionsTitle;
    ElaPopularCard* m_openFileCard;
    ElaPopularCard* m_recentFilesCard;
    ElaPopularCard* m_settingsCard;

    // UI Components - Recent Files Section
    ElaText* m_recentFilesTitle;
    ElaScrollPageArea* m_recentFilesContainer;
    QVBoxLayout* m_recentFilesLayout;
    ElaText* m_emptyRecentFilesLabel;
    ElaPushButton* m_clearRecentFilesButton;
    QWidget* m_recentFilesListWidget;
    QVBoxLayout* m_recentFilesListLayout;

    // UI Components - Info Section
    ElaScrollPageArea* m_infoContainer;
    ElaText* m_versionText;
    ElaText* m_copyrightText;

    // UI Components - Tutorial Section
    ElaText* m_tutorialTitle;
    QWidget* m_tutorialContainer;
    QHBoxLayout* m_tutorialLayout;

    // UI Components - Tips Section
    ElaScrollPageArea* m_tipsContainer;
    ElaText* m_tipsTitle;
    ElaText* m_currentTipLabel;
    ElaPushButton* m_nextTipButton;
    int m_currentTipIndex;
    QStringList m_tips;

    // State
    bool m_isInitialized;
    bool m_useEnhancedWelcome;  // Toggle between simple and enhanced welcome
};

#endif  // HOMEPAGE_H
