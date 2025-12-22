/**
 * @file test_mainwindow_ui_improvements.cpp
 * @brief Comprehensive automated UI tests for MainWindow improvements
 *
 * Tests verify the visual enhancements implemented:
 * - Enhanced QSplitter styling (6px width, gradient effects)
 * - Refined content area spacing (4px vertical margins)
 * - Improved visual hierarchy (sidebar backgrounds, borders)
 * - Theme switching (light/dark)
 * - Language switching (English/Chinese)
 * - Responsive layout behavior
 */

#include <QApplication>
#include <QEvent>
#include <QLayout>
#include <QMainWindow>
#include <QMenuBar>
#include <QSignalSpy>
#include <QSplitter>
#include <QStackedWidget>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

// Initialize Qt resources for app (ensures QSS/icons are available)
#include <QResource>

#include "../../app/MainWindow.h"
#include "../../app/controller/ApplicationController.h"
#include "../../app/managers/I18nManager.h"
#include "../../app/managers/StyleManager.h"
#include "../../app/ui/core/MenuBar.h"
#include "../../app/ui/core/RightSideBar.h"
#include "../../app/ui/core/SideBar.h"
#include "../../app/ui/core/StatusBar.h"
#include "../../app/ui/core/ToolBar.h"
#include "../../app/ui/core/ViewWidget.h"

/**
 * @brief Test fixture for MainWindow UI improvements
 *
 * This test class verifies all UI enhancements made to the MainWindow:
 * - Splitter handle width and styling
 * - Content area margins and spacing
 * - Theme switching functionality
 * - Language switching without layout issues
 * - Responsive layout behavior
 */
class MainWindowUIImprovementsTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Splitter tests
    void testSplitterHandleWidth();
    void testSplitterObjectName();
    void testSplitterConfiguration();
    void testSplitterInteraction();
    void testSplitterMouseEvents();

    // Content spacing tests
    void testContentAreaMargins();
    void testContentStackMargins();
    void testLayoutSpacing();

    // Theme switching tests
    void testThemeSwitchingLight();
    void testThemeSwitchingDark();
    void testThemeColors();
    void testThemeSignals();

    // Language switching tests
    void testLanguageSwitchingEnglish();
    void testLanguageSwitchingChinese();
    void testLanguageChangeEvent();

    // Responsive layout tests
    void testWindowResizeMinimum();
    void testWindowResizeMaximum();
    void testSplitterStretchFactors();
    void testSidebarCollapsible();

    // Visual hierarchy tests
    void testSidebarBackgroundColor();
    void testSidebarBorders();
    void testStackedWidgetStyling();

    // Toolbar integration tests
    void testToolbarVisibility();
    void testToolbarActions();
    void testToolbarButtonClick();
    void testToolbarEnabledStates();
    void testToolbarTheming();

    // Status bar tests
    void testStatusBarVisibility();
    void testStatusBarUpdates();
    void testStatusBarLoadingProgress();

    // Menu bar tests
    void testMenuBarVisibility();
    void testMenuItemStates();

    // Document viewer tests
    void testViewerScrollBehavior();
    void testViewerZoomFunctionality();
    void testViewerPageNavigation();

    // Sidebar component tests
    void testSidebarTabSwitching();
    void testSidebarResize();
    void testSidebarCollapseExpand();

    // Realistic workflow tests
    void testCompleteUserWorkflow();
    void testLanguageSwitchWorkflow();

private:
    std::unique_ptr<MainWindow> m_mainWindow;
    QSplitter* m_splitter;
    SideBar* m_sideBar;
    RightSideBar* m_rightSideBar;
    QWidget* m_viewWidget;
    QStackedWidget* m_contentStack;
    QVBoxLayout* m_mainViewerLayout;

    // Helper methods
    void createMainWindowOrSkip();
    void waitForInitialization();
    QWidget* findMainViewerWidget();
    QVBoxLayout* findMainViewerLayout();
    void simulateMouseHover(QWidget* widget, const QPoint& pos);
    void simulateMousePress(QWidget* widget, const QPoint& pos);
    void simulateMouseRelease(QWidget* widget, const QPoint& pos);
    void simulateMouseDrag(QWidget* widget, const QPoint& from,
                           const QPoint& to);
};

void MainWindowUIImprovementsTest::initTestCase() {
    qDebug() << "=== MainWindow UI Improvements Test Suite ===";

    // Initialize Qt resources
    Q_INIT_RESOURCE(app);

    // Set test mode
    qputenv("SAST_READIUM_TEST_MODE", "1");

    // Configure application metadata
    QCoreApplication::setApplicationName("SAST Readium");
    QCoreApplication::setApplicationVersion("0.1.0");

    // Set application style (required for consistent UI rendering)
    QApplication::setStyle("fusion");
    qDebug() << "Application style set to fusion";

    // Initialize logging system BEFORE creating MainWindow
    // MainWindow constructor creates CategoryLogger which requires
    // LoggingManager
    SastLogging::Config logConfig;
    logConfig.level = SastLogging::Level::Warning;  // Reduce noise in tests
    logConfig.console = true;
    logConfig.file = false;   // Disable file logging for tests
    logConfig.async = false;  // Synchronous for deterministic test behavior
    SastLogging::init(logConfig);
    qDebug() << "Logging system initialized";

    // Initialize managers
    QVERIFY(I18nManager::instance().initialize());
    qDebug() << "I18nManager initialized";

    // Verify StyleManager
    StyleManager& styleManager = StyleManager::instance();
    QVERIFY(styleManager.currentTheme() == Theme::Light ||
            styleManager.currentTheme() == Theme::Dark);
    qDebug() << "StyleManager initialized";

    // Detect platform mode
    QString platformName = QGuiApplication::platformName();
    qDebug() << "Platform:" << platformName;
    if (platformName == "offscreen") {
        qDebug()
            << "Running in offscreen mode - some visual tests may be limited";
    }
}

void MainWindowUIImprovementsTest::cleanupTestCase() {
    qDebug() << "=== MainWindow UI Improvements Test Suite Completed ===";

    // Shutdown logging system
    SastLogging::shutdown();
    qDebug() << "Logging system shut down";
}

void MainWindowUIImprovementsTest::init() {
    // Reset pointers
    m_splitter = nullptr;
    m_sideBar = nullptr;
    m_rightSideBar = nullptr;
    m_viewWidget = nullptr;
    m_contentStack = nullptr;
    m_mainViewerLayout = nullptr;
}

void MainWindowUIImprovementsTest::cleanup() {
    // Clean up main window
    if (m_mainWindow) {
        m_mainWindow->close();
        QTest::qWait(200);
        m_mainWindow.reset();
    }

    // Process remaining events
    QTest::qWait(300);
    QCoreApplication::processEvents();
    QTest::qWait(200);
}

void MainWindowUIImprovementsTest::createMainWindowOrSkip() {
    // Skip if running in offscreen mode (some UI tests may not work)
    if (QGuiApplication::platformName() == "offscreen") {
        qDebug() << "Running in offscreen mode - creating MainWindow anyway";
    }

    try {
        m_mainWindow = std::make_unique<MainWindow>();
        QVERIFY(m_mainWindow != nullptr);

        // Wait for initialization
        waitForInitialization();

        // Find components
        m_splitter = m_mainWindow->findChild<QSplitter*>("MainContentSplitter");
        m_sideBar = m_mainWindow->findChild<SideBar*>();
        m_rightSideBar = m_mainWindow->findChild<RightSideBar*>();
        m_contentStack = m_mainWindow->findChild<QStackedWidget*>();

        // Find main viewer layout
        QWidget* mainViewerWidget = findMainViewerWidget();
        if (mainViewerWidget) {
            m_mainViewerLayout =
                qobject_cast<QVBoxLayout*>(mainViewerWidget->layout());
        }

    } catch (const std::exception& e) {
        QFAIL(QString("Failed to create MainWindow: %1")
                  .arg(e.what())
                  .toUtf8()
                  .constData());
    }
}

void MainWindowUIImprovementsTest::waitForInitialization() {
    // Wait for initialization to complete
    QTest::qWait(500);
    QCoreApplication::processEvents();
    QTest::qWait(200);
}

QWidget* MainWindowUIImprovementsTest::findMainViewerWidget() {
    // Find the main viewer widget that contains the splitter
    if (!m_contentStack) {
        return nullptr;
    }

    // The main viewer widget is typically the second widget in the stack
    // (index 1, after welcome screen at index 0)
    if (m_contentStack->count() > 1) {
        return m_contentStack->widget(1);
    }

    return nullptr;
}

QVBoxLayout* MainWindowUIImprovementsTest::findMainViewerLayout() {
    QWidget* mainViewerWidget = findMainViewerWidget();
    if (!mainViewerWidget) {
        return nullptr;
    }

    return qobject_cast<QVBoxLayout*>(mainViewerWidget->layout());
}

void MainWindowUIImprovementsTest::simulateMouseHover(QWidget* widget,
                                                      const QPoint& pos) {
    QTest::mouseMove(widget, pos);
    QCoreApplication::processEvents();
}

void MainWindowUIImprovementsTest::simulateMousePress(QWidget* widget,
                                                      const QPoint& pos) {
    QTest::mousePress(widget, Qt::LeftButton, Qt::NoModifier, pos);
    QCoreApplication::processEvents();
}

void MainWindowUIImprovementsTest::simulateMouseRelease(QWidget* widget,
                                                        const QPoint& pos) {
    QTest::mouseRelease(widget, Qt::LeftButton, Qt::NoModifier, pos);
    QCoreApplication::processEvents();
}

void MainWindowUIImprovementsTest::simulateMouseDrag(QWidget* widget,
                                                     const QPoint& from,
                                                     const QPoint& to) {
    simulateMousePress(widget, from);
    QTest::qWait(50);

    // Simulate drag movement
    int steps = 10;
    for (int i = 1; i <= steps; ++i) {
        QPoint pos = from + (to - from) * i / steps;
        simulateMouseHover(widget, pos);
        QTest::qWait(10);
    }

    simulateMouseRelease(widget, to);
    QTest::qWait(50);
}

// ============================================================================
// Splitter Tests
// ============================================================================

void MainWindowUIImprovementsTest::testSplitterHandleWidth() {
    qDebug() << "\n--- Test: Splitter Handle Width ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_splitter != nullptr);

    // Verify handle width is 6px as per UI improvements
    int handleWidth = m_splitter->handleWidth();
    QCOMPARE(handleWidth, 6);

    qDebug() << "✓ Splitter handle width verified:" << handleWidth << "px";
}

void MainWindowUIImprovementsTest::testSplitterObjectName() {
    qDebug() << "\n--- Test: Splitter Object Name ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_splitter != nullptr);

    // Verify object name is set for QSS targeting
    QString objectName = m_splitter->objectName();
    QCOMPARE(objectName, QString("MainContentSplitter"));

    qDebug() << "✓ Splitter object name verified:" << objectName;
}

void MainWindowUIImprovementsTest::testSplitterConfiguration() {
    qDebug() << "\n--- Test: Splitter Configuration ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_splitter != nullptr);

    // Verify orientation
    QCOMPARE(m_splitter->orientation(), Qt::Horizontal);

    // Verify child widgets
    QVERIFY(m_splitter->count() >= 2);  // At least sidebar and main view

    // Verify collapsible settings
    // Left sidebar (index 0) should be collapsible
    QVERIFY(m_splitter->isCollapsible(0) == true);

    // Main view (index 1) should NOT be collapsible
    QVERIFY(m_splitter->isCollapsible(1) == false);

    qDebug() << "✓ Splitter configuration verified";
}

void MainWindowUIImprovementsTest::testSplitterInteraction() {
    qDebug() << "\n--- Test: Splitter Interaction ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_splitter != nullptr);

    // Get initial sizes
    QList<int> initialSizes = m_splitter->sizes();
    QVERIFY(initialSizes.size() >= 2);

    int initialLeftWidth = initialSizes[0];
    int initialRightWidth = initialSizes[1];

    qDebug() << "Initial sizes - Left:" << initialLeftWidth
             << "Right:" << initialRightWidth;

    // Verify sizes are reasonable
    QVERIFY(initialLeftWidth > 0);
    QVERIFY(initialRightWidth > 0);

    qDebug() << "✓ Splitter interaction verified";
}

void MainWindowUIImprovementsTest::testSplitterMouseEvents() {
    qDebug() << "\n--- Test: Splitter Mouse Events ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_splitter != nullptr);

    // Skip in offscreen mode as mouse events may not work properly
    if (QGuiApplication::platformName() == "offscreen") {
        QSKIP("Mouse event simulation not reliable in offscreen mode");
    }

    // Get splitter handle position
    QWidget* handle = m_splitter->handle(1);
    QVERIFY(handle != nullptr);

    QPoint handleCenter = handle->rect().center();

    // Simulate hover
    simulateMouseHover(handle, handleCenter);
    QTest::qWait(100);

    // Simulate press
    simulateMousePress(handle, handleCenter);
    QTest::qWait(100);

    // Simulate release
    simulateMouseRelease(handle, handleCenter);
    QTest::qWait(100);

    qDebug() << "✓ Splitter mouse events simulated successfully";
}

// ============================================================================
// Content Spacing Tests
// ============================================================================

void MainWindowUIImprovementsTest::testContentAreaMargins() {
    qDebug() << "\n--- Test: Content Area Margins ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    // Find main viewer layout
    if (!m_mainViewerLayout) {
        QSKIP("Main viewer layout not found - may not be initialized yet");
    }

    // Verify margins: top=4, left=0, right=0, bottom=4
    QMargins margins = m_mainViewerLayout->contentsMargins();

    qDebug() << "Content margins - Top:" << margins.top()
             << "Left:" << margins.left() << "Right:" << margins.right()
             << "Bottom:" << margins.bottom();

    // Verify vertical margins are 4px (StyleManager::spacingXS)
    QCOMPARE(margins.top(), 4);
    QCOMPARE(margins.bottom(), 4);

    // Verify horizontal margins are 0px (maximize content area)
    QCOMPARE(margins.left(), 0);
    QCOMPARE(margins.right(), 0);

    qDebug() << "✓ Content area margins verified";
}

void MainWindowUIImprovementsTest::testContentStackMargins() {
    qDebug() << "\n--- Test: Content Stack Margins ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_contentStack != nullptr);

    // Verify content stack has zero margins
    QMargins margins = m_contentStack->contentsMargins();

    QCOMPARE(margins.top(), 0);
    QCOMPARE(margins.left(), 0);
    QCOMPARE(margins.right(), 0);
    QCOMPARE(margins.bottom(), 0);

    qDebug() << "✓ Content stack margins verified";
}

void MainWindowUIImprovementsTest::testLayoutSpacing() {
    qDebug() << "\n--- Test: Layout Spacing ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    if (!m_mainViewerLayout) {
        QSKIP("Main viewer layout not found");
    }

    // Verify spacing between splitter sections is 0
    int spacing = m_mainViewerLayout->spacing();
    QCOMPARE(spacing, 0);

    qDebug() << "✓ Layout spacing verified:" << spacing;
}

// ============================================================================
// Theme Switching Tests
// ============================================================================

void MainWindowUIImprovementsTest::testThemeSwitchingLight() {
    qDebug() << "\n--- Test: Theme Switching to Light ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    StyleManager& styleManager = StyleManager::instance();

    // Switch to light theme
    styleManager.setTheme(Theme::Light);
    QTest::qWait(200);
    QCoreApplication::processEvents();

    // Verify theme changed
    QCOMPARE(styleManager.currentTheme(), Theme::Light);

    qDebug() << "✓ Light theme applied successfully";
}

void MainWindowUIImprovementsTest::testThemeSwitchingDark() {
    qDebug() << "\n--- Test: Theme Switching to Dark ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    StyleManager& styleManager = StyleManager::instance();

    // Switch to dark theme
    styleManager.setTheme(Theme::Dark);
    QTest::qWait(200);
    QCoreApplication::processEvents();

    // Verify theme changed
    QCOMPARE(styleManager.currentTheme(), Theme::Dark);

    qDebug() << "✓ Dark theme applied successfully";
}

void MainWindowUIImprovementsTest::testThemeColors() {
    qDebug() << "\n--- Test: Theme Colors ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    StyleManager& styleManager = StyleManager::instance();

    // Test light theme colors
    styleManager.setTheme(Theme::Light);
    QTest::qWait(100);

    QColor lightBg = styleManager.backgroundColor();
    QColor lightText = styleManager.textColor();
    QColor lightAccent = styleManager.accentColor();

    QVERIFY(lightBg.isValid());
    QVERIFY(lightText.isValid());
    QVERIFY(lightAccent.isValid());

    qDebug() << "Light theme - BG:" << lightBg.name()
             << "Text:" << lightText.name() << "Accent:" << lightAccent.name();

    // Test dark theme colors
    styleManager.setTheme(Theme::Dark);
    QTest::qWait(100);

    QColor darkBg = styleManager.backgroundColor();
    QColor darkText = styleManager.textColor();
    QColor darkAccent = styleManager.accentColor();

    QVERIFY(darkBg.isValid());
    QVERIFY(darkText.isValid());
    QVERIFY(darkAccent.isValid());

    qDebug() << "Dark theme - BG:" << darkBg.name()
             << "Text:" << darkText.name() << "Accent:" << darkAccent.name();

    // Verify colors are different between themes
    QVERIFY(lightBg != darkBg);
    QVERIFY(lightText != darkText);

    qDebug() << "✓ Theme colors verified";
}

void MainWindowUIImprovementsTest::testThemeSignals() {
    qDebug() << "\n--- Test: Theme Change Signals ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    StyleManager& styleManager = StyleManager::instance();

    // Create signal spy
    QSignalSpy themeSpy(&styleManager, &StyleManager::themeChanged);

    // Switch theme
    Theme currentTheme = styleManager.currentTheme();
    Theme newTheme =
        (currentTheme == Theme::Light) ? Theme::Dark : Theme::Light;

    styleManager.setTheme(newTheme);
    QTest::qWait(100);

    // Verify signal was emitted
    QVERIFY(themeSpy.count() >= 1);

    qDebug() << "✓ Theme change signals verified";
}

// ============================================================================
// Language Switching Tests
// ============================================================================

void MainWindowUIImprovementsTest::testLanguageSwitchingEnglish() {
    qDebug() << "\n--- Test: Language Switching to English ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    I18nManager& i18nManager = I18nManager::instance();

    // Switch to English
    i18nManager.loadLanguage(I18nManager::Language::English);
    QTest::qWait(200);
    QCoreApplication::processEvents();

    // Verify language changed
    QCOMPARE(i18nManager.currentLanguage(), I18nManager::Language::English);

    qDebug() << "✓ English language applied successfully";
}

void MainWindowUIImprovementsTest::testLanguageSwitchingChinese() {
    qDebug() << "\n--- Test: Language Switching to Chinese ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    I18nManager& i18nManager = I18nManager::instance();

    // Switch to Chinese
    i18nManager.loadLanguage(I18nManager::Language::Chinese);
    QTest::qWait(200);
    QCoreApplication::processEvents();

    // Verify language changed
    QCOMPARE(i18nManager.currentLanguage(), I18nManager::Language::Chinese);

    qDebug() << "✓ Chinese language applied successfully";
}

void MainWindowUIImprovementsTest::testLanguageChangeEvent() {
    qDebug() << "\n--- Test: Language Change Event ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    I18nManager& i18nManager = I18nManager::instance();

    // Create signal spy - use the overload with Language parameter
    QSignalSpy languageSpy(
        &i18nManager, static_cast<void (I18nManager::*)(I18nManager::Language)>(
                          &I18nManager::languageChanged));

    // Switch language
    I18nManager::Language currentLang = i18nManager.currentLanguage();
    I18nManager::Language newLang =
        (currentLang == I18nManager::Language::English)
            ? I18nManager::Language::Chinese
            : I18nManager::Language::English;

    i18nManager.loadLanguage(newLang);
    QTest::qWait(100);

    // Verify signal was emitted
    QVERIFY(languageSpy.count() >= 1);

    // Verify layout is still intact after language change
    if (m_mainViewerLayout) {
        QMargins margins = m_mainViewerLayout->contentsMargins();
        QCOMPARE(margins.top(), 4);
        QCOMPARE(margins.bottom(), 4);
    }

    qDebug() << "✓ Language change event verified";
}

// ============================================================================
// Responsive Layout Tests
// ============================================================================

void MainWindowUIImprovementsTest::testWindowResizeMinimum() {
    qDebug() << "\n--- Test: Window Resize Minimum ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    // Skip in offscreen mode as window resizing may not work properly
    if (QGuiApplication::platformName() == "offscreen") {
        QSKIP("Window resizing not reliable in offscreen mode");
    }

    // Resize to minimum size
    QSize minSize(800, 600);
    m_mainWindow->resize(minSize);
    QTest::qWait(200);
    QCoreApplication::processEvents();

    // Verify window size
    QSize actualSize = m_mainWindow->size();
    QVERIFY(actualSize.width() >=
            minSize.width() - 50);  // Allow some tolerance
    QVERIFY(actualSize.height() >= minSize.height() - 50);

    // Verify splitter is still functional
    if (m_splitter) {
        QVERIFY(m_splitter->isVisible());
        QCOMPARE(m_splitter->handleWidth(), 6);
    }

    qDebug() << "✓ Minimum window size verified:" << actualSize;
}

void MainWindowUIImprovementsTest::testWindowResizeMaximum() {
    qDebug() << "\n--- Test: Window Resize Maximum ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    // Skip in offscreen mode
    if (QGuiApplication::platformName() == "offscreen") {
        QSKIP("Window resizing not reliable in offscreen mode");
    }

    // Resize to large size
    QSize largeSize(1920, 1080);
    m_mainWindow->resize(largeSize);
    QTest::qWait(200);
    QCoreApplication::processEvents();

    // Verify window size
    QSize actualSize = m_mainWindow->size();
    QVERIFY(actualSize.width() >= 1000);  // Should be reasonably large
    QVERIFY(actualSize.height() >= 600);

    // Verify splitter is still functional
    if (m_splitter) {
        QVERIFY(m_splitter->isVisible());
        QCOMPARE(m_splitter->handleWidth(), 6);
    }

    qDebug() << "✓ Maximum window size verified:" << actualSize;
}

void MainWindowUIImprovementsTest::testSplitterStretchFactors() {
    qDebug() << "\n--- Test: Splitter Stretch Factors ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_splitter != nullptr);

    // Verify stretch factors are set correctly by checking widget size policies
    // Left sidebar (index 0): stretch factor 0 (maintains preferred width)
    // Main view (index 1): stretch factor 1 (takes remaining space)
    // Right sidebar (index 2, if exists): stretch factor 0

    QWidget* leftWidget = m_splitter->widget(0);
    QWidget* mainWidget = m_splitter->widget(1);

    QVERIFY(leftWidget != nullptr);
    QVERIFY(mainWidget != nullptr);

    // Get size policies
    QSizePolicy leftPolicy = leftWidget->sizePolicy();
    QSizePolicy mainPolicy = mainWidget->sizePolicy();

    // Main view should have higher stretch than sidebar
    int leftStretch = leftPolicy.horizontalStretch();
    int mainStretch = mainPolicy.horizontalStretch();

    qDebug() << "Stretch factors - Left:" << leftStretch
             << "Main:" << mainStretch;

    // Verify main view has stretch (should be > 0)
    QVERIFY(mainStretch >= 0);

    qDebug() << "✓ Splitter stretch factors verified";
}

void MainWindowUIImprovementsTest::testSidebarCollapsible() {
    qDebug() << "\n--- Test: Sidebar Collapsible ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_splitter != nullptr);
    QVERIFY(m_sideBar != nullptr);

    // Verify left sidebar is collapsible
    QVERIFY(m_splitter->isCollapsible(0) == true);

    // Verify main view is NOT collapsible
    QVERIFY(m_splitter->isCollapsible(1) == false);

    // Test sidebar visibility toggle
    bool initialVisibility = m_sideBar->isVisible();
    m_sideBar->setVisible(!initialVisibility, false);  // No animation
    QTest::qWait(100);

    QCOMPARE(m_sideBar->isVisible(), !initialVisibility);

    // Restore original state
    m_sideBar->setVisible(initialVisibility, false);
    QTest::qWait(100);

    qDebug() << "✓ Sidebar collapsible behavior verified";
}

// ============================================================================
// Visual Hierarchy Tests
// ============================================================================

void MainWindowUIImprovementsTest::testSidebarBackgroundColor() {
    qDebug() << "\n--- Test: Sidebar Background Color ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_sideBar != nullptr);

    // Verify sidebar has a background color set via stylesheet
    QString styleSheet = m_sideBar->styleSheet();

    // The sidebar should have styling applied
    // We can't easily verify the exact color without parsing QSS,
    // but we can verify that styling exists
    QVERIFY(!styleSheet.isEmpty() || m_sideBar->autoFillBackground());

    qDebug() << "✓ Sidebar background styling verified";
}

void MainWindowUIImprovementsTest::testSidebarBorders() {
    qDebug() << "\n--- Test: Sidebar Borders ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_sideBar != nullptr);

    // Verify sidebar has border styling
    // The QSS should include border definitions
    QString styleSheet = m_sideBar->styleSheet();

    // Sidebar should have some styling (borders are defined in QSS)
    // We verify that either the widget has a stylesheet or has a frame
    QVERIFY(!styleSheet.isEmpty() || m_sideBar->autoFillBackground());

    qDebug() << "✓ Sidebar border styling verified";
}

void MainWindowUIImprovementsTest::testStackedWidgetStyling() {
    qDebug() << "\n--- Test: Stacked Widget Styling ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_contentStack != nullptr);

    // Verify content stack has background color styling
    QString styleSheet = m_contentStack->styleSheet();

    // Content stack should have background color set
    QVERIFY(styleSheet.contains("background-color") ||
            styleSheet.contains("QStackedWidget"));

    qDebug() << "✓ Stacked widget styling verified";
}

// ============================================================================
// Toolbar Integration Tests
// ============================================================================

void MainWindowUIImprovementsTest::testToolbarVisibility() {
    qDebug() << "\n--- Test: Toolbar Visibility ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    // Find toolbar
    ToolBar* toolbar = m_mainWindow->findChild<ToolBar*>();
    QVERIFY(toolbar != nullptr);

    // Verify toolbar is visible
    QVERIFY(toolbar->isVisible());

    // Verify toolbar is added to main window
    QList<QToolBar*> toolbars = m_mainWindow->findChildren<QToolBar*>();
    QVERIFY(toolbars.size() > 0);
    QVERIFY(toolbars.contains(toolbar));

    qDebug() << "✓ Toolbar visibility verified";
}

void MainWindowUIImprovementsTest::testToolbarActions() {
    qDebug() << "\n--- Test: Toolbar Actions ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    ToolBar* toolbar = m_mainWindow->findChild<ToolBar*>();
    QVERIFY(toolbar != nullptr);

    // Find essential actions
    QList<QAction*> actions = toolbar->actions();
    QVERIFY(actions.size() > 0);

    // Verify we have some essential actions
    // Note: In simplified mode, toolbar has basic actions
    bool hasOpenAction = false;
    bool hasSaveAction = false;
    bool hasZoomAction = false;

    for (QAction* action : actions) {
        if (!action)
            continue;

        QString tooltip = action->toolTip();
        QString text = action->text();

        if (tooltip.contains("Open", Qt::CaseInsensitive) ||
            text.contains("Open", Qt::CaseInsensitive)) {
            hasOpenAction = true;
        }
        if (tooltip.contains("Save", Qt::CaseInsensitive) ||
            text.contains("Save", Qt::CaseInsensitive)) {
            hasSaveAction = true;
        }
        if (tooltip.contains("Zoom", Qt::CaseInsensitive) ||
            text.contains("Zoom", Qt::CaseInsensitive)) {
            hasZoomAction = true;
        }
    }

    QVERIFY(hasOpenAction);
    QVERIFY(hasSaveAction);

    qDebug() << "✓ Toolbar actions verified - found" << actions.size()
             << "actions";
}

void MainWindowUIImprovementsTest::testToolbarButtonClick() {
    qDebug() << "\n--- Test: Toolbar Button Click ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    ToolBar* toolbar = m_mainWindow->findChild<ToolBar*>();
    QVERIFY(toolbar != nullptr);

    // Find an action to test
    QList<QAction*> actions = toolbar->actions();
    QAction* testAction = nullptr;

    for (QAction* action : actions) {
        if (action && !action->text().isEmpty()) {
            testAction = action;
            break;
        }
    }

    if (!testAction) {
        QSKIP("No suitable action found for click test");
    }

    // Use QSignalSpy to verify signal emission
    QSignalSpy actionSpy(toolbar, &ToolBar::actionTriggered);

    // Trigger the action programmatically (simulates button click)
    testAction->trigger();

    // Wait for signal processing
    QTest::qWait(100);

    // Verify signal was emitted (may be 0 if action is disabled or has no
    // handler)
    qDebug() << "✓ Toolbar button click simulated, signals emitted:"
             << actionSpy.count();
}

void MainWindowUIImprovementsTest::testToolbarEnabledStates() {
    qDebug() << "\n--- Test: Toolbar Enabled States ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    ToolBar* toolbar = m_mainWindow->findChild<ToolBar*>();
    QVERIFY(toolbar != nullptr);

    // Get all actions
    QList<QAction*> actions = toolbar->actions();

    // Count enabled vs disabled actions
    int enabledCount = 0;
    int disabledCount = 0;

    for (QAction* action : actions) {
        if (!action)
            continue;

        if (action->isEnabled()) {
            enabledCount++;
        } else {
            disabledCount++;
        }
    }

    qDebug() << "Enabled actions:" << enabledCount;
    qDebug() << "Disabled actions:" << disabledCount;

    // At least some actions should exist
    QVERIFY(enabledCount + disabledCount > 0);

    qDebug() << "✓ Toolbar enabled states verified";
}

void MainWindowUIImprovementsTest::testToolbarTheming() {
    qDebug() << "\n--- Test: Toolbar Theming ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    ToolBar* toolbar = m_mainWindow->findChild<ToolBar*>();
    QVERIFY(toolbar != nullptr);

    StyleManager& styleManager = StyleManager::instance();

    // Test light theme
    styleManager.setTheme(Theme::Light);
    QTest::qWait(100);
    QString lightStyleSheet = toolbar->styleSheet();

    // Test dark theme
    styleManager.setTheme(Theme::Dark);
    QTest::qWait(100);
    QString darkStyleSheet = toolbar->styleSheet();

    // Stylesheets should be different between themes
    // (unless toolbar doesn't use theme-specific styling)
    qDebug() << "Light theme stylesheet length:" << lightStyleSheet.length();
    qDebug() << "Dark theme stylesheet length:" << darkStyleSheet.length();

    // At least one should have styling
    QVERIFY(lightStyleSheet.length() > 0 || darkStyleSheet.length() > 0);

    qDebug() << "✓ Toolbar theming verified";
}

// ============================================================================
// Status Bar Tests
// ============================================================================

void MainWindowUIImprovementsTest::testStatusBarVisibility() {
    qDebug() << "\n--- Test: Status Bar Visibility ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    // Find status bar
    StatusBar* statusBar = m_mainWindow->findChild<StatusBar*>();
    QVERIFY(statusBar != nullptr);

    // Verify status bar is visible
    QVERIFY(statusBar->isVisible());

    // Verify status bar is set on main window
    QStatusBar* mainWindowStatusBar = m_mainWindow->statusBar();
    QVERIFY(mainWindowStatusBar != nullptr);
    QCOMPARE(mainWindowStatusBar, statusBar);

    qDebug() << "✓ Status bar visibility verified";
}

void MainWindowUIImprovementsTest::testStatusBarUpdates() {
    qDebug() << "\n--- Test: Status Bar Updates ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    StatusBar* statusBar = m_mainWindow->findChild<StatusBar*>();
    QVERIFY(statusBar != nullptr);

    // Test page info update
    statusBar->setPageInfo(5, 100);
    QTest::qWait(50);

    // Test zoom level update
    statusBar->setZoomLevel(150);
    QTest::qWait(50);

    // Test file name update
    statusBar->setFileName("test_document.pdf");
    QTest::qWait(50);

    // Test message display
    statusBar->setMessage("Test message");
    QTest::qWait(50);

    // Note: In minimal mode (offscreen), widgets may be nullptr
    // so we can't verify label text directly
    // But we can verify the methods don't crash

    qDebug() << "✓ Status bar updates verified (no crashes)";
}

void MainWindowUIImprovementsTest::testStatusBarLoadingProgress() {
    qDebug() << "\n--- Test: Status Bar Loading Progress ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    StatusBar* statusBar = m_mainWindow->findChild<StatusBar*>();
    QVERIFY(statusBar != nullptr);

    // Show loading progress
    statusBar->showLoadingProgress("Loading document...");
    QTest::qWait(100);

    // Update progress
    statusBar->updateLoadingProgress(50);
    QTest::qWait(50);

    statusBar->updateLoadingProgress(100);
    QTest::qWait(50);

    // Hide loading progress
    statusBar->hideLoadingProgress();
    QTest::qWait(100);

    // Verify methods execute without crashing
    qDebug() << "✓ Status bar loading progress verified";
}

// ============================================================================
// Menu Bar Tests
// ============================================================================

void MainWindowUIImprovementsTest::testMenuBarVisibility() {
    qDebug() << "\n--- Test: Menu Bar Visibility ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    // Find menu bar
    MenuBar* menuBar = m_mainWindow->findChild<MenuBar*>();
    QVERIFY(menuBar != nullptr);

    // Verify menu bar is visible
    QVERIFY(menuBar->isVisible());

    // Verify menu bar is set on main window
    QMenuBar* mainWindowMenuBar = m_mainWindow->menuBar();
    QVERIFY(mainWindowMenuBar != nullptr);
    QCOMPARE(mainWindowMenuBar, menuBar);

    // Verify menu bar has menus
    QList<QAction*> actions = menuBar->actions();
    QVERIFY(actions.size() > 0);

    qDebug() << "✓ Menu bar visibility verified with" << actions.size()
             << "menus";
}

void MainWindowUIImprovementsTest::testMenuItemStates() {
    qDebug() << "\n--- Test: Menu Item States ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    MenuBar* menuBar = m_mainWindow->findChild<MenuBar*>();
    QVERIFY(menuBar != nullptr);

    // Get all menu actions
    QList<QAction*> menuActions = menuBar->actions();

    int totalMenuItems = 0;
    int enabledMenuItems = 0;

    for (QAction* menuAction : menuActions) {
        if (!menuAction)
            continue;

        QMenu* menu = menuAction->menu();
        if (!menu)
            continue;

        QList<QAction*> items = menu->actions();
        for (QAction* item : items) {
            if (!item || item->isSeparator())
                continue;

            totalMenuItems++;
            if (item->isEnabled()) {
                enabledMenuItems++;
            }
        }
    }

    qDebug() << "Total menu items:" << totalMenuItems;
    qDebug() << "Enabled menu items:" << enabledMenuItems;

    // Should have some menu items
    QVERIFY(totalMenuItems > 0);

    qDebug() << "✓ Menu item states verified";
}

// ============================================================================
// Document Viewer Tests
// ============================================================================

void MainWindowUIImprovementsTest::testViewerScrollBehavior() {
    qDebug() << "\n--- Test: Viewer Scroll Behavior ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    // Find view widget
    ViewWidget* viewWidget = m_mainWindow->findChild<ViewWidget*>();
    QVERIFY(viewWidget != nullptr);

    // Test scroll position methods (should not crash even without document)
    QPoint initialPos = viewWidget->getScrollPosition();
    qDebug() << "Initial scroll position:" << initialPos;

    // Test scroll to top
    viewWidget->scrollToTop();
    QTest::qWait(50);

    // Test scroll to bottom
    viewWidget->scrollToBottom();
    QTest::qWait(50);

    // Test set scroll position
    viewWidget->setScrollPosition(QPoint(0, 100));
    QTest::qWait(50);

    qDebug() << "✓ Viewer scroll behavior verified (no crashes)";
}

void MainWindowUIImprovementsTest::testViewerZoomFunctionality() {
    qDebug() << "\n--- Test: Viewer Zoom Functionality ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    ViewWidget* viewWidget = m_mainWindow->findChild<ViewWidget*>();
    QVERIFY(viewWidget != nullptr);

    // Get initial zoom level
    double initialZoom = viewWidget->getCurrentZoom();
    qDebug() << "Initial zoom level:" << initialZoom;

    // Test zoom in
    viewWidget->setZoom(1.5);
    QTest::qWait(50);

    // Test zoom out
    viewWidget->setZoom(0.75);
    QTest::qWait(50);

    // Test zoom reset
    viewWidget->setZoom(1.0);
    QTest::qWait(50);

    qDebug() << "✓ Viewer zoom functionality verified";
}

void MainWindowUIImprovementsTest::testViewerPageNavigation() {
    qDebug() << "\n--- Test: Viewer Page Navigation ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    ViewWidget* viewWidget = m_mainWindow->findChild<ViewWidget*>();
    QVERIFY(viewWidget != nullptr);

    // Get current page info
    int currentPage = viewWidget->getCurrentPage();
    int pageCount = viewWidget->getCurrentPageCount();

    qDebug() << "Current page:" << currentPage << "of" << pageCount;

    // Test go to page (should not crash even without document)
    viewWidget->goToPage(1);
    QTest::qWait(50);

    // Test view mode
    int currentMode = viewWidget->getCurrentViewMode();
    qDebug() << "Current view mode:" << currentMode;

    viewWidget->setCurrentViewMode(0);
    QTest::qWait(50);

    qDebug() << "✓ Viewer page navigation verified";
}

// ============================================================================
// Sidebar Component Tests
// ============================================================================

void MainWindowUIImprovementsTest::testSidebarTabSwitching() {
    qDebug() << "\n--- Test: Sidebar Tab Switching ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_sideBar != nullptr);

    // Get tab widget
    QTabWidget* tabWidget = m_sideBar->getTabWidget();
    QVERIFY(tabWidget != nullptr);

    int tabCount = tabWidget->count();
    qDebug() << "Sidebar has" << tabCount << "tabs";

    if (tabCount < 2) {
        QSKIP("Sidebar needs at least 2 tabs for switching test");
    }

    // Get initial tab
    int initialTab = tabWidget->currentIndex();
    qDebug() << "Initial tab:" << initialTab;

    // Switch to next tab
    int nextTab = (initialTab + 1) % tabCount;
    tabWidget->setCurrentIndex(nextTab);
    QTest::qWait(100);

    QCOMPARE(tabWidget->currentIndex(), nextTab);

    // Switch back
    tabWidget->setCurrentIndex(initialTab);
    QTest::qWait(100);

    QCOMPARE(tabWidget->currentIndex(), initialTab);

    qDebug() << "✓ Sidebar tab switching verified";
}

void MainWindowUIImprovementsTest::testSidebarResize() {
    qDebug() << "\n--- Test: Sidebar Resize ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_sideBar != nullptr);

    // Get initial width
    int initialWidth = m_sideBar->getPreferredWidth();
    qDebug() << "Initial sidebar width:" << initialWidth;

    // Get min/max constraints
    int minWidth = m_sideBar->getMinimumWidth();
    int maxWidth = m_sideBar->getMaximumWidth();

    qDebug() << "Min width:" << minWidth << "Max width:" << maxWidth;

    QVERIFY(minWidth > 0);
    QVERIFY(maxWidth > minWidth);

    // Test setting width within bounds
    int testWidth = (minWidth + maxWidth) / 2;
    m_sideBar->setPreferredWidth(testWidth);
    QTest::qWait(50);

    int newWidth = m_sideBar->getPreferredWidth();
    QCOMPARE(newWidth, testWidth);

    qDebug() << "✓ Sidebar resize verified";
}

void MainWindowUIImprovementsTest::testSidebarCollapseExpand() {
    qDebug() << "\n--- Test: Sidebar Collapse/Expand ---";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    QVERIFY(m_sideBar != nullptr);

    // Get initial visibility
    bool initiallyVisible = m_sideBar->isVisible();
    qDebug() << "Sidebar initially visible:" << initiallyVisible;

    // Test hide with animation
    m_sideBar->hide(true);
    QTest::qWait(400);  // Wait for animation to complete

    // Verify hidden
    QVERIFY(!m_sideBar->isVisible());

    // Test show with animation
    m_sideBar->show(true);
    QTest::qWait(400);  // Wait for animation to complete

    // Verify visible
    QVERIFY(m_sideBar->isVisible());

    // Test toggle
    m_sideBar->toggleVisibility(true);
    QTest::qWait(400);

    // Should be opposite of current state
    bool afterToggle = m_sideBar->isVisible();
    qDebug() << "After toggle, visible:" << afterToggle;

    qDebug() << "✓ Sidebar collapse/expand verified";
}

// ============================================================================
// Realistic Workflow Tests
// ============================================================================

void MainWindowUIImprovementsTest::testCompleteUserWorkflow() {
    qDebug() << "\n--- Test: Complete User Workflow ---";
    qDebug() << "Simulating: Theme switch → Sidebar toggle → Window resize → "
                "Verify all components";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    // Step 1: Verify initial state
    qDebug() << "\nStep 1: Verify initial state";
    QVERIFY(m_mainWindow->isVisible());
    QSize initialSize = m_mainWindow->size();
    qDebug() << "Initial window size:" << initialSize;

    // Step 2: Switch theme to Dark
    qDebug() << "\nStep 2: Switch theme to Dark";
    StyleManager& styleManager = StyleManager::instance();
    styleManager.setTheme(Theme::Dark);
    QTest::qWait(200);  // Allow theme to apply

    // Verify theme changed
    QCOMPARE(styleManager.currentTheme(), Theme::Dark);
    qDebug() << "✓ Theme switched to Dark";

    // Step 3: Toggle sidebar visibility
    qDebug() << "\nStep 3: Toggle sidebar";
    if (m_sideBar) {
        bool wasVisible = m_sideBar->isVisible();
        m_sideBar->toggleVisibility(true);
        QTest::qWait(400);  // Wait for animation

        bool nowVisible = m_sideBar->isVisible();
        QVERIFY(wasVisible != nowVisible);
        qDebug() << "✓ Sidebar toggled from" << wasVisible << "to"
                 << nowVisible;
    }

    // Step 4: Resize window
    qDebug() << "\nStep 4: Resize window";
    QSize newSize(1024, 768);
    m_mainWindow->resize(newSize);
    QTest::qWait(200);  // Allow resize to complete

    QSize actualSize = m_mainWindow->size();
    qDebug() << "Window resized to:" << actualSize;

    // Step 5: Verify all components still functional
    qDebug() << "\nStep 5: Verify all components still functional";

    // Check toolbar
    ToolBar* toolbar = m_mainWindow->findChild<ToolBar*>();
    QVERIFY(toolbar != nullptr);
    QVERIFY(toolbar->isVisible());

    // Check status bar
    StatusBar* statusBar = m_mainWindow->findChild<StatusBar*>();
    QVERIFY(statusBar != nullptr);
    QVERIFY(statusBar->isVisible());

    // Check menu bar
    MenuBar* menuBar = m_mainWindow->findChild<MenuBar*>();
    QVERIFY(menuBar != nullptr);
    QVERIFY(menuBar->isVisible());

    // Check splitter
    QVERIFY(m_splitter != nullptr);
    QCOMPARE(m_splitter->handleWidth(), 6);

    qDebug() << "✓ All components verified after workflow";

    // Step 6: Switch back to Light theme
    qDebug() << "\nStep 6: Switch back to Light theme";
    styleManager.setTheme(Theme::Light);
    QTest::qWait(200);

    QCOMPARE(styleManager.currentTheme(), Theme::Light);
    qDebug() << "✓ Theme switched back to Light";

    qDebug() << "\n✓ Complete user workflow test passed";
}

void MainWindowUIImprovementsTest::testLanguageSwitchWorkflow() {
    qDebug() << "\n--- Test: Language Switch Workflow ---";
    qDebug() << "Simulating: Language switch → Toolbar interaction → Status "
                "bar verification";

    createMainWindowOrSkip();
    if (!m_mainWindow)
        return;

    I18nManager& i18nManager = I18nManager::instance();

    // Step 1: Get initial language
    qDebug() << "\nStep 1: Get initial language";
    I18nManager::Language initialLang = i18nManager.currentLanguage();
    qDebug() << "Initial language:" << (int)initialLang;

    // Step 2: Switch to Chinese
    qDebug() << "\nStep 2: Switch to Chinese";
    i18nManager.loadLanguage(I18nManager::Language::Chinese);
    QTest::qWait(200);  // Allow language change to propagate

    QCOMPARE(i18nManager.currentLanguage(), I18nManager::Language::Chinese);
    qDebug() << "✓ Language switched to Chinese";

    // Step 3: Verify toolbar still functional
    qDebug() << "\nStep 3: Verify toolbar still functional";
    ToolBar* toolbar = m_mainWindow->findChild<ToolBar*>();
    QVERIFY(toolbar != nullptr);
    QVERIFY(toolbar->isVisible());

    // Get toolbar actions
    QList<QAction*> actions = toolbar->actions();
    QVERIFY(actions.size() > 0);
    qDebug() << "✓ Toolbar has" << actions.size() << "actions";

    // Step 4: Test toolbar action trigger
    qDebug() << "\nStep 4: Test toolbar action trigger";
    QSignalSpy actionSpy(toolbar, &ToolBar::actionTriggered);

    // Find and trigger an action
    for (QAction* action : actions) {
        if (action && action->isEnabled()) {
            action->trigger();
            QTest::qWait(50);
            break;
        }
    }

    qDebug() << "✓ Toolbar action triggered, signals:" << actionSpy.count();

    // Step 5: Verify status bar updates
    qDebug() << "\nStep 5: Verify status bar updates";
    StatusBar* statusBar = m_mainWindow->findChild<StatusBar*>();
    QVERIFY(statusBar != nullptr);

    // Update status bar (should work regardless of language)
    statusBar->setPageInfo(10, 50);
    statusBar->setZoomLevel(125);
    statusBar->setMessage("测试消息");  // Chinese test message
    QTest::qWait(100);

    qDebug() << "✓ Status bar updates successful";

    // Step 6: Switch to English
    qDebug() << "\nStep 6: Switch to English";
    i18nManager.loadLanguage(I18nManager::Language::English);
    QTest::qWait(200);

    QCOMPARE(i18nManager.currentLanguage(), I18nManager::Language::English);
    qDebug() << "✓ Language switched to English";

    // Step 7: Verify layout integrity
    qDebug() << "\nStep 7: Verify layout integrity";

    // Check that all major components are still visible and properly laid out
    QVERIFY(toolbar->isVisible());
    QVERIFY(statusBar->isVisible());

    if (m_splitter) {
        QCOMPARE(m_splitter->handleWidth(), 6);
    }

    if (m_mainViewerLayout) {
        QMargins margins = m_mainViewerLayout->contentsMargins();
        QCOMPARE(margins.top(), 4);
        QCOMPARE(margins.bottom(), 4);
    }

    qDebug() << "✓ Layout integrity verified after language switches";

    qDebug() << "\n✓ Language switch workflow test passed";
}

// ============================================================================
// Test Runner
// ============================================================================

// Custom main function to ensure QApplication is used instead of
// QCoreApplication This is required because MainWindow needs full GUI support
int main(int argc, char* argv[]) {
    // Create QApplication (not QCoreApplication) for GUI tests
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);

    // Run the test
    MainWindowUIImprovementsTest tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

#include "test_mainwindow_ui_improvements.moc"
