#include "HomePage.h"

// ElaWidgetTools
#include "ElaAcrylicUrlCard.h"
#include "ElaFlowLayout.h"
#include "ElaImageCard.h"
#include "ElaPopularCard.h"
#include "ElaProgressRing.h"
#include "ElaPushButton.h"
#include "ElaScrollArea.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"

// Enhanced widgets
#include "ui/widgets/OnboardingWidget.h"
#include "ui/widgets/SkeletonWidget.h"
#include "ui/widgets/TutorialCard.h"
#include "ui/widgets/WelcomeWidget.h"

// Business Logic
#include "config.h"  // For PROJECT_VER
#include "managers/OnboardingManager.h"
#include "managers/RecentFilesManager.h"
#include "ui/managers/WelcomeScreenManager.h"

// Qt
#include <QDesktopServices>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QUrl>
#include <QVBoxLayout>

#include <QCursor>
#include <QDateTime>
#include <QFileInfo>
#include <QFrame>
#include <QMouseEvent>
#include <QTimer>

HomePage::HomePage(QWidget* parent)
    : ElaScrollPage(parent),
      m_recentFilesManager(nullptr),
      m_onboardingManager(nullptr),
      m_welcomeScreenManager(nullptr),
      m_commandManager(nullptr),
      m_welcomeWidget(nullptr),
      m_onboardingWidget(nullptr),
      m_loadingSkeleton(nullptr),
      m_backgroundCard(nullptr),
      m_titleText(nullptr),
      m_subtitleText(nullptr),
      m_githubCard(nullptr),
      m_documentationCard(nullptr),
      m_urlScrollArea(nullptr),
      m_quickActionsTitle(nullptr),
      m_openFileCard(nullptr),
      m_recentFilesCard(nullptr),
      m_settingsCard(nullptr),
      m_recentFilesTitle(nullptr),
      m_recentFilesContainer(nullptr),
      m_recentFilesLayout(nullptr),
      m_emptyRecentFilesLabel(nullptr),
      m_clearRecentFilesButton(nullptr),
      m_recentFilesListWidget(nullptr),
      m_recentFilesListLayout(nullptr),
      m_infoContainer(nullptr),
      m_versionText(nullptr),
      m_copyrightText(nullptr),
      m_tutorialTitle(nullptr),
      m_tutorialContainer(nullptr),
      m_tutorialLayout(nullptr),
      m_tipsContainer(nullptr),
      m_tipsTitle(nullptr),
      m_currentTipLabel(nullptr),
      m_nextTipButton(nullptr),
      m_currentTipIndex(0),
      m_isInitialized(false),
      m_useEnhancedWelcome(false) {
    // Set window title for navigation
    setWindowTitle(tr("Home"));

    // Set title visibility (following example pattern)
    setTitleVisible(false);

    // Set margins following example pattern
    setContentsMargins(2, 2, 0, 0);

    initUI();
    m_isInitialized = true;
}

HomePage::~HomePage() {}

void HomePage::initUI() {
    // Create main content widget
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(25);

    // Setup all sections
    setupTitleSection();
    setupQuickActionsSection();
    setupRecentFilesSection();
    setupInfoSection();

    // Add sections to main layout
    mainLayout->addWidget(m_backgroundCard);

    // Quick Actions Section
    QHBoxLayout* quickActionsTextLayout = new QHBoxLayout();
    quickActionsTextLayout->setContentsMargins(33, 0, 0, 0);
    quickActionsTextLayout->addWidget(m_quickActionsTitle);
    mainLayout->addLayout(quickActionsTextLayout);

    ElaFlowLayout* flowLayout = new ElaFlowLayout(0, 15, 15);
    flowLayout->setContentsMargins(33, 0, 33, 0);
    flowLayout->addWidget(m_openFileCard);
    flowLayout->addWidget(m_recentFilesCard);
    flowLayout->addWidget(m_settingsCard);
    mainLayout->addLayout(flowLayout);

    // Recent Files Section
    QHBoxLayout* recentFilesTitleLayout = new QHBoxLayout();
    recentFilesTitleLayout->setContentsMargins(33, 0, 33, 0);
    recentFilesTitleLayout->addWidget(m_recentFilesTitle);
    recentFilesTitleLayout->addStretch();
    recentFilesTitleLayout->addWidget(m_clearRecentFilesButton);
    mainLayout->addLayout(recentFilesTitleLayout);
    mainLayout->addWidget(m_recentFilesContainer);

    // Info Section
    mainLayout->addWidget(m_infoContainer);

    // Tutorial Section
    setupTutorialSection();
    QHBoxLayout* tutorialTitleLayout = new QHBoxLayout();
    tutorialTitleLayout->setContentsMargins(33, 0, 0, 0);
    tutorialTitleLayout->addWidget(m_tutorialTitle);
    mainLayout->addLayout(tutorialTitleLayout);
    mainLayout->addWidget(m_tutorialContainer);

    // Tips Section
    setupTipsSection();
    mainLayout->addWidget(m_tipsContainer);

    mainLayout->addStretch();

    // Add central widget using ElaScrollPage method (following example pattern)
    addCentralWidget(centralWidget, true, true, 0.5);

    // Initialize tips
    initializeTips();

    retranslateUi();
}

void HomePage::setupTutorialSection() {
    m_tutorialTitle = new ElaText(tr("Getting Started"), this);
    m_tutorialTitle->setTextPixelSize(20);

    m_tutorialContainer = new QWidget(this);
    m_tutorialLayout = new QHBoxLayout(m_tutorialContainer);
    m_tutorialLayout->setContentsMargins(33, 0, 33, 0);
    m_tutorialLayout->setSpacing(15);

    // Create tutorial cards
    auto* openFileTutorial =
        new TutorialCard("open_file", tr("Opening Documents"),
                         tr("Learn how to open and navigate PDF documents"),
                         QIcon(":/icons/open_file"), this);
    openFileTutorial->setDuration(tr("2 min"));
    openFileTutorial->setDifficulty(tr("Beginner"));
    connect(openFileTutorial, &TutorialCard::clicked, this,
            &HomePage::tutorialRequested);

    auto* annotationTutorial = new TutorialCard(
        "annotations", tr("Annotations & Highlights"),
        tr("Add notes, highlights, and bookmarks to your documents"),
        QIcon(":/icons/annotation"), this);
    annotationTutorial->setDuration(tr("5 min"));
    annotationTutorial->setDifficulty(tr("Intermediate"));
    connect(annotationTutorial, &TutorialCard::clicked, this,
            &HomePage::tutorialRequested);

    auto* searchTutorial = new TutorialCard(
        "search", tr("Search & Navigation"),
        tr("Find text and navigate efficiently through documents"),
        QIcon(":/icons/search"), this);
    searchTutorial->setDuration(tr("3 min"));
    searchTutorial->setDifficulty(tr("Beginner"));
    connect(searchTutorial, &TutorialCard::clicked, this,
            &HomePage::tutorialRequested);

    m_tutorialLayout->addWidget(openFileTutorial);
    m_tutorialLayout->addWidget(annotationTutorial);
    m_tutorialLayout->addWidget(searchTutorial);
    m_tutorialLayout->addStretch();
}

void HomePage::setupTipsSection() {
    m_tipsContainer = new ElaScrollPageArea(this);
    m_tipsContainer->setFixedHeight(100);
    m_tipsContainer->setBorderRadius(8);

    auto* tipsLayout = new QVBoxLayout(m_tipsContainer);
    tipsLayout->setContentsMargins(20, 15, 20, 15);
    tipsLayout->setSpacing(8);

    auto* headerLayout = new QHBoxLayout();
    m_tipsTitle = new ElaText(tr("💡 Tip of the Day"), this);
    m_tipsTitle->setTextPixelSize(14);
    headerLayout->addWidget(m_tipsTitle);
    headerLayout->addStretch();

    m_nextTipButton = new ElaPushButton(tr("Next Tip"), this);
    m_nextTipButton->setFixedSize(80, 28);
    connect(m_nextTipButton, &ElaPushButton::clicked, this, [this]() {
        m_currentTipIndex = (m_currentTipIndex + 1) % m_tips.size();
        if (m_currentTipLabel && !m_tips.isEmpty()) {
            m_currentTipLabel->setText(m_tips[m_currentTipIndex]);
        }
    });
    headerLayout->addWidget(m_nextTipButton);

    tipsLayout->addLayout(headerLayout);

    m_currentTipLabel = new ElaText("", this);
    m_currentTipLabel->setTextPixelSize(13);
    m_currentTipLabel->setWordWrap(true);
    tipsLayout->addWidget(m_currentTipLabel);
    tipsLayout->addStretch();
}

void HomePage::initializeTips() {
    m_tips = {tr("Press Ctrl+O to quickly open a PDF file."),
              tr("Use Ctrl+F to search for text in the current document."),
              tr("Press F11 to toggle full-screen mode for distraction-free "
                 "reading."),
              tr("Double-click on a page thumbnail to jump to that page."),
              tr("Use Ctrl+B to add a bookmark at the current page."),
              tr("Press Ctrl++ or Ctrl+- to zoom in and out."),
              tr("Enable Night Mode from the View menu for comfortable reading "
                 "in dark environments."),
              tr("Right-click on selected text to copy or highlight it."),
              tr("Use the outline panel on the left to navigate through "
                 "document sections."),
              tr("Press Ctrl+G to go to a specific page number.")};

    if (!m_tips.isEmpty() && m_currentTipLabel) {
        m_currentTipLabel->setText(m_tips[0]);
    }
}

void HomePage::setupWelcomeWidget() {
    // This method can be used to switch to enhanced WelcomeWidget mode
    if (m_useEnhancedWelcome && !m_welcomeWidget) {
        m_welcomeWidget = new WelcomeWidget(this);
        connect(m_welcomeWidget, &WelcomeWidget::fileOpenRequested, this,
                &HomePage::openRecentFileRequested);
        connect(m_welcomeWidget, &WelcomeWidget::openFileRequested, this,
                &HomePage::openFileRequested);
        connect(m_welcomeWidget, &WelcomeWidget::showSettingsRequested, this,
                &HomePage::showSettingsRequested);
        connect(m_welcomeWidget, &WelcomeWidget::tutorialRequested, this,
                &HomePage::tutorialRequested);
    }
}

void HomePage::showLoadingSkeleton() {
    if (!m_loadingSkeleton) {
        m_loadingSkeleton =
            new SkeletonWidget(SkeletonWidget::Shape::Rectangle, this);
        m_loadingSkeleton->setFixedHeight(200);
    }
    m_loadingSkeleton->startAnimation();
    m_loadingSkeleton->show();
}

void HomePage::hideLoadingSkeleton() {
    if (m_loadingSkeleton) {
        m_loadingSkeleton->stopAnimation();
        m_loadingSkeleton->hide();
    }
}

void HomePage::setupTitleSection() {
    // Title and subtitle
    m_subtitleText = new ElaText("Modern PDF Reader", this);
    m_subtitleText->setTextPixelSize(18);

    m_titleText = new ElaText("SAST Readium", this);
    m_titleText->setTextPixelSize(35);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setContentsMargins(30, 10, 0, 0);
    titleLayout->addWidget(m_subtitleText);
    titleLayout->addWidget(m_titleText);

    // Background card
    m_backgroundCard = new ElaImageCard(this);
    m_backgroundCard->setBorderRadius(10);
    m_backgroundCard->setFixedHeight(340);
    // Set background image from resources
    m_backgroundCard->setCardImage(QImage(":/images/home_background"));

    // URL cards (GitHub and Documentation)
    m_githubCard = new ElaAcrylicUrlCard(this);
    m_githubCard->setCardPixmapSize(QSize(62, 62));
    m_githubCard->setFixedSize(195, 225);
    m_githubCard->setTitlePixelSize(17);
    m_githubCard->setTitleSpacing(25);
    m_githubCard->setSubTitleSpacing(13);
    m_githubCard->setUrl("https://github.com/NJUPT-SAST/sast-readium");
    m_githubCard->setCardPixmap(QPixmap(":/icons/github"));
    m_githubCard->setTitle("GitHub Repository");
    m_githubCard->setSubTitle("View source code and contribute");

    m_documentationCard = new ElaAcrylicUrlCard(this);
    m_documentationCard->setCardPixmapSize(QSize(62, 62));
    m_documentationCard->setFixedSize(195, 225);
    m_documentationCard->setTitlePixelSize(17);
    m_documentationCard->setTitleSpacing(25);
    m_documentationCard->setSubTitleSpacing(13);
    m_documentationCard->setUrl("https://github.com/NJUPT-SAST/sast-readium");
    m_documentationCard->setCardPixmap(QPixmap(":/icons/documentation"));
    m_documentationCard->setTitle("Documentation");
    m_documentationCard->setSubTitle("Learn how to use SAST Readium");

    // URL cards scroll area
    m_urlScrollArea = new ElaScrollArea(this);
    m_urlScrollArea->setWidgetResizable(true);
    m_urlScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_urlScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_urlScrollArea->setIsGrabGesture(true, 0);
    m_urlScrollArea->setIsOverShoot(Qt::Horizontal, true);

    QWidget* scrollWidget = new QWidget(this);
    scrollWidget->setStyleSheet("background-color:transparent;");
    m_urlScrollArea->setWidget(scrollWidget);

    QHBoxLayout* urlCardLayout = new QHBoxLayout();
    urlCardLayout->setSpacing(15);
    urlCardLayout->setContentsMargins(30, 0, 0, 6);
    urlCardLayout->addWidget(m_githubCard);
    urlCardLayout->addWidget(m_documentationCard);
    urlCardLayout->addStretch();

    QVBoxLayout* scrollWidgetLayout = new QVBoxLayout(scrollWidget);
    scrollWidgetLayout->setContentsMargins(0, 0, 0, 0);
    scrollWidgetLayout->addStretch();
    scrollWidgetLayout->addLayout(urlCardLayout);

    // Combine title and URL cards in background card
    QVBoxLayout* backgroundLayout = new QVBoxLayout(m_backgroundCard);
    backgroundLayout->setContentsMargins(0, 0, 0, 0);
    backgroundLayout->addLayout(titleLayout);
    backgroundLayout->addWidget(m_urlScrollArea);
}

void HomePage::setupQuickActionsSection() {
    m_quickActionsTitle = new ElaText("Quick Actions", this);
    m_quickActionsTitle->setTextPixelSize(20);

    // Open File Card
    m_openFileCard = new ElaPopularCard(this);
    m_openFileCard->setTitle("Open PDF File");
    m_openFileCard->setSubTitle("5.0⭐ Quick Action");
    m_openFileCard->setCardPixmap(QPixmap(":/icons/open_file"));
    m_openFileCard->setInteractiveTips("Click to open");
    m_openFileCard->setDetailedText(
        "Open a PDF file from your computer to start reading");
    connect(m_openFileCard, &ElaPopularCard::popularCardButtonClicked, this,
            &HomePage::openFileRequested);

    // Recent Files Card
    m_recentFilesCard = new ElaPopularCard(this);
    m_recentFilesCard->setTitle("Recent Files");
    m_recentFilesCard->setSubTitle("5.0⭐ Quick Access");
    m_recentFilesCard->setCardPixmap(QPixmap(":/icons/recent_files"));
    m_recentFilesCard->setInteractiveTips("View recent");
    m_recentFilesCard->setDetailedText("Access your recently opened PDF files");
    connect(m_recentFilesCard, &ElaPopularCard::popularCardButtonClicked, this,
            [this]() {
                // Scroll to recent files section
                if (m_recentFilesContainer) {
                    m_recentFilesContainer->setFocus();
                }
            });

    // Settings Card
    m_settingsCard = new ElaPopularCard(this);
    m_settingsCard->setTitle("Settings");
    m_settingsCard->setSubTitle("5.0⭐ Configuration");
    m_settingsCard->setCardPixmap(QPixmap(":/icons/settings"));
    m_settingsCard->setInteractiveTips("Configure");
    m_settingsCard->setDetailedText(
        "Customize your reading experience and application settings");
    connect(m_settingsCard, &ElaPopularCard::popularCardButtonClicked, this,
            &HomePage::showSettingsRequested);
}

void HomePage::setupRecentFilesSection() {
    m_recentFilesTitle = new ElaText("Recent Files", this);
    m_recentFilesTitle->setTextPixelSize(20);

    // Clear recent files button
    m_clearRecentFilesButton = new ElaPushButton("Clear All", this);
    m_clearRecentFilesButton->setFixedSize(100, 35);
    connect(m_clearRecentFilesButton, &ElaPushButton::clicked, this,
            &HomePage::onClearRecentFilesClicked);

    // Recent files container
    m_recentFilesContainer = new ElaScrollPageArea(this);
    m_recentFilesContainer->setFixedHeight(300);
    m_recentFilesContainer->setBorderRadius(8);

    // Recent files list widget
    m_recentFilesListWidget = new QWidget(m_recentFilesContainer);
    m_recentFilesListLayout = new QVBoxLayout(m_recentFilesListWidget);
    m_recentFilesListLayout->setContentsMargins(15, 15, 15, 15);
    m_recentFilesListLayout->setSpacing(10);

    // Empty state label
    m_emptyRecentFilesLabel =
        new ElaText("No recent files", m_recentFilesListWidget);
    m_emptyRecentFilesLabel->setTextPixelSize(16);
    m_emptyRecentFilesLabel->setAlignment(Qt::AlignCenter);
    m_recentFilesListLayout->addWidget(m_emptyRecentFilesLabel);
    m_recentFilesListLayout->addStretch();

    // Set layout for container
    auto* containerLayout = new QVBoxLayout(m_recentFilesContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(m_recentFilesListWidget);
}

void HomePage::setupInfoSection() {
    m_infoContainer = new ElaScrollPageArea(this);
    m_infoContainer->setFixedHeight(100);
    m_infoContainer->setBorderRadius(8);

    auto* infoLayout = new QVBoxLayout(m_infoContainer);
    infoLayout->setContentsMargins(20, 15, 20, 15);
    infoLayout->setSpacing(8);

    // Version information
    m_versionText = new ElaText(QString("Version %1").arg(PROJECT_VER), this);
    m_versionText->setTextPixelSize(14);

    // Copyright information
    m_copyrightText =
        new ElaText("© 2024 SAST Team. All rights reserved.", this);
    m_copyrightText->setTextPixelSize(12);

    infoLayout->addWidget(m_versionText);
    infoLayout->addWidget(m_copyrightText);
    infoLayout->addStretch();
}

void HomePage::setRecentFilesManager(RecentFilesManager* manager) {
    m_recentFilesManager = manager;

    if (m_recentFilesManager) {
        // Connect to recent files changes
        connect(m_recentFilesManager, &RecentFilesManager::recentFilesChanged,
                this, &HomePage::refreshRecentFiles);

        // Initial load
        refreshRecentFiles();
    }
}

void HomePage::refreshRecentFiles() {
    if (!m_recentFilesManager || !m_isInitialized) {
        return;
    }

    clearRecentFilesList();

    QList<RecentFileInfo> recentFiles = m_recentFilesManager->getRecentFiles();

    if (recentFiles.isEmpty()) {
        updateEmptyRecentFilesState();
        return;
    }

    // Hide empty label
    if (m_emptyRecentFilesLabel) {
        m_emptyRecentFilesLabel->setVisible(false);
    }

    // Add recent file items (limit to 5 for home page)
    int maxItems = qMin(recentFiles.size(), 5);
    for (int i = 0; i < maxItems; ++i) {
        const RecentFileInfo& fileInfo = recentFiles[i];
        if (fileInfo.isValid()) {
            createRecentFileItem(fileInfo.filePath, fileInfo.fileName,
                                 fileInfo.lastOpened);
        }
    }
}

void HomePage::createRecentFileItem(const QString& filePath,
                                    const QString& fileName,
                                    const QDateTime& lastOpened) {
    // Create item widget
    auto* itemWidget = new QWidget(m_recentFilesListWidget);
    itemWidget->setFixedHeight(50);
    itemWidget->setCursor(Qt::PointingHandCursor);
    itemWidget->setProperty("filePath", filePath);

    auto* itemLayout = new QHBoxLayout(itemWidget);
    itemLayout->setContentsMargins(10, 5, 10, 5);
    itemLayout->setSpacing(10);

    // File icon
    auto* iconLabel = new QLabel(itemWidget);
    iconLabel->setFixedSize(32, 32);
    iconLabel->setPixmap(
        QPixmap(":/icons/pdf")
            .scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // File info
    auto* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);

    auto* nameLabel = new ElaText(fileName, itemWidget);
    nameLabel->setTextPixelSize(14);

    QString timeText;
    QDateTime now = QDateTime::currentDateTime();
    qint64 secondsAgo = lastOpened.secsTo(now);

    if (secondsAgo < 60) {
        timeText = tr("Just now");
    } else if (secondsAgo < 3600) {
        timeText = tr("%1 minutes ago").arg(secondsAgo / 60);
    } else if (secondsAgo < 86400) {
        timeText = tr("%1 hours ago").arg(secondsAgo / 3600);
    } else {
        timeText = lastOpened.toString("yyyy-MM-dd hh:mm");
    }

    auto* timeLabel = new ElaText(timeText, itemWidget);
    timeLabel->setTextPixelSize(11);

    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(timeLabel);

    itemLayout->addWidget(iconLabel);
    itemLayout->addLayout(infoLayout);
    itemLayout->addStretch();

    // Add to layout (before stretch)
    int insertIndex = m_recentFilesListLayout->count() - 1;
    m_recentFilesListLayout->insertWidget(insertIndex, itemWidget);

    // Make item clickable
    connect(itemWidget, &QWidget::destroyed, this, [this, filePath]() {
        // Cleanup if needed
    });

    // Use lambda to handle click
    itemWidget->setProperty(
        "clickHandler",
        QVariant::fromValue<std::function<void()>>(
            [this, filePath]() { onRecentFileClicked(filePath); }));
}

void HomePage::clearRecentFilesList() {
    if (!m_recentFilesListLayout) {
        return;
    }

    // Remove all items except empty label and stretch
    while (m_recentFilesListLayout->count() > 2) {
        QLayoutItem* item = m_recentFilesListLayout->takeAt(0);
        if (item->widget() && item->widget() != m_emptyRecentFilesLabel) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void HomePage::updateEmptyRecentFilesState() {
    if (m_emptyRecentFilesLabel) {
        m_emptyRecentFilesLabel->setVisible(true);
    }
}

void HomePage::onRecentFileClicked(const QString& filePath) {
    emit openRecentFileRequested(filePath);
}

void HomePage::onClearRecentFilesClicked() {
    if (m_recentFilesManager) {
        m_recentFilesManager->clearRecentFiles();
    }
}

void HomePage::setOnboardingManager(OnboardingManager* manager) {
    m_onboardingManager = manager;
    if (m_onboardingWidget && m_onboardingManager) {
        m_onboardingWidget->setOnboardingManager(m_onboardingManager);
    }
    if (m_welcomeWidget && m_onboardingManager) {
        m_welcomeWidget->setOnboardingManager(m_onboardingManager);
    }
}

void HomePage::setWelcomeScreenManager(WelcomeScreenManager* manager) {
    m_welcomeScreenManager = manager;
    if (m_welcomeWidget && m_welcomeScreenManager) {
        m_welcomeWidget->setWelcomeScreenManager(m_welcomeScreenManager);
    }
}

void HomePage::setCommandManager(CommandManager* manager) {
    m_commandManager = manager;
    if (m_welcomeWidget && m_commandManager) {
        m_welcomeWidget->setCommandManager(m_commandManager);
    }
}

void HomePage::startOnboarding() {
    if (!m_onboardingWidget) {
        m_onboardingWidget = new OnboardingWidget(this);
        if (m_onboardingManager) {
            m_onboardingWidget->setOnboardingManager(m_onboardingManager);
        }
    }
    m_onboardingWidget->show();
    m_onboardingWidget->raise();
}

void HomePage::stopOnboarding() {
    if (m_onboardingWidget) {
        m_onboardingWidget->hide();
    }
}

bool HomePage::isOnboardingActive() const {
    return m_onboardingWidget && m_onboardingWidget->isVisible();
}

void HomePage::retranslateUi() {
    // Update translatable strings
    setWindowTitle(tr("Home"));
    if (m_subtitleText)
        m_subtitleText->setText(tr("Modern PDF Reader"));
    if (m_titleText)
        m_titleText->setText(tr("SAST Readium"));

    if (m_githubCard) {
        m_githubCard->setTitle(tr("GitHub Repository"));
        m_githubCard->setSubTitle(tr("View source code and contribute"));
    }

    if (m_documentationCard) {
        m_documentationCard->setTitle(tr("Documentation"));
        m_documentationCard->setSubTitle(tr("Learn how to use SAST Readium"));
    }

    if (m_quickActionsTitle)
        m_quickActionsTitle->setText(tr("Quick Actions"));

    if (m_openFileCard) {
        m_openFileCard->setTitle(tr("Open PDF File"));
        m_openFileCard->setSubTitle(tr("5.0⭐ Quick Action"));
        m_openFileCard->setInteractiveTips(tr("Click to open"));
        m_openFileCard->setDetailedText(
            tr("Open a PDF file from your computer to start reading"));
    }

    if (m_recentFilesCard) {
        m_recentFilesCard->setTitle(tr("Recent Files"));
        m_recentFilesCard->setSubTitle(tr("5.0⭐ Quick Access"));
        m_recentFilesCard->setInteractiveTips(tr("View recent"));
        m_recentFilesCard->setDetailedText(
            tr("Access your recently opened PDF files"));
    }

    if (m_settingsCard) {
        m_settingsCard->setTitle(tr("Settings"));
        m_settingsCard->setSubTitle(tr("5.0⭐ Configuration"));
        m_settingsCard->setInteractiveTips(tr("Configure"));
        m_settingsCard->setDetailedText(
            tr("Customize your reading experience and application settings"));
    }

    if (m_recentFilesTitle)
        m_recentFilesTitle->setText(tr("Recent Files"));
    if (m_clearRecentFilesButton)
        m_clearRecentFilesButton->setText(tr("Clear All"));
    if (m_emptyRecentFilesLabel)
        m_emptyRecentFilesLabel->setText(tr("No recent files"));

    if (m_versionText)
        m_versionText->setText(tr("Version %1").arg(PROJECT_VER));
    if (m_copyrightText)
        m_copyrightText->setText(tr("© 2024 SAST Team. All rights reserved."));

    // Tutorial section
    if (m_tutorialTitle)
        m_tutorialTitle->setText(tr("Getting Started"));

    // Tips section
    if (m_tipsTitle)
        m_tipsTitle->setText(tr("💡 Tip of the Day"));
    if (m_nextTipButton)
        m_nextTipButton->setText(tr("Next Tip"));

    // Re-initialize tips with translated strings
    initializeTips();
}

void HomePage::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    ElaScrollPage::changeEvent(event);
}

void HomePage::showEvent(QShowEvent* event) {
    ElaScrollPage::showEvent(event);

    // PERFORMANCE FIX: Defer refreshRecentFiles() to prevent UI freeze
    // The refresh was blocking the UI thread because it checks file existence
    // synchronously. Using QTimer::singleShot(0) defers it to the next event
    // loop.
    if (m_isInitialized) {
        QTimer::singleShot(0, this, [this]() { refreshRecentFiles(); });
    }
}

bool HomePage::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        auto* widget = qobject_cast<QWidget*>(watched);
        if (widget && widget->property("filePath").isValid()) {
            QString filePath = widget->property("filePath").toString();
            onRecentFileClicked(filePath);
            return true;
        }
    }
    return ElaScrollPage::eventFilter(watched, event);
}
