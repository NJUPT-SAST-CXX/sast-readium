#include "MainWindow.h"

// ElaWidgetTools
#include "ElaApplication.h"
#include "ElaContentDialog.h"
#include "ElaMenu.h"
#include "ElaMenuBar.h"
#include "ElaStatusBar.h"
#include "ElaText.h"
#include "ElaTheme.h"
#include "ElaToolBar.h"

// Pages
#include "ui/pages/AboutPage.h"
#include "ui/pages/HomePage.h"
#include "ui/pages/PDFViewerPage.h"
#include "ui/pages/PluginManagerPage.h"
#include "ui/pages/SettingsPage.h"

// Adapters
#include "adapters/DocumentAdapter.h"
#include "adapters/ViewAdapter.h"

// Business Logic (from app_lib)
#include "controller/ApplicationController.h"
#include "controller/DocumentController.h"
#include "controller/ServiceLocator.h"
#include "logging/SimpleLogging.h"
#include "managers/I18nManager.h"
#include "managers/RecentFilesManager.h"
#include "managers/StyleManager.h"
#include "plugin/PluginInterface.h"
#include "plugin/PluginManager.h"
#include "search/SearchEngine.h"

// Qt
#include <QCloseEvent>
#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <iostream>

MainWindow::MainWindow(QWidget* parent)
    : ElaWindow(parent),
      m_homePage(nullptr),
      m_pdfViewerPage(nullptr),
      m_settingsPage(nullptr),
      m_aboutPage(nullptr),
      m_pluginManagerPage(nullptr),
      m_isInitialized(false),
      m_currentPage(0),
      m_totalPages(0),
      m_currentZoom(1.0) {
    std::cout << "[TRACE] MainWindow: Constructor started" << std::endl;
    SLOG_INFO("MainWindow: Constructor started");

    try {
        // Initialize window
        std::cout << "[TRACE] MainWindow: Calling initWindow()" << std::endl;
        initWindow();
        std::cout << "[TRACE] MainWindow: initWindow() completed" << std::endl;

        // Initialize theme
        std::cout << "[TRACE] MainWindow: Calling initTheme()" << std::endl;
        initTheme();
        std::cout << "[TRACE] MainWindow: initTheme() completed" << std::endl;

        // Initialize navigation structure
        std::cout << "[TRACE] MainWindow: Calling initNavigation()"
                  << std::endl;
        initNavigation();
        std::cout << "[TRACE] MainWindow: initNavigation() completed"
                  << std::endl;

        // Initialize pages
        std::cout << "[TRACE] MainWindow: Calling initPages()" << std::endl;
        initPages();
        std::cout << "[TRACE] MainWindow: initPages() completed" << std::endl;

        // Initialize business logic
        std::cout << "[TRACE] MainWindow: Calling initBusinessLogic()"
                  << std::endl;
        initBusinessLogic();
        std::cout << "[TRACE] MainWindow: initBusinessLogic() completed"
                  << std::endl;

        // Initialize plugin UI extensions
        std::cout << "[TRACE] MainWindow: Calling initPluginUIExtensions()"
                  << std::endl;
        initPluginUIExtensions();
        std::cout << "[TRACE] MainWindow: initPluginUIExtensions() completed"
                  << std::endl;

        // Connect signals
        std::cout << "[TRACE] MainWindow: Calling connectSignals()"
                  << std::endl;
        connectSignals();
        std::cout << "[TRACE] MainWindow: connectSignals() completed"
                  << std::endl;

        // Restore window state
        std::cout << "[TRACE] MainWindow: Calling restoreWindowState()"
                  << std::endl;
        restoreWindowState();
        std::cout << "[TRACE] MainWindow: restoreWindowState() completed"
                  << std::endl;

        m_isInitialized = true;
        std::cout << "[TRACE] MainWindow: Initialization completed successfully"
                  << std::endl;
        SLOG_INFO("MainWindow: Initialization completed");
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] MainWindow: Exception during initialization: "
                  << e.what() << std::endl;
        SLOG_ERROR_F("MainWindow: Exception during initialization: {}",
                     e.what());
        throw;
    } catch (...) {
        std::cerr
            << "[ERROR] MainWindow: Unknown exception during initialization"
            << std::endl;
        SLOG_ERROR("MainWindow: Unknown exception during initialization");
        throw;
    }
}

MainWindow::~MainWindow() { SLOG_INFO("MainWindow: Destructor called"); }

void MainWindow::initWindow() {
    SLOG_INFO("MainWindow: Initializing window properties");

    // Set window properties
    setWindowTitle(tr("SAST Readium - ElaWidgetTools Edition"));
    setWindowIcon(QIcon(":/icons/app_icon"));

    // Set optimal window size (16:10 aspect ratio for reading)
    resize(1400, 900);
    setMinimumSize(1024, 768);

    // Configure window features
    setIsStayTop(false);
    setIsFixedSize(false);
    setIsDefaultClosed(false);  // Custom close handling
    setIsNavigationBarEnable(true);
    setNavigationBarDisplayMode(ElaNavigationType::Auto);

    // Set window button flags for better UX
    setWindowButtonFlags(ElaAppBarType::MinimizeButtonHint |
                         ElaAppBarType::MaximizeButtonHint |
                         ElaAppBarType::CloseButtonHint);

    // Set user info card with better branding
    setUserInfoCardVisible(true);
    setUserInfoCardPixmap(QPixmap(":/icons/user_avatar"));
    setUserInfoCardTitle(tr("SAST Readium"));
    setUserInfoCardSubTitle(tr("Modern PDF Reader"));

    // Add central stack page (default page when no navigation selected)
    m_centralWelcomeText = new ElaText(tr("Welcome to SAST Readium"), this);
    m_centralWelcomeText->setTextPixelSize(32);
    m_centralWelcomeText->setAlignment(Qt::AlignCenter);
    addCentralWidget(m_centralWelcomeText);

    SLOG_INFO("MainWindow: Window properties initialized");
}

void MainWindow::initTheme() {
    SLOG_INFO("MainWindow: Initializing theme system");

    // Sync ElaTheme with StyleManager
    Theme currentTheme = StyleManager::instance().currentTheme();
    ElaThemeType::ThemeMode elaMode = (currentTheme == Theme::Light)
                                          ? ElaThemeType::Light
                                          : ElaThemeType::Dark;
    eTheme->setThemeMode(elaMode);

    SLOG_INFO("MainWindow: Theme system initialized");
}

void MainWindow::initNavigation() {
    SLOG_INFO("MainWindow: Initializing navigation structure");

    // Home page (top-level)
    // Will be added when creating HomePage in initPages()

    // Documents expander node - for file management
    addExpanderNode(tr("Documents"), m_documentsKey, ElaIconType::FolderOpen);

    // Tools expander node - for utilities
    addExpanderNode(tr("Tools"), m_toolsKey, ElaIconType::Toolbox);

    // Footer nodes will be added in initPages() with actual page widgets
    // Following ElaWidgetTools example pattern for Settings and About

    SLOG_INFO("MainWindow: Navigation structure initialized");
}

void MainWindow::initPages() {
    SLOG_INFO("MainWindow: Initializing pages");

    // ========================================================================
    // Home Page - Top-level navigation
    // ========================================================================
    m_homePage = new HomePage(this);
    addPageNode(tr("Home"), m_homePage, ElaIconType::House);
    m_homeKey = m_homePage->property("ElaPageKey").toString();

    // ========================================================================
    // Documents Section
    // ========================================================================

    // PDF Viewer - Main document viewing page
    m_pdfViewerPage = new PDFViewerPage(this);
    addPageNode(tr("PDF Viewer"), m_pdfViewerPage, m_documentsKey,
                ElaIconType::FileLines);
    m_pdfViewerKey = m_pdfViewerPage->property("ElaPageKey").toString();

    // Recent Files - Quick access to recent documents (navigates to HomePage)
    addPageNode(tr("Recent Files"), m_homePage, m_documentsKey,
                ElaIconType::ClockRotateLeft);
    m_recentFilesKey = m_homePage->property("ElaPageKey").toString();

    // ========================================================================
    // Tools Section
    // ========================================================================

    // Plugin Manager
    m_pluginManagerPage = new PluginManagerPage(this);
    addPageNode(tr("Plugin Manager"), m_pluginManagerPage, m_toolsKey,
                ElaIconType::Puzzle);
    m_pluginManagerKey = m_pluginManagerPage->property("ElaPageKey").toString();

    // ========================================================================
    // Footer Section - Settings and About
    // ========================================================================

    // Settings Page
    m_settingsPage = new SettingsPage(this);
    addFooterNode(tr("Settings"), m_settingsPage, m_settingsKey, 0,
                  ElaIconType::GearComplex);
    m_settingsKey = m_settingsPage->property("ElaPageKey").toString();

    // Set managers for SettingsPage
    m_settingsPage->setI18nManager(&I18nManager::instance());
    m_settingsPage->setStyleManager(&StyleManager::instance());

    // About Page - Shown as dialog
    m_aboutPage = new AboutPage(this);
    addFooterNode(tr("About"), nullptr, m_aboutKey, 0, ElaIconType::CircleInfo);

    // ========================================================================
    // Connect Page Signals
    // ========================================================================

    // HomePage - Open file request
    connect(m_homePage, &HomePage::openFileRequested, this, [this]() {
        if (m_pdfViewerPage) {
            navigation(m_pdfViewerKey);
            m_pdfViewerPage->openFile(QString());
        }
    });

    // HomePage - Open recent file request
    connect(m_homePage, &HomePage::openRecentFileRequested, this,
            [this](const QString& filePath) {
                if (m_pdfViewerPage) {
                    navigation(m_pdfViewerKey);
                    m_pdfViewerPage->openFile(filePath);

                    // Add to recent files
                    if (m_recentFilesManager) {
                        m_recentFilesManager->addRecentFile(filePath);
                    }
                }
            });

    // HomePage - Show settings request
    connect(m_homePage, &HomePage::showSettingsRequested, this,
            [this]() { navigation(m_settingsKey); });

    // ========================================================================
    // Initial Navigation State
    // ========================================================================

    // Expand Documents node by default for better discoverability
    expandNavigationNode(m_documentsKey);

    // Navigate to home page by default
    navigation(m_homeKey);

    SLOG_INFO("MainWindow: Pages initialized");
}

void MainWindow::initBusinessLogic() {
    SLOG_INFO("MainWindow: Initializing business logic");

    // Initialize RecentFilesManager
    m_recentFilesManager = std::make_unique<RecentFilesManager>(this);

    // Connect RecentFilesManager to HomePage
    if (m_homePage && m_recentFilesManager) {
        m_homePage->setRecentFilesManager(m_recentFilesManager.get());
    }

    // Initialize business logic components
    // The controllers and delegates are created and managed by the
    // PDFViewerPage This method can be used to perform any additional
    // initialization if needed

    // Connect to application-level services
    // For example: connect to I18nManager, StyleManager, etc.

    // The actual business logic (DocumentController, PageController, etc.)
    // is initialized when a document is opened in PDFViewerPage

    SLOG_INFO("MainWindow: Business logic initialized");
}

void MainWindow::connectSignals() {
    SLOG_INFO("MainWindow: Connecting signals");

    // Navigation signals
    connect(this, &ElaWindow::navigationNodeClicked, this,
            &MainWindow::onNavigationNodeClicked);

    // Theme signals
    connect(eTheme, &ElaTheme::themeModeChanged, this,
            &MainWindow::onThemeChanged);

    // I18n signals (use qOverload to disambiguate overloaded signal)
    connect(&I18nManager::instance(),
            qOverload<const QString&>(&I18nManager::languageChanged), this,
            &MainWindow::onLanguageChanged);

    // User info card clicked
    connect(this, &ElaWindow::userInfoCardClicked, this, [this]() {
        SLOG_INFO("MainWindow: User info card clicked");
        navigation(m_homeKey);
    });

    // Custom close handling
    connect(this, &ElaWindow::closeButtonClicked, this, [this]() {
        SLOG_INFO("MainWindow: Close button clicked");

        // Show close confirmation dialog
        ElaContentDialog* closeDialog = new ElaContentDialog(this);
        closeDialog->setWindowTitle(tr("Confirm Exit"));
        closeDialog->setMiddleButtonText(tr("Minimize"));
        closeDialog->setRightButtonText(tr("Exit"));
        closeDialog->setLeftButtonText(tr("Cancel"));

        ElaText* messageText =
            new ElaText(tr("Do you want to exit SAST Readium?"), this);
        messageText->setTextPixelSize(15);
        closeDialog->setCentralWidget(messageText);

        connect(closeDialog, &ElaContentDialog::rightButtonClicked, this,
                [this]() { closeWindow(); });

        connect(closeDialog, &ElaContentDialog::middleButtonClicked, this,
                [this, closeDialog]() {
                    closeDialog->close();
                    showMinimized();
                });

        closeDialog->exec();
    });

    SLOG_INFO("MainWindow: Signals connected");
}

void MainWindow::retranslateUi() {
    SLOG_INFO("MainWindow: Retranslating UI");

    // Update window title
    setWindowTitle(tr("SAST Readium - ElaWidgetTools Edition"));

    // Update user info card
    setUserInfoCardTitle(tr("SAST Readium"));
    setUserInfoCardSubTitle(tr("Modern PDF Reader"));

    if (m_centralWelcomeText != nullptr) {
        m_centralWelcomeText->setText(tr("Welcome to SAST Readium"));
    }

    // Update navigation node titles
    setNavigationNodeTitle(m_documentsKey, tr("Documents"));
    setNavigationNodeTitle(m_toolsKey, tr("Tools"));
    setNavigationNodeTitle(m_settingsKey, tr("Settings"));
    setNavigationNodeTitle(m_aboutKey, tr("About"));

    // Pages will handle their own retranslation
}

void MainWindow::updateTheme() {
    SLOG_INFO("MainWindow: Updating theme");
    // Theme is automatically handled by ElaTheme
    // Additional custom theme logic can be added here if needed
}

// ============================================================================
// Slots
// ============================================================================

void MainWindow::onNavigationNodeClicked(
    ElaNavigationType::NavigationNodeType nodeType, QString nodeKey) {
    SLOG_INFO("MainWindow: Navigation node clicked: " + nodeKey);

    // Handle special nodes (following ElaWidgetTools example pattern)
    if (nodeKey == m_aboutKey) {
        // Show about dialog instead of navigating
        // ElaDialog automatically centers itself and handles modality
        if (m_aboutPage) {
            m_aboutPage->show();
        }
    }
}

void MainWindow::onThemeChanged(ElaThemeType::ThemeMode themeMode) {
    SLOG_INFO("MainWindow: Theme changed");

    // Sync with StyleManager
    Theme theme =
        (themeMode == ElaThemeType::Light) ? Theme::Light : Theme::Dark;
    StyleManager::instance().setTheme(theme);
}

void MainWindow::onLanguageChanged(const QString& languageCode) {
    SLOG_INFO("MainWindow: Language changed to " + languageCode);
    retranslateUi();
}

void MainWindow::onDocumentLoaded(const QString& filePath) {
    SLOG_INFO("MainWindow: Document loaded: " + filePath);
    m_currentDocumentPath = filePath;
}

void MainWindow::onDocumentClosed() {
    SLOG_INFO("MainWindow: Document closed");
    m_currentDocumentPath.clear();
    m_currentPage = 0;
    m_totalPages = 0;
}

void MainWindow::onPageChanged(int currentPage, int totalPages) {
    m_currentPage = currentPage;
    m_totalPages = totalPages;
}

void MainWindow::onZoomChanged(double zoomFactor) {
    m_currentZoom = zoomFactor;
}

void MainWindow::onSearchCompleted(int resultCount) {
    SLOG_INFO("MainWindow: Search completed with " +
              QString::number(resultCount) + " results");
}

void MainWindow::onError(const QString& context, const QString& error) {
    SLOG_ERROR("MainWindow: Error in " + context + ": " + error);
    QMessageBox::critical(this, tr("Error"), error);
}

// ============================================================================
// Command-line integration
// ============================================================================

void MainWindow::openFileFromCommandLine(const QString& filePath) {
    SLOG_INFO("MainWindow: Opening file from command line: " + filePath);
    // Switch to PDF viewer page and open the file
    if (m_pdfViewerPage) {
        // Navigate to PDF viewer page using ElaWindow's navigation
        // Note: The navigation key should match the one used in
        // initNavigation() For now, just call openFile directly
        m_pdfViewerPage->openFile(filePath);
    }
}

void MainWindow::setViewModeFromCommandLine(int mode) {
    SLOG_INFO("MainWindow: Setting view mode from command line: " +
              QString::number(mode));

    if (!m_pdfViewerPage) {
        SLOG_ERROR("MainWindow: PDFViewerPage is null");
        return;
    }

    // Set view mode on PDF viewer page
    m_pdfViewerPage->setViewMode(mode);
}

void MainWindow::setZoomLevelFromCommandLine(double zoom) {
    SLOG_INFO("MainWindow: Setting zoom level from command line: " +
              QString::number(zoom));
    // Set zoom level on PDF viewer page
    if (m_pdfViewerPage) {
        m_pdfViewerPage->setZoom(zoom);
    }
}

void MainWindow::goToPageFromCommandLine(int page) {
    SLOG_INFO("MainWindow: Going to page from command line: " +
              QString::number(page));
    // Navigate to page on PDF viewer page
    if (m_pdfViewerPage) {
        m_pdfViewerPage->goToPage(page);
    }
}

// ============================================================================
// Event handlers
// ============================================================================

void MainWindow::closeEvent(QCloseEvent* event) {
    SLOG_INFO("MainWindow: Close event received");

    // Save window state using QSettings
    QSettings settings("SAST", "Readium");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("isMaximized", isMaximized());

    // Save current page if available
    if (m_pdfViewerPage && m_pdfViewerPage->hasDocument()) {
        settings.setValue("lastFilePath", m_pdfViewerPage->currentFilePath());
        settings.setValue("lastPage", m_pdfViewerPage->currentPage());
        settings.setValue("lastZoom", m_pdfViewerPage->zoomLevel());
    }

    SLOG_INFO("MainWindow: Window state saved");

    event->accept();
}

void MainWindow::restoreWindowState() {
    SLOG_INFO("MainWindow: Restoring window state");

    QSettings settings("SAST", "Readium");

    // Restore geometry
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    }

    // Restore window state
    if (settings.contains("windowState")) {
        restoreState(settings.value("windowState").toByteArray());
    }

    // Restore maximized state
    if (settings.value("isMaximized", false).toBool()) {
        showMaximized();
    }

    // CRITICAL FIX: Defer file opening to after event loop starts
    // Opening files synchronously during construction freezes the UI
    // Use QTimer::singleShot to defer until after show() and event loop start
    if (settings.value("viewer/rememberLastPage", true).toBool()) {
        QString lastFilePath = settings.value("lastFilePath").toString();
        if (!lastFilePath.isEmpty() && QFile::exists(lastFilePath)) {
            SLOG_INFO(
                "MainWindow: Will restore last file after event loop starts: " +
                lastFilePath);

            // Capture settings values for deferred execution
            int lastPage = settings.value("lastPage", 1).toInt();
            double lastZoom = settings.value("lastZoom", 1.0).toDouble();

            // Defer file opening until after event loop starts (200ms delay)
            // This ensures the window is fully initialized and visible
            QTimer::singleShot(
                200, this, [this, lastFilePath, lastPage, lastZoom]() {
                    SLOG_INFO("MainWindow: Restoring last file (deferred): " +
                              lastFilePath);

                    // Open the file
                    if (m_pdfViewerPage &&
                        m_pdfViewerPage->openFile(lastFilePath)) {
                        // Navigate to PDF viewer page
                        navigation(m_pdfViewerKey);

                        // Restore last page and zoom with a small delay to
                        // ensure document is loaded
                        QTimer::singleShot(
                            100, this, [this, lastPage, lastZoom]() {
                                if (m_pdfViewerPage) {
                                    m_pdfViewerPage->goToPage(lastPage);
                                    m_pdfViewerPage->setZoom(lastZoom);
                                    SLOG_INFO(
                                        "MainWindow: Last page and zoom "
                                        "restored");
                                }
                            });
                    } else {
                        SLOG_WARNING("MainWindow: Failed to restore last file");
                    }
                });
        }
    }

    SLOG_INFO("MainWindow: Window state restored (file opening deferred)");
}

void MainWindow::initPluginUIExtensions() {
    SLOG_INFO("MainWindow: Initializing plugin UI extensions");

    // Get PluginManager from ServiceLocator
    auto& serviceLocator = ServiceLocator::instance();
    PluginManager* pluginManager = serviceLocator.getService<PluginManager>();

    if (!pluginManager) {
        SLOG_WARNING(
            "MainWindow: PluginManager not available in ServiceLocator");
        return;
    }

    // Register this MainWindow with ServiceLocator for plugin access
    // Note: ElaWindow doesn't inherit from QMainWindow, so plugins expecting
    // QMainWindow interface won't work. We would need to wrap or cast
    // appropriately. For now, we'll register extension points that don't rely
    // on QMainWindow

    SLOG_DEBUG("MainWindow: Registering plugin extension points");

    // Register Menu extension point
    static MenuExtensionPoint menuEP;
    pluginManager->registerExtensionPoint(&menuEP);

    // Register Toolbar extension point
    static ToolbarExtensionPoint toolbarEP;
    pluginManager->registerExtensionPoint(&toolbarEP);

    // Register Dock Widget extension point
    // Note: Dock widgets require QMainWindow, which ElaWindow doesn't provide
    // Uncomment when we have a proper wrapper or if ElaWindow adds dock support
    // static DockWidgetExtensionPoint dockWidgetEP;
    // pluginManager->registerExtensionPoint(&dockWidgetEP);

    // Register Context Menu extension point
    static ContextMenuExtensionPoint contextMenuEP;
    pluginManager->registerExtensionPoint(&contextMenuEP);

    // Register Status Bar extension point
    static StatusBarExtensionPoint statusBarEP;
    pluginManager->registerExtensionPoint(&statusBarEP);

    SLOG_INFO("MainWindow: Plugin UI extensions initialized successfully");
}
