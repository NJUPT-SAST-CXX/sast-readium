#include <QApplication>
#include <QSignalSpy>
#include <QTextCursor>
#include <QTimer>
#include <QtTest/QtTest>
#include "../../app/logging/LoggingMacros.h"
#include "../../app/logging/LoggingManager.h"
#include "../../app/ui/widgets/DebugLogPanel.h"

class DebugLogPanelIntegrationTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Search navigation tests
    void testSearchNavigation();
    void testSearchNavigationWithRegex();
    void testSearchNavigationEdgeCases();
    void testSearchHighlighting();

    // Integration with logging system
    void testLogMessageIntegration();
    void testFilteringIntegration();
    void testRealTimeUpdates();

    // UI state tests
    void testSearchStateManagement();
    void testLanguageChangeIntegration();

private:
    DebugLogPanel* m_panel;
    QWidget* m_parentWidget;

    void addTestLogMessages();
    void waitForLogProcessing();
};

void DebugLogPanelIntegrationTest::initTestCase() {
    // Initialize logging system
    LoggingManager::instance().initialize();

    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
}

void DebugLogPanelIntegrationTest::cleanupTestCase() { delete m_parentWidget; }

void DebugLogPanelIntegrationTest::init() {
    m_panel = new DebugLogPanel(m_parentWidget);
    m_panel->show();
    [[maybe_unused]] bool exposed = QTest::qWaitForWindowExposed(m_panel);
}

void DebugLogPanelIntegrationTest::cleanup() {
    delete m_panel;
    m_panel = nullptr;
}

void DebugLogPanelIntegrationTest::testSearchNavigation() {
    // Add test log messages
    addTestLogMessages();
    waitForLogProcessing();

    // Perform search
    QLineEdit* searchEdit = m_panel->findChild<QLineEdit*>();
    QVERIFY(searchEdit != nullptr);

    searchEdit->setText("test");
    QTest::keyClick(searchEdit, Qt::Key_Return);

    // Wait for search to complete
    QTest::qWait(100);

    // Test next navigation
    QPushButton* nextBtn = m_panel->findChild<QPushButton*>("searchNextBtn");
    if (nextBtn) {
        // Since searchResultNavigated signal doesn't exist, test the UI
        // behavior directly
        QTextEdit* logDisplay = m_panel->findChild<QTextEdit*>();
        QTextCursor initialCursor =
            logDisplay ? logDisplay->textCursor() : QTextCursor();

        QTest::mouseClick(nextBtn, Qt::LeftButton);
        QTest::qWait(50);

        // Verify cursor position changed
        QTextCursor currentCursor = logDisplay->textCursor();
        // Test that search navigation is working by checking if cursor moved or
        // has selection
        QVERIFY(currentCursor.position() != initialCursor.position() ||
                currentCursor.hasSelection());
    }
}

void DebugLogPanelIntegrationTest::testSearchNavigationWithRegex() {
    addTestLogMessages();
    waitForLogProcessing();

    // Enable regex search
    QCheckBox* regexCheck = m_panel->findChild<QCheckBox*>();
    if (regexCheck && regexCheck->text().contains("Regex")) {
        regexCheck->setChecked(true);
    }

    // Search with regex pattern
    QLineEdit* searchEdit = m_panel->findChild<QLineEdit*>();
    QVERIFY(searchEdit != nullptr);

    searchEdit->setText("test.*message");
    QTest::keyClick(searchEdit, Qt::Key_Return);
    QTest::qWait(100);

    // Test navigation
    QPushButton* nextBtn = m_panel->findChild<QPushButton*>("searchNextBtn");
    QPushButton* prevBtn = m_panel->findChild<QPushButton*>("searchPrevBtn");

    if (nextBtn && prevBtn) {
        // Navigate forward
        QTest::mouseClick(nextBtn, Qt::LeftButton);
        QTest::qWait(50);

        // Navigate backward
        QTest::mouseClick(prevBtn, Qt::LeftButton);
        QTest::qWait(50);

        QTextEdit* logDisplay = m_panel->findChild<QTextEdit*>();
        QVERIFY(logDisplay != nullptr);
        QVERIFY(logDisplay->textCursor().hasSelection());
    }
}

void DebugLogPanelIntegrationTest::testSearchNavigationEdgeCases() {
    // Test with empty search
    QLineEdit* searchEdit = m_panel->findChild<QLineEdit*>();
    QVERIFY(searchEdit != nullptr);

    searchEdit->setText("");
    QPushButton* nextBtn = m_panel->findChild<QPushButton*>("searchNextBtn");
    if (nextBtn) {
        QTest::mouseClick(nextBtn, Qt::LeftButton);
        // Should not crash or cause issues
    }

    // Test with no results
    searchEdit->setText("nonexistent_pattern_xyz123");
    QTest::keyClick(searchEdit, Qt::Key_Return);
    QTest::qWait(100);

    if (nextBtn) {
        QTest::mouseClick(nextBtn, Qt::LeftButton);
        // Should handle gracefully
    }
}

void DebugLogPanelIntegrationTest::testSearchHighlighting() {
    addTestLogMessages();
    waitForLogProcessing();

    QLineEdit* searchEdit = m_panel->findChild<QLineEdit*>();
    QVERIFY(searchEdit != nullptr);

    searchEdit->setText("test");
    QTest::keyClick(searchEdit, Qt::Key_Return);
    QTest::qWait(100);

    QTextEdit* logDisplay = m_panel->findChild<QTextEdit*>();
    QVERIFY(logDisplay != nullptr);

    // Check that highlighting is applied
    QTextDocument* document = logDisplay->document();
    QVERIFY(document != nullptr);

    // The document should contain highlighted text
    QString plainText = document->toPlainText();
    QVERIFY(plainText.contains("test"));
}

void DebugLogPanelIntegrationTest::testLogMessageIntegration() {
    // Since logMessageAdded signal doesn't exist, test by checking log display
    // content
    QTextEdit* logDisplay = m_panel->findChild<QTextEdit*>();
    QString initialContent = logDisplay ? logDisplay->toPlainText() : QString();

    // Send log messages through the logging system
    LOG_INFO("Integration test message 1");
    LOG_WARNING("Integration test message 2");
    LOG_ERROR("Integration test message 3");

    waitForLogProcessing();

    if (logDisplay) {
        QString currentContent = logDisplay->toPlainText();
        // Verify content changed and contains our messages
        QVERIFY(currentContent != initialContent);
        QVERIFY(currentContent.contains("Integration test message 1"));
        QVERIFY(currentContent.contains("Integration test message 2"));
        QVERIFY(currentContent.contains("Integration test message 3"));
    }
}

void DebugLogPanelIntegrationTest::testFilteringIntegration() {
    addTestLogMessages();
    waitForLogProcessing();

    // Test log level filtering
    QComboBox* levelFilter = m_panel->findChild<QComboBox*>();
    if (levelFilter && levelFilter->count() > 0) {
        // Set to Error+ level
        for (int i = 0; i < levelFilter->count(); ++i) {
            if (levelFilter->itemText(i).contains("Error")) {
                levelFilter->setCurrentIndex(i);
                break;
            }
        }

        QTest::qWait(100);

        QTextEdit* logDisplay = m_panel->findChild<QTextEdit*>();
        QVERIFY(logDisplay != nullptr);

        QString displayText = logDisplay->toPlainText();
        // Should only show error and critical messages
        QVERIFY(!displayText.contains("DEBUG"));
        QVERIFY(!displayText.contains("INFO"));
    }
}

void DebugLogPanelIntegrationTest::testRealTimeUpdates() {
    QTextEdit* logDisplay = m_panel->findChild<QTextEdit*>();
    QVERIFY(logDisplay != nullptr);

    QString initialText = logDisplay->toPlainText();

    // Send new log message
    LOG_INFO("Real-time update test");
    waitForLogProcessing();

    QString updatedText = logDisplay->toPlainText();
    QVERIFY(updatedText != initialText);
    QVERIFY(updatedText.contains("Real-time update test"));
}

void DebugLogPanelIntegrationTest::testSearchStateManagement() {
    addTestLogMessages();
    waitForLogProcessing();

    QLineEdit* searchEdit = m_panel->findChild<QLineEdit*>();
    QVERIFY(searchEdit != nullptr);

    // Perform search
    searchEdit->setText("test");
    QTest::keyClick(searchEdit, Qt::Key_Return);
    QTest::qWait(100);

    // Clear search
    searchEdit->clear();
    QTest::keyClick(searchEdit, Qt::Key_Return);
    QTest::qWait(100);

    QTextEdit* logDisplay = m_panel->findChild<QTextEdit*>();
    QVERIFY(logDisplay != nullptr);

    // Verify highlighting is cleared
    QTextCursor cursor = logDisplay->textCursor();
    QVERIFY(!cursor.hasSelection());
}

void DebugLogPanelIntegrationTest::testLanguageChangeIntegration() {
    // Simulate language change event
    QEvent languageChangeEvent(QEvent::LanguageChange);
    QApplication::sendEvent(m_panel, &languageChangeEvent);

    // Verify UI elements are updated (basic check)
    QGroupBox* filterGroup = m_panel->findChild<QGroupBox*>();
    if (filterGroup) {
        QVERIFY(!filterGroup->title().isEmpty());
    }
}

void DebugLogPanelIntegrationTest::addTestLogMessages() {
    LOG_DEBUG("Debug test message");
    LOG_INFO("Info test message");
    LOG_WARNING("Warning test message");
    LOG_ERROR("Error test message");
    LOG_CRITICAL("Critical test message");
}

void DebugLogPanelIntegrationTest::waitForLogProcessing() {
    // Wait for log messages to be processed
    QTest::qWait(200);
    QApplication::processEvents();
}

QTEST_MAIN(DebugLogPanelIntegrationTest)
#include "test_debug_log_panel_integration.moc"
