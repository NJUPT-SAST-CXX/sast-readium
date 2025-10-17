#include "WelcomeWidget.h"
#include <QApplication>
#include <QDebug>
#include <QEasingCurve>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QShowEvent>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>
#include "../../logging/LoggingMacros.h"
#include "../../managers/OnboardingManager.h"
#include "../../managers/RecentFilesManager.h"
#include "../../managers/StyleManager.h"
#include "../managers/WelcomeScreenManager.h"
#include "RecentFileListWidget.h"
#include "TutorialCard.h"

// Static const member definitions - required for linking when address is taken
const int WelcomeWidget::LOGO_SIZE;
const int WelcomeWidget::CONTENT_MAX_WIDTH;
const int WelcomeWidget::SPACING_XLARGE;
const int WelcomeWidget::SPACING_LARGE;
const int WelcomeWidget::SPACING_MEDIUM;
const int WelcomeWidget::SPACING_SMALL;
const int WelcomeWidget::SPACING_XSMALL;

WelcomeWidget::WelcomeWidget(QWidget* parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_contentWidget(nullptr),
      m_scrollArea(nullptr),
      m_logoWidget(nullptr),
      m_logoLayout(nullptr),
      m_logoLabel(nullptr),
      m_titleLabel(nullptr),
      m_versionLabel(nullptr),
      m_actionsWidget(nullptr),
      m_actionsLayout(nullptr),
      m_newFileButton(nullptr),
      m_openFileButton(nullptr),
      m_recentFilesWidget(nullptr),
      m_recentFilesLayout(nullptr),
      m_recentFilesTitle(nullptr),
      m_recentFilesList(nullptr),
      m_noRecentFilesLabel(nullptr),
      m_separatorLine(nullptr),
      m_recentFilesManager(nullptr),
      m_welcomeScreenManager(nullptr),
      m_opacityEffect(nullptr),
      m_fadeAnimation(nullptr),
      m_refreshTimer(nullptr),
      m_isInitialized(false),
      m_isVisible(false) {
    LOG_DEBUG("WelcomeWidget: Initializing...");

    // 设置基本属性
    setObjectName("WelcomeWidget");
    setAttribute(Qt::WA_StyledBackground, true);

    // 初始化UI
    initializeUI();

    // 设置动画效果
    StyleManager* styleManager = &StyleManager::instance();
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(m_opacityEffect);

    m_fadeAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeAnimation->setDuration(styleManager->animationSlow());
    m_fadeAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    // 设置刷新定时器
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(100);

    setupConnections();

    m_isInitialized = true;
    LOG_DEBUG("WelcomeWidget: Initialization completed");
}

WelcomeWidget::~WelcomeWidget() { LOG_DEBUG("WelcomeWidget: Destroying..."); }

void WelcomeWidget::setRecentFilesManager(RecentFilesManager* manager) {
    if (m_recentFilesManager == manager)
        return;

    // 断开旧连接
    if (m_recentFilesManager) {
        disconnect(m_recentFilesManager, nullptr, this, nullptr);
    }

    m_recentFilesManager = manager;

    // 建立新连接
    if (m_recentFilesManager && m_recentFilesList) {
        m_recentFilesList->setRecentFilesManager(m_recentFilesManager);
        connect(m_recentFilesManager, &RecentFilesManager::recentFilesChanged,
                this, &WelcomeWidget::onRecentFilesChanged);
    }

    // 刷新内容
    refreshContent();
}

void WelcomeWidget::setWelcomeScreenManager(WelcomeScreenManager* manager) {
    m_welcomeScreenManager = manager;
}

void WelcomeWidget::applyTheme() {
    if (!m_isInitialized)
        return;

    LOG_DEBUG("WelcomeWidget: Applying theme...");

    // 清除所有内联样式，让QSS文件接管样式控制
    setStyleSheet("");
    if (m_scrollArea)
        m_scrollArea->setStyleSheet("");
    if (m_contentWidget)
        m_contentWidget->setStyleSheet("");
    if (m_titleLabel)
        m_titleLabel->setStyleSheet("");
    if (m_versionLabel)
        m_versionLabel->setStyleSheet("");
    if (m_recentFilesTitle)
        m_recentFilesTitle->setStyleSheet("");
    if (m_noRecentFilesLabel)
        m_noRecentFilesLabel->setStyleSheet("");
    if (m_separatorLine)
        m_separatorLine->setStyleSheet("");
    if (m_newFileButton)
        m_newFileButton->setStyleSheet("");
    if (m_openFileButton)
        m_openFileButton->setStyleSheet("");

    // 更新logo（仍需要根据主题选择不同的图标）
    updateLogo();

    // 应用主题到最近文件列表
    if (m_recentFilesList) {
        m_recentFilesList->applyTheme();
    }

    // 强制样式更新
    style()->unpolish(this);
    style()->polish(this);
    if (m_scrollArea) {
        style()->unpolish(m_scrollArea);
        style()->polish(m_scrollArea);
    }
    if (m_contentWidget) {
        style()->unpolish(m_contentWidget);
        style()->polish(m_contentWidget);
    }
    update();

    LOG_DEBUG("WelcomeWidget: Theme applied successfully");
}

void WelcomeWidget::refreshContent() {
    if (!m_isInitialized)
        return;

    LOG_DEBUG("WelcomeWidget: Refreshing content...");

    // 刷新最近文件列表
    if (m_recentFilesList && m_recentFilesManager) {
        m_recentFilesList->refreshList();

        // 更新最近文件区域的可见性
        bool hasRecentFiles = m_recentFilesManager->hasRecentFiles();
        if (m_recentFilesList)
            m_recentFilesList->setVisible(hasRecentFiles);
        if (m_noRecentFilesLabel)
            m_noRecentFilesLabel->setVisible(!hasRecentFiles);
    } else {
        // 如果没有管理器，显示无最近文件标签
        if (m_recentFilesList)
            m_recentFilesList->setVisible(false);
        if (m_noRecentFilesLabel)
            m_noRecentFilesLabel->setVisible(true);
    }

    // 更新布局
    updateLayout();
}

void WelcomeWidget::onRecentFilesChanged() {
    LOG_DEBUG("WelcomeWidget: Recent files changed, refreshing...");

    // 延迟刷新以避免频繁更新
    if (m_refreshTimer) {
        m_refreshTimer->start();
    }
}

void WelcomeWidget::onThemeChanged() {
    LOG_DEBUG("WelcomeWidget: Theme changed, applying new theme...");
    applyTheme();
}

void WelcomeWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
}

void WelcomeWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateLayout();
}

void WelcomeWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    if (!m_isVisible) {
        m_isVisible = true;
        startFadeInAnimation();
        refreshContent();
    }
}

void WelcomeWidget::onNewFileClicked() {
    LOG_DEBUG("WelcomeWidget: New file requested");
    emit newFileRequested();
}

void WelcomeWidget::onOpenFileClicked() {
    LOG_DEBUG("WelcomeWidget: Open file requested");
    emit openFileRequested();
}

void WelcomeWidget::onOpenFolderClicked() {
    LOG_DEBUG("WelcomeWidget: Open folder requested");
    emit openFolderRequested();
}

void WelcomeWidget::onRecentFileClicked(const QString& filePath) {
    LOG_DEBUG("WelcomeWidget: Recent file clicked: {}", filePath.toStdString());
    emit fileOpenRequested(filePath);
}

void WelcomeWidget::onFadeInFinished() {
    LOG_DEBUG("WelcomeWidget: Fade in animation finished");
}

void WelcomeWidget::onTutorialCardClicked(const QString& tutorialId) {
    LOG_DEBUG("WelcomeWidget: Tutorial card clicked: {}",
              tutorialId.toStdString());
    emit tutorialRequested(tutorialId);
}

void WelcomeWidget::onQuickActionClicked() {
    QToolButton* btn = qobject_cast<QToolButton*>(sender());
    if (btn) {
        QString action = btn->text();
        LOG_DEBUG("WelcomeWidget: Quick action clicked: {}",
                  action.toStdString());

        if (action == tr("Search")) {
            // Handle search action
        } else if (action == tr("Bookmarks")) {
            // Handle bookmarks action
        } else if (action == tr("Settings")) {
            emit showSettingsRequested();
        } else if (action == tr("Help")) {
            emit showDocumentationRequested();
        }
    }
}

void WelcomeWidget::onShowMoreTipsClicked() {
    LOG_DEBUG("WelcomeWidget: Show more tips requested");
    refreshTips();
}

void WelcomeWidget::onKeyboardShortcutClicked() {
    LOG_DEBUG("WelcomeWidget: Keyboard shortcuts requested");
    emit showDocumentationRequested();
}

void WelcomeWidget::onStartTourClicked() {
    LOG_DEBUG("WelcomeWidget: Start tour requested");
    emit startOnboardingRequested();

    if (m_onboardingManager) {
        m_onboardingManager->startOnboarding();
    }
}

void WelcomeWidget::initializeUI() {
    LOG_DEBUG("WelcomeWidget: Initializing UI components...");

    // Set size policy for welcome widget
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(400, 300);  // Minimum size for readability

    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 创建滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 创建内容容器
    m_contentWidget = new QWidget();
    m_contentWidget->setObjectName("WelcomeContentWidget");

    // 设置内容布局
    setupLayout();
    setupLogo();
    setupActions();
    setupQuickActions();
    setupTutorialCards();
    setupRecentFiles();
    setupTipsSection();
    setupKeyboardShortcuts();

    // 将内容设置到滚动区域
    m_scrollArea->setWidget(m_contentWidget);
    m_mainLayout->addWidget(m_scrollArea);

    LOG_DEBUG("WelcomeWidget: UI components initialized");
}

void WelcomeWidget::setupLayout() {
    if (!m_contentWidget)
        return;

    QVBoxLayout* contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(SPACING_XLARGE, SPACING_XLARGE,
                                      SPACING_XLARGE, SPACING_XLARGE);
    contentLayout->setSpacing(SPACING_XLARGE);
    contentLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    // 添加弹性空间以居中内容
    contentLayout->addStretch(1);

    // Logo区域 - Enhanced with better spacing
    m_logoWidget = new QWidget();
    m_logoWidget->setObjectName("WelcomeLogoWidget");
    contentLayout->addWidget(m_logoWidget, 0, Qt::AlignCenter);

    // 操作按钮区域 - Optimized layout for better visual hierarchy
    m_actionsWidget = new QWidget();
    m_actionsWidget->setObjectName("WelcomeActionsWidget");
    contentLayout->addWidget(m_actionsWidget, 0, Qt::AlignCenter);

    // 分隔线1 - Enhanced visual design
    m_separatorLine = new QFrame();
    m_separatorLine->setObjectName("WelcomeSeparatorLine");
    m_separatorLine->setFrameShape(QFrame::HLine);
    m_separatorLine->setFrameShadow(QFrame::Plain);
    m_separatorLine->setFixedHeight(2);  // Increased visibility
    m_separatorLine->setMaximumWidth(CONTENT_MAX_WIDTH);
    contentLayout->addWidget(m_separatorLine, 0, Qt::AlignCenter);

    // Create a horizontal layout for better content organization
    QHBoxLayout* mainContentLayout = new QHBoxLayout();
    mainContentLayout->setSpacing(SPACING_LARGE);
    mainContentLayout->setAlignment(Qt::AlignCenter);

    // Left column - Quick Actions and Tutorial Cards
    QVBoxLayout* leftColumnLayout = new QVBoxLayout();
    leftColumnLayout->setSpacing(SPACING_LARGE);
    leftColumnLayout->setAlignment(Qt::AlignTop);

    // Quick Actions区域 - Compact and efficient
    m_quickActionsWidget = new QWidget();
    m_quickActionsWidget->setObjectName("WelcomeQuickActionsWidget");
    m_quickActionsWidget->setMaximumWidth(CONTENT_MAX_WIDTH / 2 - SPACING_LARGE / 2);
    leftColumnLayout->addWidget(m_quickActionsWidget, 0, Qt::AlignTop);

    // Tutorial Cards区域 - Enhanced layout
    m_tutorialCardsWidget = new QWidget();
    m_tutorialCardsWidget->setObjectName("WelcomeTutorialCardsWidget");
    m_tutorialCardsWidget->setMaximumWidth(CONTENT_MAX_WIDTH / 2 - SPACING_LARGE / 2);
    leftColumnLayout->addWidget(m_tutorialCardsWidget, 0, Qt::AlignTop);

    leftColumnLayout->addStretch();

    // Right column - Recent Files (Primary focus)
    QVBoxLayout* rightColumnLayout = new QVBoxLayout();
    rightColumnLayout->setSpacing(SPACING_LARGE);
    rightColumnLayout->setAlignment(Qt::AlignTop);

    // 最近文件区域 - Enhanced prominence and organization
    m_recentFilesWidget = new QWidget();
    m_recentFilesWidget->setObjectName("WelcomeRecentFilesWidget");
    m_recentFilesWidget->setMaximumWidth(CONTENT_MAX_WIDTH / 2 - SPACING_LARGE / 2);
    rightColumnLayout->addWidget(m_recentFilesWidget, 0, Qt::AlignTop);

    // Tips区域 - Repositioned for better UX
    m_tipsWidget = new QWidget();
    m_tipsWidget->setObjectName("WelcomeTipsWidget");
    m_tipsWidget->setMaximumWidth(CONTENT_MAX_WIDTH / 2 - SPACING_LARGE / 2);
    rightColumnLayout->addWidget(m_tipsWidget, 0, Qt::AlignTop);

    rightColumnLayout->addStretch();

    // Add columns to main layout
    QWidget* leftColumnWidget = new QWidget();
    leftColumnWidget->setLayout(leftColumnLayout);
    mainContentLayout->addWidget(leftColumnWidget);

    // 分隔线2 - Vertical separator between columns
    QFrame* verticalSeparator = new QFrame();
    verticalSeparator->setFrameShape(QFrame::VLine);
    verticalSeparator->setFrameShadow(QFrame::Plain);
    verticalSeparator->setFixedWidth(2);
    mainContentLayout->addWidget(verticalSeparator);

    QWidget* rightColumnWidget = new QWidget();
    rightColumnWidget->setLayout(rightColumnLayout);
    mainContentLayout->addWidget(rightColumnWidget);

    // Add the main content layout
    QWidget* mainContentContainer = new QWidget();
    mainContentContainer->setLayout(mainContentLayout);
    mainContentContainer->setMaximumWidth(CONTENT_MAX_WIDTH);
    contentLayout->addWidget(mainContentContainer, 0, Qt::AlignCenter);

    // Keyboard Shortcuts区域 - Moved to bottom for less frequent access
    m_shortcutsWidget = new QWidget();
    m_shortcutsWidget->setObjectName("WelcomeShortcutsWidget");
    m_shortcutsWidget->setMaximumWidth(CONTENT_MAX_WIDTH / 2);
    contentLayout->addWidget(m_shortcutsWidget, 0, Qt::AlignCenter);

    // 添加底部弹性空间
    contentLayout->addStretch(2);
}

void WelcomeWidget::setupLogo() {
    if (!m_logoWidget)
        return;

    m_logoLayout = new QVBoxLayout(m_logoWidget);
    m_logoLayout->setContentsMargins(0, 0, 0, 0);
    m_logoLayout->setSpacing(SPACING_SMALL);
    m_logoLayout->setAlignment(Qt::AlignCenter);

    // Logo图标
    m_logoLabel = new QLabel();
    m_logoLabel->setObjectName("WelcomeLogoLabel");
    m_logoLabel->setFixedSize(LOGO_SIZE, LOGO_SIZE);
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setScaledContents(true);

    // 应用程序标题
    m_titleLabel = new QLabel(QApplication::applicationDisplayName().isEmpty()
                                  ? "SAST Readium"
                                  : QApplication::applicationDisplayName());
    m_titleLabel->setObjectName("WelcomeTitleLabel");
    m_titleLabel->setAlignment(Qt::AlignCenter);

    // 版本信息
    QString version = QApplication::applicationVersion();
    if (version.isEmpty())
        version = "1.0.0";
    m_versionLabel = new QLabel(QString("Version %1").arg(version));
    m_versionLabel->setObjectName("WelcomeVersionLabel");
    m_versionLabel->setAlignment(Qt::AlignCenter);

    m_logoLayout->addWidget(m_logoLabel);
    m_logoLayout->addWidget(m_titleLabel);
    m_logoLayout->addWidget(m_versionLabel);

    // 初始加载logo
    updateLogo();
}

void WelcomeWidget::updateLogo() {
    if (!m_logoLabel)
        return;

    // 根据当前主题选择合适的logo
    StyleManager& styleManager = StyleManager::instance();
    QString logoPath;

    if (styleManager.currentTheme() == Theme::Dark) {
        logoPath = ":/images/logo-dark";
    } else {
        logoPath = ":/images/logo";
    }

    // 尝试加载SVG logo
    QPixmap logoPixmap(logoPath);
    if (logoPixmap.isNull()) {
        // 如果SVG加载失败，尝试加载应用程序图标
        logoPixmap = QPixmap(":/images/icon");
        if (logoPixmap.isNull()) {
            // 如果都没有，创建一个简单的占位符
            logoPixmap = QPixmap(LOGO_SIZE, LOGO_SIZE);
            logoPixmap.fill(Qt::transparent);
            QPainter painter(&logoPixmap);
            painter.setRenderHint(QPainter::Antialiasing);

            QColor logoColor = styleManager.currentTheme() == Theme::Dark
                                   ? QColor(79, 195, 247)
                                   : QColor(0, 120, 212);
            painter.setBrush(QBrush(logoColor));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(0, 0, LOGO_SIZE, LOGO_SIZE);

            // 添加简单的文档图标
            painter.setBrush(QBrush(Qt::white));
            painter.drawRect(LOGO_SIZE / 4, LOGO_SIZE / 4, LOGO_SIZE / 2,
                             LOGO_SIZE / 2);
        }
    }

    // 确保logo大小正确
    if (logoPixmap.size() != QSize(LOGO_SIZE, LOGO_SIZE)) {
        logoPixmap =
            logoPixmap.scaled(LOGO_SIZE, LOGO_SIZE, Qt::KeepAspectRatio,
                              Qt::SmoothTransformation);
    }

    m_logoLabel->setPixmap(logoPixmap);
}

void WelcomeWidget::setupActions() {
    if (!m_actionsWidget)
        return;

    // Create a more visually appealing action button layout
    QHBoxLayout* actionsContainerLayout = new QHBoxLayout(m_actionsWidget);
    actionsContainerLayout->setContentsMargins(0, 0, 0, 0);
    actionsContainerLayout->setSpacing(SPACING_MEDIUM);
    actionsContainerLayout->setAlignment(Qt::AlignCenter);

    // Create a visual container for the primary actions
    QWidget* primaryActionsWidget = new QWidget();
    primaryActionsWidget->setObjectName("PrimaryActionsWidget");
    QHBoxLayout* primaryActionsLayout = new QHBoxLayout(primaryActionsWidget);
    primaryActionsLayout->setSpacing(SPACING_MEDIUM);
    primaryActionsLayout->setContentsMargins(SPACING_SMALL, SPACING_SMALL, SPACING_SMALL, SPACING_SMALL);

    // Primary action - Open File (most common action)
    m_openFileButton = new QPushButton(tr("Open File..."));
    m_openFileButton->setObjectName("WelcomeOpenFileButton");
    m_openFileButton->setCursor(Qt::PointingHandCursor);
    m_openFileButton->setProperty("buttonRole", "primary");  // For styling

    // Secondary actions
    m_newFileButton = new QPushButton(tr("New File"));
    m_newFileButton->setObjectName("WelcomeNewFileButton");
    m_newFileButton->setCursor(Qt::PointingHandCursor);
    m_newFileButton->setProperty("buttonRole", "secondary");

    m_openFolderButton = new QPushButton(tr("Open Folder..."));
    m_openFolderButton->setObjectName("WelcomeOpenFolderButton");
    m_openFolderButton->setCursor(Qt::PointingHandCursor);
    m_openFolderButton->setProperty("buttonRole", "secondary");

    // Add buttons with proper visual hierarchy
    primaryActionsLayout->addWidget(m_newFileButton);
    primaryActionsLayout->addWidget(m_openFileButton);
    primaryActionsLayout->addWidget(m_openFolderButton);

    // Add the primary actions container to the main layout
    actionsContainerLayout->addWidget(primaryActionsWidget, 0, Qt::AlignCenter);

    // Add keyboard shortcut hints for better UX
    QLabel* shortcutsHint = new QLabel(tr("Quick access: Ctrl+O to open, Ctrl+N for new file"));
    shortcutsHint->setObjectName("ShortcutsHintLabel");
    shortcutsHint->setAlignment(Qt::AlignCenter);
    shortcutsHint->setStyleSheet("color: #8c8c8c; font-size: 11px; margin-top: 8px;");
    actionsContainerLayout->addWidget(shortcutsHint, 0, Qt::AlignCenter);
}

void WelcomeWidget::setupRecentFiles() {
    if (!m_recentFilesWidget)
        return;

    m_recentFilesLayout = new QVBoxLayout(m_recentFilesWidget);
    m_recentFilesLayout->setContentsMargins(0, 0, 0, 0);
    m_recentFilesLayout->setSpacing(SPACING_SMALL);

    // 最近文件标题
    m_recentFilesTitle = new QLabel(tr("Recent Files"));
    m_recentFilesTitle->setObjectName("WelcomeRecentFilesTitle");
    m_recentFilesTitle->setAlignment(Qt::AlignLeft);

    // 最近文件列表
    m_recentFilesList = new RecentFileListWidget();
    m_recentFilesList->setObjectName("WelcomeRecentFilesList");

    // 无最近文件标签
    m_noRecentFilesLabel = new QLabel(tr("No recent files"));
    m_noRecentFilesLabel->setObjectName("WelcomeNoRecentFilesLabel");
    m_noRecentFilesLabel->setAlignment(Qt::AlignCenter);
    m_noRecentFilesLabel->setVisible(false);

    m_recentFilesLayout->addWidget(m_recentFilesTitle);
    m_recentFilesLayout->addWidget(m_recentFilesList);
    m_recentFilesLayout->addWidget(m_noRecentFilesLabel);
}

void WelcomeWidget::setupQuickActions() {
    if (!m_quickActionsWidget)
        return;

    m_quickActionsLayout = new QGridLayout(m_quickActionsWidget);
    m_quickActionsLayout->setContentsMargins(0, 0, 0, 0);
    m_quickActionsLayout->setSpacing(SPACING_MEDIUM);

    // Create quick action buttons
    auto createQuickAction = [this](const QString& text, const QString& icon,
                                    int row, int col) {
        QToolButton* btn = new QToolButton();
        btn->setText(text);
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        btn->setCursor(Qt::PointingHandCursor);
        if (!icon.isEmpty()) {
            btn->setIcon(QIcon(icon));
            btn->setIconSize(QSize(32, 32));
        }
        m_quickActionsLayout->addWidget(btn, row, col);
        m_quickActionButtons.append(btn);
        return btn;
    };

    createQuickAction(tr("Search"), ":/icons/search", 0, 0);
    createQuickAction(tr("Bookmarks"), ":/icons/bookmark", 0, 1);
    createQuickAction(tr("Settings"), ":/icons/settings", 0, 2);
    createQuickAction(tr("Help"), ":/icons/help", 0, 3);
}

void WelcomeWidget::setupTutorialCards() {
    if (!m_tutorialCardsWidget)
        return;

    m_tutorialCardsLayout = new QVBoxLayout(m_tutorialCardsWidget);
    m_tutorialCardsLayout->setContentsMargins(0, 0, 0, 0);
    m_tutorialCardsLayout->setSpacing(SPACING_MEDIUM);

    // Title
    m_tutorialCardsTitle = new QLabel(tr("Interactive Tutorials"));
    m_tutorialCardsTitle->setObjectName("WelcomeTutorialCardsTitle");
    m_tutorialCardsTitle->setAlignment(Qt::AlignLeft);

    // Container for cards
    m_tutorialCardsContainer = new QWidget();
    m_tutorialCardsContainerLayout = new QHBoxLayout(m_tutorialCardsContainer);
    m_tutorialCardsContainerLayout->setSpacing(SPACING_MEDIUM);
    m_tutorialCardsContainerLayout->setAlignment(Qt::AlignLeft);

    // Add tutorial cards using OnboardingManager's data
    if (m_onboardingManager) {
        QJsonArray tutorials = m_onboardingManager->getAvailableTutorials();
        for (const auto& tutorialValue : tutorials) {
            QJsonObject tutorial = tutorialValue.toObject();
            createTutorialCard(tutorial["id"].toString(),
                               tutorial["title"].toString(),
                               tutorial["description"].toString(),
                               QString(":/icons/tutorial"));
        }
    }

    m_tutorialCardsLayout->addWidget(m_tutorialCardsTitle);
    m_tutorialCardsLayout->addWidget(m_tutorialCardsContainer);

    // Start tour button
    QPushButton* startTourBtn = new QPushButton(tr("Start Tour"));
    startTourBtn->setObjectName("WelcomeStartTourButton");
    startTourBtn->setCursor(Qt::PointingHandCursor);
    connect(startTourBtn, &QPushButton::clicked, this,
            &WelcomeWidget::onStartTourClicked);
    m_tutorialCardsLayout->addWidget(startTourBtn, 0, Qt::AlignLeft);
}

void WelcomeWidget::setupTipsSection() {
    if (!m_tipsWidget)
        return;

    m_tipsLayout = new QVBoxLayout(m_tipsWidget);
    m_tipsLayout->setContentsMargins(0, 0, 0, 0);
    m_tipsLayout->setSpacing(SPACING_SMALL);

    // Title
    m_tipsTitle = new QLabel(tr("Tips & Tricks"));
    m_tipsTitle->setObjectName("WelcomeTipsTitle");
    m_tipsTitle->setAlignment(Qt::AlignLeft);

    // Initialize tips list
    m_tips =
        QStringList()
        << tr("Press Ctrl+F to quickly search within the document")
        << tr("Use Ctrl+B to add a bookmark to the current page")
        << tr("Double-click on the page to zoom in, right-click to zoom out")
        << tr("Press F11 to toggle full-screen mode")
        << tr("Use Page Up/Down keys for quick navigation")
        << tr("Drag and drop PDF files directly into the window to open them")
        << tr("Press Ctrl+Tab to switch between open documents")
        << tr("Use Ctrl+G to jump to a specific page number");

    m_currentTipIndex = 0;

    // Current tip label
    m_currentTipLabel = new QLabel(m_tips[m_currentTipIndex]);
    m_currentTipLabel->setObjectName("WelcomeCurrentTipLabel");
    m_currentTipLabel->setWordWrap(true);
    m_currentTipLabel->setAlignment(Qt::AlignLeft);

    // Navigation buttons
    QHBoxLayout* tipNavLayout = new QHBoxLayout();
    m_previousTipButton = new QPushButton(tr("Previous Tip"));
    m_nextTipButton = new QPushButton(tr("Next Tip"));

    connect(m_previousTipButton, &QPushButton::clicked, [this]() {
        if (--m_currentTipIndex < 0)
            m_currentTipIndex = m_tips.size() - 1;
        m_currentTipLabel->setText(m_tips[m_currentTipIndex]);
    });

    connect(m_nextTipButton, &QPushButton::clicked, [this]() {
        if (++m_currentTipIndex >= m_tips.size())
            m_currentTipIndex = 0;
        m_currentTipLabel->setText(m_tips[m_currentTipIndex]);
    });

    tipNavLayout->addWidget(m_previousTipButton);
    tipNavLayout->addWidget(m_nextTipButton);
    tipNavLayout->addStretch();

    m_tipsLayout->addWidget(m_tipsTitle);
    m_tipsLayout->addWidget(m_currentTipLabel);
    m_tipsLayout->addLayout(tipNavLayout);
}

void WelcomeWidget::setupKeyboardShortcuts() {
    if (!m_shortcutsWidget)
        return;

    m_shortcutsLayout = new QVBoxLayout(m_shortcutsWidget);
    m_shortcutsLayout->setContentsMargins(0, 0, 0, 0);
    m_shortcutsLayout->setSpacing(SPACING_SMALL);

    // Title
    m_shortcutsTitle = new QLabel(tr("Keyboard Shortcuts"));
    m_shortcutsTitle->setObjectName("WelcomeShortcutsTitle");
    m_shortcutsTitle->setAlignment(Qt::AlignLeft);

    // Shortcuts list widget
    m_shortcutsListWidget = new QWidget();
    QGridLayout* shortcutsGrid = new QGridLayout(m_shortcutsListWidget);
    shortcutsGrid->setSpacing(SPACING_XSMALL);

    // Add common shortcuts
    struct Shortcut {
        QString keys;
        QString description;
    };

    QList<Shortcut> shortcuts = {
        {"Ctrl+O", tr("Open file")},  {"Ctrl+S", tr("Save file")},
        {"Ctrl+F", tr("Search")},     {"Ctrl+B", tr("Add bookmark")},
        {"Ctrl+G", tr("Go to page")}, {"F11", tr("Full screen")},
        {"Ctrl++", tr("Zoom in")},    {"Ctrl+-", tr("Zoom out")}};

    int row = 0;
    for (const auto& shortcut : shortcuts) {
        QLabel* keysLabel = new QLabel(shortcut.keys);
        QLabel* descLabel = new QLabel(shortcut.description);
        keysLabel->setObjectName("ShortcutKeys");
        descLabel->setObjectName("ShortcutDescription");
        shortcutsGrid->addWidget(keysLabel, row, 0);
        shortcutsGrid->addWidget(descLabel, row, 1);
        row++;
    }

    // Learn more button
    QPushButton* learnMoreBtn = new QPushButton(tr("Learn More Shortcuts"));
    learnMoreBtn->setObjectName("WelcomeLearnShortcutsButton");
    connect(learnMoreBtn, &QPushButton::clicked, this,
            &WelcomeWidget::onKeyboardShortcutClicked);

    m_shortcutsLayout->addWidget(m_shortcutsTitle);
    m_shortcutsLayout->addWidget(m_shortcutsListWidget);
    m_shortcutsLayout->addWidget(learnMoreBtn, 0, Qt::AlignLeft);
}

void WelcomeWidget::setupConnections() {
    // 按钮连接 - Enhanced with accessibility and keyboard shortcuts
    if (m_newFileButton) {
        connect(m_newFileButton, &QPushButton::clicked, this,
                &WelcomeWidget::onNewFileClicked);
        // Set accessibility properties
        m_newFileButton->setAccessibleName(tr("Create new PDF document"));
        m_newFileButton->setAccessibleDescription(tr("Click to create a new PDF document or press Ctrl+N"));
        m_newFileButton->setShortcut(QKeySequence::New);
    }

    if (m_openFileButton) {
        connect(m_openFileButton, &QPushButton::clicked, this,
                &WelcomeWidget::onOpenFileClicked);
        // Set accessibility properties
        m_openFileButton->setAccessibleName(tr("Open PDF file"));
        m_openFileButton->setAccessibleDescription(tr("Click to open an existing PDF file or press Ctrl+O"));
        m_openFileButton->setShortcut(QKeySequence::Open);
    }

    if (m_openFolderButton) {
        connect(m_openFolderButton, &QPushButton::clicked, this,
                &WelcomeWidget::onOpenFolderClicked);
        // Set accessibility properties
        m_openFolderButton->setAccessibleName(tr("Open folder containing PDF files"));
        m_openFolderButton->setAccessibleDescription(tr("Click to open a folder and browse PDF files or press Ctrl+Shift+O"));
        m_openFolderButton->setShortcut(QKeySequence("Ctrl+Shift+O"));
    }

    // 最近文件列表连接 - Enhanced accessibility
    if (m_recentFilesList) {
        connect(m_recentFilesList, &RecentFileListWidget::fileClicked, this,
                &WelcomeWidget::onRecentFileClicked);
        m_recentFilesList->setAccessibleName(tr("Recent files list"));
        m_recentFilesList->setAccessibleDescription(tr("List of recently opened PDF files. Use arrow keys to navigate and Enter to open."));
    }

    // Quick action buttons accessibility
    for (QToolButton* button : m_quickActionButtons) {
        if (button) {
            QString action = button->text();
            if (action == tr("Search")) {
                button->setAccessibleName(tr("Search in documents"));
                button->setAccessibleDescription(tr("Search for text within PDF documents or press Ctrl+F"));
                button->setShortcut(QKeySequence::Find);
            } else if (action == tr("Bookmarks")) {
                button->setAccessibleName(tr("Bookmarks"));
                button->setAccessibleDescription(tr("View and manage bookmarks or press Ctrl+B"));
                button->setShortcut(QKeySequence("Ctrl+B"));
            } else if (action == tr("Settings")) {
                button->setAccessibleName(tr("Application settings"));
                button->setAccessibleDescription(tr("Open application settings and preferences"));
            } else if (action == tr("Help")) {
                button->setAccessibleName(tr("Help and documentation"));
                button->setAccessibleDescription(tr("Get help and view documentation or press F1"));
                button->setShortcut(QKeySequence::HelpContents);
            }
        }
    }

    // 动画连接
    if (m_fadeAnimation) {
        connect(m_fadeAnimation, &QPropertyAnimation::finished, this,
                &WelcomeWidget::onFadeInFinished);
    }

    // 刷新定时器连接
    if (m_refreshTimer) {
        connect(m_refreshTimer, &QTimer::timeout, this,
                &WelcomeWidget::refreshContent);
    }

    // 主题管理器连接
    connect(&StyleManager::instance(), &StyleManager::themeChanged, this,
            &WelcomeWidget::onThemeChanged);

    // Setup keyboard navigation for the entire widget
    setupKeyboardNavigation();
}

void WelcomeWidget::setupKeyboardNavigation() {
    // Set overall accessibility
    setAccessibleName(tr("Welcome screen"));
    setAccessibleDescription(tr("SAST Readium PDF viewer welcome screen. Use Tab to navigate between sections."));

    // Enable keyboard focus
    setFocusPolicy(Qt::TabFocus);

    // Setup global keyboard shortcuts for enhanced accessibility
    QShortcut* shortcut1 = new QShortcut(QKeySequence("Ctrl+1"), this);
    connect(shortcut1, &QShortcut::activated, [this]() {
        if (m_openFileButton) {
            m_openFileButton->click();
        }
    });

    QShortcut* shortcut2 = new QShortcut(QKeySequence("Ctrl+2"), this);
    connect(shortcut2, &QShortcut::activated, [this]() {
        if (m_newFileButton) {
            m_newFileButton->click();
        }
    });

    QShortcut* shortcut3 = new QShortcut(QKeySequence("Ctrl+3"), this);
    connect(shortcut3, &QShortcut::activated, [this]() {
        if (m_openFolderButton) {
            m_openFolderButton->click();
        }
    });

    // Focus management for better accessibility
    setFocusProxy(m_openFileButton ? m_openFileButton : m_newFileButton);
}

void WelcomeWidget::updateLayout() {
    if (!m_contentWidget)
        return;

    // 根据窗口大小调整内容宽度
    int availableWidth = width();
    int contentWidth =
        qMin(availableWidth - 2 * SPACING_LARGE, CONTENT_MAX_WIDTH);

    if (m_recentFilesWidget) {
        m_recentFilesWidget->setMaximumWidth(contentWidth);
    }

    if (m_separatorLine) {
        m_separatorLine->setMaximumWidth(contentWidth);
    }
}

void WelcomeWidget::createTutorialCard(const QString& id, const QString& title,
                                       const QString& description,
                                       const QString& iconPath) {
    if (!m_tutorialCardsContainerLayout)
        return;

    TutorialCard* card =
        new TutorialCard(id, title, description, QIcon(iconPath));
    connect(card, &TutorialCard::clicked, this,
            &WelcomeWidget::onTutorialCardClicked);
    m_tutorialCardsContainerLayout->addWidget(card);
}

void WelcomeWidget::setOnboardingManager(OnboardingManager* manager) {
    m_onboardingManager = manager;

    // Refresh tutorial cards if widget is already initialized
    if (m_isInitialized && m_tutorialCardsWidget) {
        setupTutorialCards();
    }
}

void WelcomeWidget::refreshTips() {
    // Rotate to next tip
    if (++m_currentTipIndex >= m_tips.size()) {
        m_currentTipIndex = 0;
    }

    if (m_currentTipLabel && !m_tips.isEmpty()) {
        m_currentTipLabel->setText(m_tips[m_currentTipIndex]);
    }
}

void WelcomeWidget::refreshShortcuts() {
    // Refresh keyboard shortcuts if needed
    if (m_shortcutsWidget) {
        setupKeyboardShortcuts();
    }
}

void WelcomeWidget::startFadeInAnimation() {
    if (!m_fadeAnimation || !m_opacityEffect)
        return;

    m_opacityEffect->setOpacity(0.0);
    m_fadeAnimation->setStartValue(0.0);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();
}
