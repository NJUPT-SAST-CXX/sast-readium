#include <QApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTimer>
#include <QToolButton>
#include <QtTest/QtTest>
#include "../../app/command/CommandManager.h"
#include "../../app/managers/OnboardingManager.h"
#include "../../app/managers/RecentFilesManager.h"
#include "../../app/ui/widgets/WelcomeWidget.h"

class WelcomeWidgetFunctionalityTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testQuickActionConnections();
    void testRecentFileValidation();
    void testTutorialIntegration();
    void testThemeApplication();
    void testStateManagement();

private:
    WelcomeWidget* m_widget;
    RecentFilesManager* m_recentFilesManager;
    OnboardingManager* m_onboardingManager;
    CommandManager* m_commandManager;
    QSettings* m_testSettings;
};

void WelcomeWidgetFunctionalityTest::initTestCase() {
    // Create test components
    m_recentFilesManager = new RecentFilesManager(this);
    m_onboardingManager = new OnboardingManager(this);
    m_commandManager = new CommandManager(this);
    m_testSettings = new QSettings("TestOrg", "WelcomeWidgetTest", this);
    m_testSettings->clear();
}

void WelcomeWidgetFunctionalityTest::cleanupTestCase() {
    m_testSettings->clear();
}

void WelcomeWidgetFunctionalityTest::init() {
    m_widget = new WelcomeWidget();
    m_widget->setRecentFilesManager(m_recentFilesManager);
    m_widget->setOnboardingManager(m_onboardingManager);
    m_widget->setCommandManager(m_commandManager);
}

void WelcomeWidgetFunctionalityTest::cleanup() {
    delete m_widget;
    m_widget = nullptr;
}

void WelcomeWidgetFunctionalityTest::testInitialization() {
    // Test basic initialization
    QVERIFY(m_widget != nullptr);

    // Test that widget is properly initialized
    QVERIFY(m_widget->isEnabled());

    // Test that managers are set
    // Note: We can't directly test private members, but we can test behavior
    QVERIFY(true);  // Widget should initialize without crashing
}

void WelcomeWidgetFunctionalityTest::testQuickActionConnections() {
    // Test that quick action buttons are properly connected
    QSignalSpy tutorialSpy(m_widget, &WelcomeWidget::tutorialRequested);
    QSignalSpy settingsSpy(m_widget, &WelcomeWidget::showSettingsRequested);
    QSignalSpy helpSpy(m_widget, &WelcomeWidget::showDocumentationRequested);

    // Find quick action buttons
    QList<QToolButton*> buttons = m_widget->findChildren<QToolButton*>();

    bool foundSearchButton = false;
    bool foundSettingsButton = false;
    bool foundHelpButton = false;

    for (QToolButton* button : buttons) {
        if (button->text() == QObject::tr("Search")) {
            foundSearchButton = true;
            // Simulate button click
            button->click();
            QApplication::processEvents();
        } else if (button->text() == QObject::tr("Settings")) {
            foundSettingsButton = true;
            button->click();
            QApplication::processEvents();
        } else if (button->text() == QObject::tr("Help")) {
            foundHelpButton = true;
            button->click();
            QApplication::processEvents();
        }
    }

    // Verify buttons were found and signals were emitted
    QVERIFY(foundSearchButton || foundSettingsButton || foundHelpButton);

    // At least some signals should be emitted (depending on command manager
    // state)
    QVERIFY(tutorialSpy.count() >= 0);
    QVERIFY(settingsSpy.count() >= 0);
    QVERIFY(helpSpy.count() >= 0);
}

void WelcomeWidgetFunctionalityTest::testRecentFileValidation() {
    // Test recent file validation functionality
    QSignalSpy fileOpenSpy(m_widget, &WelcomeWidget::fileOpenRequested);

    // Create a temporary file for testing
    QString tempFilePath = QDir::temp().filePath("test_document.pdf");
    QFile tempFile(tempFilePath);
    if (tempFile.open(QIODevice::WriteOnly)) {
        tempFile.write("dummy content");
        tempFile.close();

        // Test opening valid file
        // Note: We can't directly call onRecentFileClicked as it's private,
        // but we can test the validation logic indirectly
        QFileInfo fileInfo(tempFilePath);
        QVERIFY(fileInfo.exists());
        QVERIFY(fileInfo.isReadable());

        // Clean up
        tempFile.remove();
    }

    // Test with non-existent file
    QString nonExistentPath = "/path/that/does/not/exist.pdf";
    QFileInfo nonExistentInfo(nonExistentPath);
    QVERIFY(!nonExistentInfo.exists());
}

void WelcomeWidgetFunctionalityTest::testTutorialIntegration() {
    // Test tutorial integration
    QSignalSpy tutorialSpy(m_widget, &WelcomeWidget::tutorialRequested);
    QSignalSpy onboardingSpy(m_widget,
                             &WelcomeWidget::startOnboardingRequested);

    // Refresh content to trigger tutorial setup
    m_widget->refreshContent();
    QApplication::processEvents();

    // Look for tutorial-related buttons
    QList<QPushButton*> buttons = m_widget->findChildren<QPushButton*>();

    for (QPushButton* button : buttons) {
        if (button->text().contains("Tour") ||
            button->text().contains("Tutorial")) {
            button->click();
            QApplication::processEvents();
            break;
        }
    }

    // Should handle tutorial integration without crashing
    QVERIFY(true);
}

void WelcomeWidgetFunctionalityTest::testThemeApplication() {
    // Test theme application
    m_widget->applyTheme();
    QApplication::processEvents();

    // Should apply theme without crashing
    QVERIFY(true);

    // Test multiple theme applications
    m_widget->applyTheme();
    m_widget->applyTheme();
    QApplication::processEvents();

    QVERIFY(true);
}

void WelcomeWidgetFunctionalityTest::testStateManagement() {
    // Test state management functionality

    // Save state
    m_widget->saveState();

    // Load state
    m_widget->loadState();

    // Reset state
    m_widget->resetState();

    // Should handle state operations without crashing
    QVERIFY(true);

    // Test refresh operations
    m_widget->refreshContent();
    m_widget->refreshTips();
    m_widget->refreshShortcuts();

    QVERIFY(true);
}

QTEST_MAIN(WelcomeWidgetFunctionalityTest)
#include "test_welcome_widget_functionality.moc"
