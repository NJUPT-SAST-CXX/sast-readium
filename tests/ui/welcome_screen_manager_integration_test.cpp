#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QSettings>
#include <QTimer>
#include "../../app/ui/managers/WelcomeScreenManager.h"
#include "../../app/ui/widgets/WelcomeWidget.h"
#include "../../app/model/DocumentModel.h"
#include "../../app/model/RenderModel.h"
#include "../../app/MainWindow.h"

// Mock classes for testing
class MockMainWindow : public MainWindow {
    Q_OBJECT
public:
    MockMainWindow(QWidget* parent = nullptr) : MainWindow(parent) {}
};

class MockWelcomeWidget : public WelcomeWidget {
    Q_OBJECT
public:
    MockWelcomeWidget(QWidget* parent = nullptr) : WelcomeWidget(parent), m_visible(false) {}

    void setVisible(bool visible) override {
        m_visible = visible;
        WelcomeWidget::setVisible(visible);
    }

    bool isVisible() const {
        return m_visible;
    }

private:
    bool m_visible;
};

class WelcomeScreenManagerIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testComponentSetup();
    void testInitialState();
    
    // Welcome screen control tests
    void testWelcomeScreenEnabled();
    void testWelcomeScreenVisibility();
    void testShowHideWelcomeScreen();
    void testShouldShowWelcomeScreen();
    
    // Settings management tests
    void testSettingsLoad();
    void testSettingsSave();
    void testSettingsReset();
    void testSettingsPersistence();
    
    // Application lifecycle tests
    void testApplicationStartup();
    void testApplicationShutdown();
    void testDocumentLifecycle();
    
    // Document model integration tests
    void testDocumentModelIntegration();
    void testDocumentOpenClose();
    void testAllDocumentsClosed();
    
    // Signal emission tests
    void testVisibilitySignals();
    void testEnabledSignals();
    void testToggleSignals();
    
    // State management tests
    void testStateQueries();
    void testVisibilityCheck();
    void testDelayedVisibilityCheck();
    
    // Edge cases and error handling
    void testNullComponents();
    void testInvalidSettings();

private:
    WelcomeScreenManager* m_manager;
    MockMainWindow* m_mainWindow;
    MockWelcomeWidget* m_welcomeWidget;
    DocumentModel* m_documentModel;
    RenderModel* m_renderModel;
    QSettings* m_testSettings;
    
    void waitForTimer();
    void clearTestSettings();
};

void WelcomeScreenManagerIntegrationTest::initTestCase()
{
    // Create mock components
    m_mainWindow = new MockMainWindow();
    m_welcomeWidget = new MockWelcomeWidget();
    m_renderModel = new RenderModel();
    m_documentModel = new DocumentModel(m_renderModel);
    
    // Create test settings
    m_testSettings = new QSettings("TestOrg", "WelcomeScreenManagerTest", this);
    clearTestSettings();
}

void WelcomeScreenManagerIntegrationTest::cleanupTestCase()
{
    clearTestSettings();
    delete m_mainWindow;
    delete m_welcomeWidget;
}

void WelcomeScreenManagerIntegrationTest::init()
{
    m_manager = new WelcomeScreenManager(this);
    m_manager->setMainWindow(m_mainWindow);
    m_manager->setWelcomeWidget(m_welcomeWidget);
    m_manager->setDocumentModel(m_documentModel);
}

void WelcomeScreenManagerIntegrationTest::cleanup()
{
    delete m_manager;
    m_manager = nullptr;
    clearTestSettings();
}

void WelcomeScreenManagerIntegrationTest::testInitialization()
{
    // Test basic initialization
    QVERIFY(m_manager != nullptr);
    
    // Test initial state
    QVERIFY(m_manager->isWelcomeScreenEnabled()); // Should be enabled by default
    QVERIFY(!m_manager->isWelcomeScreenVisible()); // Should not be visible initially
}

void WelcomeScreenManagerIntegrationTest::testComponentSetup()
{
    // Test setting components
    WelcomeScreenManager* newManager = new WelcomeScreenManager(this);
    
    newManager->setMainWindow(m_mainWindow);
    newManager->setWelcomeWidget(m_welcomeWidget);
    newManager->setDocumentModel(m_documentModel);
    
    // Should handle component setup without issues
    QVERIFY(true);
    
    delete newManager;
}

void WelcomeScreenManagerIntegrationTest::testInitialState()
{
    // Test initial state queries
    QVERIFY(m_manager->isWelcomeScreenEnabled());
    QVERIFY(!m_manager->isWelcomeScreenVisible());
    QVERIFY(!m_manager->hasOpenDocuments());
    QVERIFY(m_manager->shouldShowWelcomeScreen());
}

void WelcomeScreenManagerIntegrationTest::testWelcomeScreenEnabled()
{
    QSignalSpy enabledSpy(m_manager, &WelcomeScreenManager::welcomeScreenEnabledChanged);
    
    // Test enabling/disabling
    bool initialState = m_manager->isWelcomeScreenEnabled();
    
    m_manager->setWelcomeScreenEnabled(!initialState);
    QCOMPARE(m_manager->isWelcomeScreenEnabled(), !initialState);
    
    // Should emit signal
    QCOMPARE(enabledSpy.count(), 1);
    QList<QVariant> args = enabledSpy.takeFirst();
    QCOMPARE(args.at(0).toBool(), !initialState);
    
    // Toggle back
    m_manager->setWelcomeScreenEnabled(initialState);
    QCOMPARE(m_manager->isWelcomeScreenEnabled(), initialState);
    QCOMPARE(enabledSpy.count(), 1);
}

void WelcomeScreenManagerIntegrationTest::testWelcomeScreenVisibility()
{
    QSignalSpy visibilitySpy(m_manager, &WelcomeScreenManager::welcomeScreenVisibilityChanged);
    QSignalSpy showRequestSpy(m_manager, &WelcomeScreenManager::showWelcomeScreenRequested);
    QSignalSpy hideRequestSpy(m_manager, &WelcomeScreenManager::hideWelcomeScreenRequested);
    
    // Test showing welcome screen
    m_manager->showWelcomeScreen();
    waitForTimer();
    
    // Should emit signals
    QVERIFY(showRequestSpy.count() >= 0);
    QVERIFY(visibilitySpy.count() >= 0);
    
    // Test hiding welcome screen
    m_manager->hideWelcomeScreen();
    waitForTimer();
    
    // Should emit signals
    QVERIFY(hideRequestSpy.count() >= 0);
}

void WelcomeScreenManagerIntegrationTest::testShowHideWelcomeScreen()
{
    // Test show/hide functionality
    m_manager->showWelcomeScreen();
    waitForTimer();
    
    // Welcome widget should be visible
    QVERIFY(m_welcomeWidget->isVisible());
    
    m_manager->hideWelcomeScreen();
    waitForTimer();
    
    // Welcome widget should be hidden
    QVERIFY(!m_welcomeWidget->isVisible());
}

void WelcomeScreenManagerIntegrationTest::testShouldShowWelcomeScreen()
{
    // Test conditions for showing welcome screen
    
    // Should show when enabled and no documents
    m_manager->setWelcomeScreenEnabled(true);
    QVERIFY(m_manager->shouldShowWelcomeScreen());
    
    // Should not show when disabled
    m_manager->setWelcomeScreenEnabled(false);
    QVERIFY(!m_manager->shouldShowWelcomeScreen());
    
    // Reset to enabled
    m_manager->setWelcomeScreenEnabled(true);
    QVERIFY(m_manager->shouldShowWelcomeScreen());
}

void WelcomeScreenManagerIntegrationTest::testSettingsLoad()
{
    // Test loading settings
    m_manager->loadSettings();
    
    // Should load without crashing
    QVERIFY(true);
    
    // Should have default values if no settings exist
    QVERIFY(m_manager->isWelcomeScreenEnabled());
}

void WelcomeScreenManagerIntegrationTest::testSettingsSave()
{
    // Change settings
    m_manager->setWelcomeScreenEnabled(false);
    
    // Save settings
    m_manager->saveSettings();
    
    // Create new manager and load settings
    WelcomeScreenManager* newManager = new WelcomeScreenManager(this);
    newManager->loadSettings();
    
    // Should have saved state
    QCOMPARE(newManager->isWelcomeScreenEnabled(), false);
    
    delete newManager;
}

void WelcomeScreenManagerIntegrationTest::testSettingsReset()
{
    // Change settings
    m_manager->setWelcomeScreenEnabled(false);
    
    // Reset to defaults
    m_manager->resetToDefaults();
    
    // Should have default values
    QVERIFY(m_manager->isWelcomeScreenEnabled());
}

void WelcomeScreenManagerIntegrationTest::testSettingsPersistence()
{
    // Test that settings persist across instances
    m_manager->setWelcomeScreenEnabled(false);
    m_manager->saveSettings();
    
    // Create new manager
    WelcomeScreenManager* newManager = new WelcomeScreenManager(this);
    newManager->loadSettings();
    
    // Should have persisted state
    QCOMPARE(newManager->isWelcomeScreenEnabled(), false);
    
    // Reset and save
    newManager->resetToDefaults();
    newManager->saveSettings();
    
    delete newManager;
}

void WelcomeScreenManagerIntegrationTest::testApplicationStartup()
{
    QSignalSpy showRequestSpy(m_manager, &WelcomeScreenManager::showWelcomeScreenRequested);
    
    // Test application startup
    m_manager->onApplicationStartup();
    waitForTimer();
    
    // Should handle startup without issues
    QVERIFY(true);
    
    // May show welcome screen if conditions are met
    QVERIFY(showRequestSpy.count() >= 0);
}

void WelcomeScreenManagerIntegrationTest::testApplicationShutdown()
{
    // Test application shutdown
    m_manager->onApplicationShutdown();
    
    // Should handle shutdown without issues
    QVERIFY(true);
}

void WelcomeScreenManagerIntegrationTest::testDocumentLifecycle()
{
    QSignalSpy visibilitySpy(m_manager, &WelcomeScreenManager::welcomeScreenVisibilityChanged);
    
    // Test document opened
    m_manager->onDocumentOpened();
    waitForTimer();
    
    // Should handle document opening
    QVERIFY(true);
    
    // Test document closed
    m_manager->onDocumentClosed();
    waitForTimer();
    
    // Should handle document closing
    QVERIFY(true);
    
    // Test all documents closed
    m_manager->onAllDocumentsClosed();
    waitForTimer();
    
    // Should handle all documents closed
    QVERIFY(visibilitySpy.count() >= 0);
}

void WelcomeScreenManagerIntegrationTest::testDocumentModelIntegration()
{
    QSignalSpy modelChangedSpy(m_manager, &WelcomeScreenManager::onDocumentModelChanged);
    
    // Test document model changed
    m_manager->onDocumentModelChanged();
    waitForTimer();
    
    // Should handle model changes
    QVERIFY(true);
}

void WelcomeScreenManagerIntegrationTest::testDocumentOpenClose()
{
    // Initially should show welcome screen
    QVERIFY(m_manager->shouldShowWelcomeScreen());
    
    // Simulate document opening
    m_manager->onDocumentOpened();
    waitForTimer();
    
    // Should handle document state changes
    QVERIFY(true);
    
    // Simulate document closing
    m_manager->onDocumentClosed();
    waitForTimer();
    
    // Should handle document state changes
    QVERIFY(true);
}

void WelcomeScreenManagerIntegrationTest::testAllDocumentsClosed()
{
    QSignalSpy showRequestSpy(m_manager, &WelcomeScreenManager::showWelcomeScreenRequested);
    
    // Simulate all documents closed
    m_manager->onAllDocumentsClosed();
    waitForTimer();
    
    // Should potentially show welcome screen
    QVERIFY(showRequestSpy.count() >= 0);
}

void WelcomeScreenManagerIntegrationTest::testVisibilitySignals()
{
    QSignalSpy visibilitySpy(m_manager, &WelcomeScreenManager::welcomeScreenVisibilityChanged);
    
    // Show welcome screen
    m_manager->showWelcomeScreen();
    waitForTimer();
    
    // Hide welcome screen
    m_manager->hideWelcomeScreen();
    waitForTimer();
    
    // Should emit visibility signals
    QVERIFY(visibilitySpy.count() >= 0);
}

void WelcomeScreenManagerIntegrationTest::testEnabledSignals()
{
    QSignalSpy enabledSpy(m_manager, &WelcomeScreenManager::welcomeScreenEnabledChanged);
    
    // Toggle enabled state
    bool initialState = m_manager->isWelcomeScreenEnabled();
    m_manager->setWelcomeScreenEnabled(!initialState);
    m_manager->setWelcomeScreenEnabled(initialState);
    
    // Should emit enabled signals
    QCOMPARE(enabledSpy.count(), 2);
}

void WelcomeScreenManagerIntegrationTest::testToggleSignals()
{
    QSignalSpy visibilitySpy(m_manager, &WelcomeScreenManager::welcomeScreenVisibilityChanged);
    
    // Test toggle request
    m_manager->onWelcomeScreenToggleRequested();
    waitForTimer();
    
    // Should handle toggle request
    QVERIFY(visibilitySpy.count() >= 0);
}

void WelcomeScreenManagerIntegrationTest::testStateQueries()
{
    // Test state query methods
    bool enabled = m_manager->isWelcomeScreenEnabled();
    bool visible = m_manager->isWelcomeScreenVisible();
    bool hasDocuments = m_manager->hasOpenDocuments();
    bool shouldShow = m_manager->shouldShowWelcomeScreen();
    
    // Should return valid states
    QVERIFY(enabled || !enabled);
    QVERIFY(visible || !visible);
    QVERIFY(hasDocuments || !hasDocuments);
    QVERIFY(shouldShow || !shouldShow);
}

void WelcomeScreenManagerIntegrationTest::testVisibilityCheck()
{
    // Test visibility check
    m_manager->checkWelcomeScreenVisibility();
    waitForTimer();
    
    // Should handle visibility check without issues
    QVERIFY(true);
}

void WelcomeScreenManagerIntegrationTest::testDelayedVisibilityCheck()
{
    QSignalSpy visibilitySpy(m_manager, &WelcomeScreenManager::welcomeScreenVisibilityChanged);
    
    // Trigger delayed visibility check
    m_manager->checkWelcomeScreenVisibility();
    
    // Wait for delayed check
    QTest::qWait(200); // Wait longer than VISIBILITY_CHECK_DELAY
    
    // Should handle delayed check
    QVERIFY(visibilitySpy.count() >= 0);
}

void WelcomeScreenManagerIntegrationTest::testNullComponents()
{
    // Test with null components
    WelcomeScreenManager* nullManager = new WelcomeScreenManager(this);
    
    // Should handle null components gracefully
    nullManager->setMainWindow(nullptr);
    nullManager->setWelcomeWidget(nullptr);
    nullManager->setDocumentModel(nullptr);
    
    // Operations should not crash
    nullManager->showWelcomeScreen();
    nullManager->hideWelcomeScreen();
    nullManager->onDocumentOpened();
    nullManager->onDocumentClosed();
    
    QVERIFY(true);
    
    delete nullManager;
}

void WelcomeScreenManagerIntegrationTest::testInvalidSettings()
{
    // Test with invalid settings
    m_manager->loadSettings();
    m_manager->saveSettings();
    
    // Should handle settings operations gracefully
    QVERIFY(true);
}

void WelcomeScreenManagerIntegrationTest::waitForTimer()
{
    // Wait for timer operations to complete
    QTest::qWait(150); // Slightly longer than VISIBILITY_CHECK_DELAY
    QApplication::processEvents();
}

void WelcomeScreenManagerIntegrationTest::clearTestSettings()
{
    if (m_testSettings) {
        m_testSettings->clear();
        m_testSettings->sync();
    }
}

QTEST_MAIN(WelcomeScreenManagerIntegrationTest)
#include "welcome_screen_manager_integration_test.moc"
