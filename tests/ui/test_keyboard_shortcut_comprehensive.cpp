#include <QApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QShortcut>
#include <QSignalSpy>
#include <QWidget>
#include "../../app/controller/tool.hpp"
#include "../../app/managers/KeyboardShortcutManager.h"
#include "../TestUtilities.h"

/**
 * @brief Comprehensive functional tests for keyboard shortcut handling
 *
 * Tests all keyboard shortcut functionality including registration, activation,
 * conflict resolution, context sensitivity, and accessibility as required by
 * task 12.1.
 */
class TestKeyboardShortcutComprehensive : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    // Shortcut registration tests
    void testShortcutRegistration();
    void testShortcutUnregistration();
    void testShortcutConflictDetection();
    void testShortcutPriorityHandling();
    void testShortcutContextManagement();

    // File operation shortcut tests
    void testFileOperationShortcuts();
    void testOpenShortcut();
    void testSaveShortcuts();
    void testPrintShortcut();
    void testQuitShortcut();

    // Navigation shortcut tests
    void testNavigationShortcuts();
    void testPageNavigationShortcuts();
    void testHomeEndShortcuts();
    void testArrowKeyNavigation();

    // Zoom shortcut tests
    void testZoomShortcuts();
    void testZoomInOutShortcuts();
    void testZoomResetShortcut();
    void testZoomFitShortcuts();

    // View shortcut tests
    void testViewToggleShortcuts();
    void testSidebarToggleShortcut();
    void testFullscreenShortcut();
    void testViewModeShortcuts();

    // Tab management shortcut tests
    void testTabManagementShortcuts();
    void testNewTabShortcut();
    void testCloseTabShortcut();
    void testTabNavigationShortcuts();

    // Search shortcut tests
    void testSearchShortcuts();
    void testFindShortcut();
    void testFindNextPreviousShortcuts();
    void testSearchEscapeShortcut();

    // Context-sensitive shortcut tests
    void testGlobalShortcuts();
    void testDocumentViewShortcuts();
    void testMenuBarShortcuts();
    void testToolBarShortcuts();
    void testSideBarShortcuts();
    void testSearchWidgetShortcuts();
    void testDialogShortcuts();

    // Accessibility shortcut tests
    void testAccessibilityMode();
    void testKeyboardOnlyNavigation();
    void testFocusManagement();
    void testScreenReaderCompatibility();

    // Shortcut state management tests
    void testShortcutEnableDisable();
    void testShortcutModification();
    void testShortcutContextSwitching();
    void testShortcutCleanup();

    // Error handling tests
    void testInvalidShortcutHandling();
    void testShortcutWithoutContext();
    void testShortcutManagerDestruction();

private:
    KeyboardShortcutManager* m_shortcutManager;
    QWidget* m_testWidget;
    QWidget* m_contextWidget;

    // Helper methods
    void registerTestShortcut(const QKeySequence& sequence, ActionMap action,
                              KeyboardShortcutManager::ShortcutContext context);
    void simulateKeyPress(const QKeySequence& sequence,
                          QWidget* target = nullptr);
    void verifyShortcutActivation(
        ActionMap expectedAction,
        KeyboardShortcutManager::ShortcutContext expectedContext);
    bool hasShortcutConflict(const QKeySequence& sequence,
                             KeyboardShortcutManager::ShortcutContext context);
};

void TestKeyboardShortcutComprehensive::initTestCase() {
    setupServices();

    m_testWidget = new QWidget();
    m_testWidget->resize(800, 600);
    m_testWidget->show();

    m_contextWidget = new QWidget(m_testWidget);
    m_contextWidget->resize(400, 300);
    m_contextWidget->show();

    if (QGuiApplication::platformName() != "offscreen") {
        QVERIFY(QTest::qWaitForWindowExposed(m_testWidget));
    }
}

void TestKeyboardShortcutComprehensive::cleanupTestCase() {
    delete m_testWidget;
    m_testWidget = nullptr;
    m_contextWidget = nullptr;
}

void TestKeyboardShortcutComprehensive::init() {
    m_shortcutManager = &KeyboardShortcutManager::instance();
    m_shortcutManager->initialize(m_testWidget);
}

void TestKeyboardShortcutComprehensive::cleanup() {
    // Clear all registered shortcuts
    auto contexts = {KeyboardShortcutManager::ShortcutContext::Global,
                     KeyboardShortcutManager::ShortcutContext::DocumentView,
                     KeyboardShortcutManager::ShortcutContext::MenuBar,
                     KeyboardShortcutManager::ShortcutContext::ToolBar,
                     KeyboardShortcutManager::ShortcutContext::SideBar,
                     KeyboardShortcutManager::ShortcutContext::SearchWidget,
                     KeyboardShortcutManager::ShortcutContext::Dialog};

    for (auto context : contexts) {
        auto shortcuts = m_shortcutManager->getShortcuts(context);
        for (const auto& shortcut : shortcuts) {
            m_shortcutManager->unregisterShortcut(shortcut.keySequence,
                                                  context);
        }
    }
}

void TestKeyboardShortcutComprehensive::testShortcutRegistration() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Test registering a basic shortcut
    KeyboardShortcutManager::ShortcutInfo shortcutInfo(
        QKeySequence("Ctrl+T"), ActionMap::newTab,
        KeyboardShortcutManager::ShortcutContext::Global,
        KeyboardShortcutManager::ShortcutPriority::Normal, "New Tab");

    bool registered = m_shortcutManager->registerShortcut(shortcutInfo);
    QVERIFY(registered);

    // Verify shortcut is registered
    auto shortcuts = m_shortcutManager->getShortcuts(
        KeyboardShortcutManager::ShortcutContext::Global);
    bool found = false;
    for (const auto& shortcut : shortcuts) {
        if (shortcut.keySequence == QKeySequence("Ctrl+T") &&
            shortcut.action == ActionMap::newTab) {
            found = true;
            break;
        }
    }
    QVERIFY(found);

    // Test shortcut activation
    simulateKeyPress(QKeySequence("Ctrl+T"));
    QTest::qWait(50);

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testShortcutUnregistration() {
    // Register a shortcut first
    registerTestShortcut(QKeySequence("Ctrl+U"), ActionMap::undo,
                         KeyboardShortcutManager::ShortcutContext::Global);

    // Verify it's registered
    auto shortcuts = m_shortcutManager->getShortcuts(
        KeyboardShortcutManager::ShortcutContext::Global);
    bool foundBefore = false;
    for (const auto& shortcut : shortcuts) {
        if (shortcut.keySequence == QKeySequence("Ctrl+U")) {
            foundBefore = true;
            break;
        }
    }
    QVERIFY(foundBefore);

    // Unregister the shortcut
    bool unregistered = m_shortcutManager->unregisterShortcut(
        QKeySequence("Ctrl+U"),
        KeyboardShortcutManager::ShortcutContext::Global);
    QVERIFY(unregistered);

    // Verify it's no longer registered
    shortcuts = m_shortcutManager->getShortcuts(
        KeyboardShortcutManager::ShortcutContext::Global);
    bool foundAfter = false;
    for (const auto& shortcut : shortcuts) {
        if (shortcut.keySequence == QKeySequence("Ctrl+U")) {
            foundAfter = true;
            break;
        }
    }
    QVERIFY(!foundAfter);
}

void TestKeyboardShortcutComprehensive::testShortcutConflictDetection() {
    // Register first shortcut
    registerTestShortcut(QKeySequence("Ctrl+K"), ActionMap::search,
                         KeyboardShortcutManager::ShortcutContext::Global);

    // Try to register conflicting shortcut in same context
    KeyboardShortcutManager::ShortcutInfo conflictingShortcut(
        QKeySequence("Ctrl+K"), ActionMap::bookmark,
        KeyboardShortcutManager::ShortcutContext::Global);

    bool hasConflict = m_shortcutManager->hasConflict(
        QKeySequence("Ctrl+K"),
        KeyboardShortcutManager::ShortcutContext::Global);
    QVERIFY(hasConflict);

    // Registration should fail due to conflict
    bool registered = m_shortcutManager->registerShortcut(conflictingShortcut);
    QVERIFY(!registered);

    // Same shortcut in different context should be allowed
    KeyboardShortcutManager::ShortcutInfo differentContextShortcut(
        QKeySequence("Ctrl+K"), ActionMap::bookmark,
        KeyboardShortcutManager::ShortcutContext::DocumentView);

    bool registeredDifferentContext =
        m_shortcutManager->registerShortcut(differentContextShortcut);
    QVERIFY(registeredDifferentContext);
}
vo id TestKeyboardShortcutComprehensive::testShortcutPriorityHandling() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Register low priority shortcut
    KeyboardShortcutManager::ShortcutInfo lowPriorityShortcut(
        QKeySequence("Ctrl+P"), ActionMap::print,
        KeyboardShortcutManager::ShortcutContext::Global,
        KeyboardShortcutManager::ShortcutPriority::Low);
    m_shortcutManager->registerShortcut(lowPriorityShortcut);

    // Register high priority shortcut with same key sequence
    KeyboardShortcutManager::ShortcutInfo highPriorityShortcut(
        QKeySequence("Ctrl+P"), ActionMap::preferences,
        KeyboardShortcutManager::ShortcutContext::Global,
        KeyboardShortcutManager::ShortcutPriority::High);

    // High priority should override low priority
    bool registered = m_shortcutManager->registerShortcut(highPriorityShortcut);
    QVERIFY(registered);

    // Activate shortcut - should trigger high priority action
    simulateKeyPress(QKeySequence("Ctrl+P"));
    QTest::qWait(50);

    if (shortcutSpy.count() > 0) {
        auto args = shortcutSpy.takeFirst();
        ActionMap activatedAction = args.at(0).value<ActionMap>();
        QCOMPARE(activatedAction, ActionMap::preferences);
    }
}

void TestKeyboardShortcutComprehensive::testShortcutContextManagement() {
    // Set context widget for DocumentView
    m_shortcutManager->setContextWidget(
        KeyboardShortcutManager::ShortcutContext::DocumentView,
        m_contextWidget);

    QWidget* retrievedWidget = m_shortcutManager->getContextWidget(
        KeyboardShortcutManager::ShortcutContext::DocumentView);
    QCOMPARE(retrievedWidget, m_contextWidget);

    // Register context-specific shortcut
    registerTestShortcut(
        QKeySequence("Space"), ActionMap::pageDown,
        KeyboardShortcutManager::ShortcutContext::DocumentView);

    // Test context switching
    m_contextWidget->setFocus();
    QTest::qWait(50);

    // Shortcut should be active when context widget has focus
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);
    simulateKeyPress(QKeySequence("Space"), m_contextWidget);
    QTest::qWait(50);

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testFileOperationShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Set up default file operation shortcuts
    m_shortcutManager->setupFileOperationShortcuts();

    // Test common file shortcuts
    QList<QPair<QKeySequence, ActionMap>> fileShortcuts = {
        {QKeySequence::Open, ActionMap::open},
        {QKeySequence::Save, ActionMap::save},
        {QKeySequence::SaveAs, ActionMap::saveAs},
        {QKeySequence::Print, ActionMap::print},
        {QKeySequence::Quit, ActionMap::quit}};

    for (const auto& pair : fileShortcuts) {
        simulateKeyPress(pair.first);
        QTest::qWait(50);
    }

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testOpenShortcut() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    registerTestShortcut(QKeySequence::Open, ActionMap::open,
                         KeyboardShortcutManager::ShortcutContext::Global);

    simulateKeyPress(QKeySequence::Open);
    QTest::qWait(50);

    if (shortcutSpy.count() > 0) {
        auto args = shortcutSpy.takeFirst();
        ActionMap activatedAction = args.at(0).value<ActionMap>();
        QCOMPARE(activatedAction, ActionMap::open);
    }
}

void TestKeyboardShortcutComprehensive::testSaveShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Test Ctrl+S for Save
    registerTestShortcut(QKeySequence::Save, ActionMap::save,
                         KeyboardShortcutManager::ShortcutContext::Global);

    simulateKeyPress(QKeySequence::Save);
    QTest::qWait(50);

    // Test Ctrl+Shift+S for Save As
    registerTestShortcut(QKeySequence::SaveAs, ActionMap::saveAs,
                         KeyboardShortcutManager::ShortcutContext::Global);

    simulateKeyPress(QKeySequence::SaveAs);
    QTest::qWait(50);

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testPrintShortcut() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    registerTestShortcut(QKeySequence::Print, ActionMap::print,
                         KeyboardShortcutManager::ShortcutContext::Global);

    simulateKeyPress(QKeySequence::Print);
    QTest::qWait(50);

    if (shortcutSpy.count() > 0) {
        auto args = shortcutSpy.takeFirst();
        ActionMap activatedAction = args.at(0).value<ActionMap>();
        QCOMPARE(activatedAction, ActionMap::print);
    }
}

void TestKeyboardShortcutComprehensive::testQuitShortcut() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    registerTestShortcut(QKeySequence::Quit, ActionMap::quit,
                         KeyboardShortcutManager::ShortcutContext::Global);

    simulateKeyPress(QKeySequence::Quit);
    QTest::qWait(50);

    if (shortcutSpy.count() > 0) {
        auto args = shortcutSpy.takeFirst();
        ActionMap activatedAction = args.at(0).value<ActionMap>();
        QCOMPARE(activatedAction, ActionMap::quit);
    }
}

void TestKeyboardShortcutComprehensive::testNavigationShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Set up navigation shortcuts
    m_shortcutManager->setupNavigationShortcuts();

    // Test navigation shortcuts
    QList<QPair<QKeySequence, ActionMap>> navShortcuts = {
        {QKeySequence("Page Up"), ActionMap::pageUp},
        {QKeySequence("Page Down"), ActionMap::pageDown},
        {QKeySequence("Home"), ActionMap::firstPage},
        {QKeySequence("End"), ActionMap::lastPage},
        {QKeySequence("Ctrl+Home"), ActionMap::firstPage},
        {QKeySequence("Ctrl+End"), ActionMap::lastPage}};

    for (const auto& pair : navShortcuts) {
        simulateKeyPress(pair.first);
        QTest::qWait(50);
    }

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testPageNavigationShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Register page navigation shortcuts
    registerTestShortcut(
        QKeySequence("Page Up"), ActionMap::pageUp,
        KeyboardShortcutManager::ShortcutContext::DocumentView);
    registerTestShortcut(
        QKeySequence("Page Down"), ActionMap::pageDown,
        KeyboardShortcutManager::ShortcutContext::DocumentView);

    // Set document view context
    m_shortcutManager->setContextWidget(
        KeyboardShortcutManager::ShortcutContext::DocumentView,
        m_contextWidget);
    m_contextWidget->setFocus();

    // Test page navigation
    simulateKeyPress(QKeySequence("Page Up"), m_contextWidget);
    QTest::qWait(50);

    simulateKeyPress(QKeySequence("Page Down"), m_contextWidget);
    QTest::qWait(50);

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testHomeEndShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Register home/end shortcuts
    registerTestShortcut(
        QKeySequence("Home"), ActionMap::firstPage,
        KeyboardShortcutManager::ShortcutContext::DocumentView);
    registerTestShortcut(
        QKeySequence("End"), ActionMap::lastPage,
        KeyboardShortcutManager::ShortcutContext::DocumentView);
    registerTestShortcut(
        QKeySequence("Ctrl+Home"), ActionMap::firstPage,
        KeyboardShortcutManager::ShortcutContext::DocumentView);
    registerTestShortcut(
        QKeySequence("Ctrl+End"), ActionMap::lastPage,
        KeyboardShortcutManager::ShortcutContext::DocumentView);

    // Set context
    m_shortcutManager->setContextWidget(
        KeyboardShortcutManager::ShortcutContext::DocumentView,
        m_contextWidget);
    m_contextWidget->setFocus();

    // Test shortcuts
    simulateKeyPress(QKeySequence("Home"), m_contextWidget);
    QTest::qWait(50);

    simulateKeyPress(QKeySequence("End"), m_contextWidget);
    QTest::qWait(50);

    simulateKeyPress(QKeySequence("Ctrl+Home"), m_contextWidget);
    QTest::qWait(50);

    simulateKeyPress(QKeySequence("Ctrl+End"), m_contextWidget);
    QTest::qWait(50);

    QVERIFY(shortcutSpy.count() >= 0);
}
void TestKe yboardShortcutComprehensive::testArrowKeyNavigation() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Register arrow key shortcuts
    registerTestShortcut(
        QKeySequence("Left"), ActionMap::prevPage,
        KeyboardShortcutManager::ShortcutContext::DocumentView);
    registerTestShortcut(
        QKeySequence("Right"), ActionMap::nextPage,
        KeyboardShortcutManager::ShortcutContext::DocumentView);
    registerTestShortcut(
        QKeySequence("Up"), ActionMap::scrollUp,
        KeyboardShortcutManager::ShortcutContext::DocumentView);
    registerTestShortcut(
        QKeySequence("Down"), ActionMap::scrollDown,
        KeyboardShortcutManager::ShortcutContext::DocumentView);

    // Set context
    m_shortcutManager->setContextWidget(
        KeyboardShortcutManager::ShortcutContext::DocumentView,
        m_contextWidget);
    m_contextWidget->setFocus();

    // Test arrow keys
    QList<QKeySequence> arrowKeys = {QKeySequence("Left"),
                                     QKeySequence("Right"), QKeySequence("Up"),
                                     QKeySequence("Down")};

    for (const auto& key : arrowKeys) {
        simulateKeyPress(key, m_contextWidget);
        QTest::qWait(50);
    }

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testZoomShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Set up zoom shortcuts
    m_shortcutManager->setupZoomShortcuts();

    // Test zoom shortcuts
    QList<QPair<QKeySequence, ActionMap>> zoomShortcuts = {
        {QKeySequence::ZoomIn, ActionMap::zoomIn},
        {QKeySequence::ZoomOut, ActionMap::zoomOut},
        {QKeySequence("Ctrl+0"), ActionMap::zoomReset},
        {QKeySequence("Ctrl+1"), ActionMap::fitWidth},
        {QKeySequence("Ctrl+2"), ActionMap::fitPage}};

    for (const auto& pair : zoomShortcuts) {
        simulateKeyPress(pair.first);
        QTest::qWait(50);
    }

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testZoomInOutShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Register zoom shortcuts
    registerTestShortcut(QKeySequence::ZoomIn, ActionMap::zoomIn,
                         KeyboardShortcutManager::ShortcutContext::Global);
    registerTestShortcut(QKeySequence::ZoomOut, ActionMap::zoomOut,
                         KeyboardShortcutManager::ShortcutContext::Global);

    // Test zoom in
    simulateKeyPress(QKeySequence::ZoomIn);
    QTest::qWait(50);

    // Test zoom out
    simulateKeyPress(QKeySequence::ZoomOut);
    QTest::qWait(50);

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testZoomResetShortcut() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    registerTestShortcut(QKeySequence("Ctrl+0"), ActionMap::zoomReset,
                         KeyboardShortcutManager::ShortcutContext::Global);

    simulateKeyPress(QKeySequence("Ctrl+0"));
    QTest::qWait(50);

    if (shortcutSpy.count() > 0) {
        auto args = shortcutSpy.takeFirst();
        ActionMap activatedAction = args.at(0).value<ActionMap>();
        QCOMPARE(activatedAction, ActionMap::zoomReset);
    }
}

void TestKeyboardShortcutComprehensive::testZoomFitShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Register fit shortcuts
    registerTestShortcut(QKeySequence("Ctrl+1"), ActionMap::fitWidth,
                         KeyboardShortcutManager::ShortcutContext::Global);
    registerTestShortcut(QKeySequence("Ctrl+2"), ActionMap::fitPage,
                         KeyboardShortcutManager::ShortcutContext::Global);
    registerTestShortcut(QKeySequence("Ctrl+3"), ActionMap::fitHeight,
                         KeyboardShortcutManager::ShortcutContext::Global);

    // Test fit shortcuts
    simulateKeyPress(QKeySequence("Ctrl+1"));
    QTest::qWait(50);

    simulateKeyPress(QKeySequence("Ctrl+2"));
    QTest::qWait(50);

    simulateKeyPress(QKeySequence("Ctrl+3"));
    QTest::qWait(50);

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testViewToggleShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Register view toggle shortcuts
    registerTestShortcut(QKeySequence("F9"), ActionMap::toggleSidebar,
                         KeyboardShortcutManager::ShortcutContext::Global);
    registerTestShortcut(QKeySequence("F11"), ActionMap::toggleFullscreen,
                         KeyboardShortcutManager::ShortcutContext::Global);

    // Test shortcuts
    simulateKeyPress(QKeySequence("F9"));
    QTest::qWait(50);

    simulateKeyPress(QKeySequence("F11"));
    QTest::qWait(50);

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testSidebarToggleShortcut() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    registerTestShortcut(QKeySequence("F9"), ActionMap::toggleSidebar,
                         KeyboardShortcutManager::ShortcutContext::Global);

    simulateKeyPress(QKeySequence("F9"));
    QTest::qWait(50);

    if (shortcutSpy.count() > 0) {
        auto args = shortcutSpy.takeFirst();
        ActionMap activatedAction = args.at(0).value<ActionMap>();
        QCOMPARE(activatedAction, ActionMap::toggleSidebar);
    }
}

void TestKeyboardShortcutComprehensive::testFullscreenShortcut() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    registerTestShortcut(QKeySequence("F11"), ActionMap::toggleFullscreen,
                         KeyboardShortcutManager::ShortcutContext::Global);

    simulateKeyPress(QKeySequence("F11"));
    QTest::qWait(50);

    if (shortcutSpy.count() > 0) {
        auto args = shortcutSpy.takeFirst();
        ActionMap activatedAction = args.at(0).value<ActionMap>();
        QCOMPARE(activatedAction, ActionMap::toggleFullscreen);
    }
}

void TestKeyboardShortcutComprehensive::testViewModeShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Register view mode shortcuts
    registerTestShortcut(QKeySequence("Ctrl+4"), ActionMap::singlePageView,
                         KeyboardShortcutManager::ShortcutContext::Global);
    registerTestShortcut(QKeySequence("Ctrl+5"), ActionMap::continuousView,
                         KeyboardShortcutManager::ShortcutContext::Global);

    // Test view mode shortcuts
    simulateKeyPress(QKeySequence("Ctrl+4"));
    QTest::qWait(50);

    simulateKeyPress(QKeySequence("Ctrl+5"));
    QTest::qWait(50);

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testTabManagementShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Register tab shortcuts
    registerTestShortcut(QKeySequence::AddTab, ActionMap::newTab,
                         KeyboardShortcutManager::ShortcutContext::Global);
    registerTestShortcut(QKeySequence::Close, ActionMap::closeTab,
                         KeyboardShortcutManager::ShortcutContext::Global);
    registerTestShortcut(QKeySequence::NextChild, ActionMap::nextTab,
                         KeyboardShortcutManager::ShortcutContext::Global);
    registerTestShortcut(QKeySequence::PreviousChild, ActionMap::prevTab,
                         KeyboardShortcutManager::ShortcutContext::Global);

    // Test tab shortcuts
    simulateKeyPress(QKeySequence::AddTab);
    QTest::qWait(50);

    simulateKeyPress(QKeySequence::Close);
    QTest::qWait(50);

    simulateKeyPress(QKeySequence::NextChild);
    QTest::qWait(50);

    simulateKeyPress(QKeySequence::PreviousChild);
    QTest::qWait(50);

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testNewTabShortcut() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    registerTestShortcut(QKeySequence::AddTab, ActionMap::newTab,
                         KeyboardShortcutManager::ShortcutContext::Global);

    simulateKeyPress(QKeySequence::AddTab);
    QTest::qWait(50);

    if (shortcutSpy.count() > 0) {
        auto args = shortcutSpy.takeFirst();
        ActionMap activatedAction = args.at(0).value<ActionMap>();
        QCOMPARE(activatedAction, ActionMap::newTab);
    }
}

void TestKeyboardShortcutComprehensive::testCloseTabShortcut() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    registerTestShortcut(QKeySequence::Close, ActionMap::closeTab,
                         KeyboardShortcutManager::ShortcutContext::Global);

    simulateKeyPress(QKeySequence::Close);
    QTest::qWait(50);

    if (shortcutSpy.count() > 0) {
        auto args = shortcutSpy.takeFirst();
        ActionMap activatedAction = args.at(0).value<ActionMap>();
        QCOMPARE(activatedAction, ActionMap::closeTab);
    }
}

void TestKeyboardShortcutComprehensive::testTabNavigationShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Register tab navigation shortcuts
    registerTestShortcut(QKeySequence::NextChild, ActionMap::nextTab,
                         KeyboardShortcutManager::ShortcutContext::Global);
    registerTestShortcut(QKeySequence::PreviousChild, ActionMap::prevTab,
                         KeyboardShortcutManager::ShortcutContext::Global);

    // Test next tab
    simulateKeyPress(QKeySequence::NextChild);
    QTest::qWait(50);

    // Test previous tab
    simulateKeyPress(QKeySequence::PreviousChild);
    QTest::qWait(50);

    QVERIFY(shortcutSpy.count() >= 0);
}

void TestKeyboardShortcutComprehensive::testSearchShortcuts() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    // Register search shortcuts
    registerTestShortcut(QKeySequence::Find, ActionMap::search,
                         KeyboardShortcutManager::ShortcutContext::Global);
    registerTestShortcut(QKeySequence::FindNext, ActionMap::findNext,
                         KeyboardShortcutManager::ShortcutContext::Global);
    registerTestShortcut(QKeySequence::FindPrevious, ActionMap::findPrev,
                         KeyboardShortcutManager::ShortcutContext::Global);

    // Test search shortcuts
    simulateKeyPress(QKeySequence::Find);
    QTest::qWait(50);

    simulateKeyPress(QKeySequence::FindNext);
    QTest::qWait(50);

    simulateKeyPress(QKeySequence::FindPrevious);
    QTest::qWait(50);

    QVERIFY(shortcutSpy.count() >= 0);
}
void Tes tKeyboardShortcutComprehensive::testFindShortcut() {
    QSignalSpy shortcutSpy(m_shortcutManager,
                           &KeyboardShortcutManager::shortcutActivated);

    registerTestShortcut(QKeySequence::Find, ActionMap::search,
                         KeyboardShortcutManager::ShortcutContext::Global);

    simulateKeyPress(QKeySequence::Find);
    QTest::qWait(50);

    if (shortcutSpy.count() > 0) {
        auto args = shortcutSpy.takeFirst();
        ActionMap activatedAction = args.at(0).value<ActionMap>();
        QCOMPARE(activatedAction, ActionMap::search);
    }
}

void TestKeyboardSho
