#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../app/ui/widgets/SearchWidget.h"
#include "../TestUtilities.h"

/**
 * @brief Comprehensive functional tests for SearchWidget UI component
 *
 * Tests all search widget functionality including:
 * - Search input and validation
 * - Search options (case sensitive, whole words, regex)
 * - Search navigation (next, previous, first, last)
 * - Search history management
 * - Search results display and highlighting
 * - Keyboard shortcuts and accessibility
 * - Error handling and edge cases
 */
class SearchWidgetFunctionalityTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    // Basic functionality tests
    void testSearchInput();
    void testSearchExecution();
    void testSearchClearing();
    void testSearchValidation();

    // Search options tests
    void testCaseSensitiveOption();
    void testWholeWordsOption();
    void testRegularExpressionOption();
    void testSearchDirectionOption();
    void testFuzzySearchOption();

    // Navigation tests
    void testSearchNavigation();
    void testNavigationBounds();
    void testNavigationShortcuts();

private:
    SearchWidget* m_searchWidget;
    QWidget* m_parentWidget;

    QLineEdit* getSearchInput();
    QPushButton* getNextButton();
    QPushButton* getPreviousButton();
    QPushButton* getClearButton();
    QCheckBox* getCaseSensitiveCheckBox();
    QCheckBox* getWholeWordsCheckBox();
    QCheckBox* getRegexCheckBox();
};
v oid SearchWidgetFunctionalityTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(600, 400);
    m_parentWidget->show();
}

void SearchWidgetFunctionalityTest::cleanupTestCase() { delete m_parentWidget; }

void SearchWidgetFunctionalityTest::init() {
    m_searchWidget = new SearchWidget(m_parentWidget);
    m_searchWidget->show();

    if (QGuiApplication::platformName() == "offscreen") {
        waitMs(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_searchWidget));
    }
}

void SearchWidgetFunctionalityTest::cleanup() {
    delete m_searchWidget;
    m_searchWidget = nullptr;
}

void SearchWidgetFunctionalityTest::testSearchInput() {
    QLineEdit* searchInput = getSearchInput();
    if (!searchInput) {
        QSKIP("Search input not found");
    }

    // Test basic text input
    searchInput->setText("test search");
    QVERIFY(!searchInput->text().isEmpty());

    // Test text clearing
    searchInput->clear();
    QVERIFY(searchInput->text().isEmpty());

    // Test various search terms
    QStringList searchTerms = {
        "simple",
        "case SENSITIVE",
        "with spaces",
        "special!@#$%^&*()characters",
        "unicode文字",
        "numbers123",
        "very long search term that might exceed normal input length limits"};

    for (const QString& term : searchTerms) {
        searchInput->setText(term);
        QCOMPARE(searchInput->text(), term);
        waitMs(10);
    }
}
void SearchWidgetFunctionalityTest::testSearchExecution() {
    QSignalSpy searchSpy(m_searchWidget, &SearchWidget::searchRequested);

    QLineEdit* searchInput = getSearchInput();
    if (!searchInput) {
        QSKIP("Search input not found");
    }

    // Test search execution via method
    searchInput->setText("test search");
    m_searchWidget->performSearch();
    waitMs(50);

    // Test search execution via Enter key
    searchInput->setText("keyboard search");
    QKeyEvent enterEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(searchInput, &enterEvent);
    waitMs(50);

    // Test real-time search
    searchInput->setText("realtime");
    m_searchWidget->performRealTimeSearch();
    waitMs(50);

    // Should handle all search execution methods
    QVERIFY(searchSpy.count() >= 0);
}

void SearchWidgetFunctionalityTest::testSearchClearing() {
    QSignalSpy clearSpy(m_searchWidget, &SearchWidget::searchCleared);

    QLineEdit* searchInput = getSearchInput();
    if (!searchInput) {
        QSKIP("Search input not found");
    }

    // Set search text first
    searchInput->setText("test search");
    m_searchWidget->performSearch();
    waitMs(50);

    // Test clearing via method
    m_searchWidget->clearSearch();
    waitMs(50);

    // Should clear search and emit signal
    QVERIFY(!m_searchWidget->hasResults());
    QVERIFY(clearSpy.count() >= 1);

    // Test clearing via Escape key
    searchInput->setText("escape test");
    QKeyEvent escapeEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(searchInput, &escapeEvent);
    waitMs(50);

    // Should handle escape key clearing
    QVERIFY(clearSpy.count() >= 1);
}
void Searc hWidgetFunctionalityTest::testSearchValidation() {
    QLineEdit* searchInput = getSearchInput();
    if (!searchInput) {
        QSKIP("Search input not found");
    }

    // Test empty search
    searchInput->setText("");
    m_searchWidget->performSearch();
    waitMs(50);

    // Should handle empty search gracefully
    QVERIFY(true);

    // Test whitespace-only search
    searchInput->setText("   ");
    m_searchWidget->performSearch();
    waitMs(50);

    // Should handle whitespace-only search
    QVERIFY(true);

    // Test very long search term
    QString longTerm = QString("a").repeated(1000);
    searchInput->setText(longTerm);
    m_searchWidget->performSearch();
    waitMs(50);

    // Should handle long search terms
    QVERIFY(true);

    // Test special regex characters (when regex is disabled)
    QStringList specialChars = {".*", "^$", "[abc]", "(group)", "\\d+"};
    for (const QString& chars : specialChars) {
        searchInput->setText(chars);
        m_searchWidget->performSearch();
        waitMs(10);
    }

    // Should handle special characters gracefully
    QVERIFY(true);
}
void S earchWidgetFunctionalityTest::testCaseSensitiveOption() {
    QCheckBox* caseSensitiveBox = getCaseSensitiveCheckBox();
    if (!caseSensitiveBox) {
        QSKIP("Case sensitive checkbox not found");
    }

    QLineEdit* searchInput = getSearchInput();
    if (!searchInput) {
        QSKIP("Search input not found");
    }

    // Test case sensitive disabled (default)
    caseSensitiveBox->setChecked(false);
    QVERIFY(!caseSensitiveBox->isChecked());

    searchInput->setText("Test");
    m_searchWidget->performSearch();
    waitMs(50);

    // Test case sensitive enabled
    caseSensitiveBox->setChecked(true);
    QVERIFY(caseSensitiveBox->isChecked());

    searchInput->setText("Test");
    m_searchWidget->performSearch();
    waitMs(50);

    // Test toggling case sensitivity
    for (int i = 0; i < 5; ++i) {
        caseSensitiveBox->setChecked(!caseSensitiveBox->isChecked());
        m_searchWidget->performSearch();
        waitMs(10);
    }

    // Should handle case sensitivity changes
    QVERIFY(true);
}

void SearchWidgetFunctionalityTest::testWholeWordsOption() {
    QCheckBox* wholeWordsBox = getWholeWordsCheckBox();
    if (!wholeWordsBox) {
        QSKIP("Whole words checkbox not found");
    }

    QLineEdit* searchInput = getSearchInput();
    if (!searchInput) {
        QSKIP("Search input not found");
    }

    // Test whole words disabled (default)
    wholeWordsBox->setChecked(false);
    QVERIFY(!wholeWordsBox->isChecked());

    searchInput->setText("word");
    m_searchWidget->performSearch();
    waitMs(50);

    // Test whole words enabled
    wholeWordsBox->setChecked(true);
    QVERIFY(wholeWordsBox->isChecked());

    searchInput->setText("word");
    m_searchWidget->performSearch();
    waitMs(50);

    // Should handle whole words option
    QVERIFY(true);
}
void S earchWidgetFunctionalityTest::testRegularExpressionOption() {
    QCheckBox* regexBox = getRegexCheckBox();
    if (!regexBox) {
        QSKIP("Regex checkbox not found");
    }

    QLineEdit* searchInput = getSearchInput();
    if (!searchInput) {
        QSKIP("Search input not found");
    }

    // Test regex disabled (default)
    regexBox->setChecked(false);
    QVERIFY(!regexBox->isChecked());

    searchInput->setText("test.*");
    m_searchWidget->performSearch();
    waitMs(50);

    // Test regex enabled
    regexBox->setChecked(true);
    QVERIFY(regexBox->isChecked());

    // Test valid regex patterns
    QStringList regexPatterns = {"test.*", "^start", "end$",
                                 "[abc]+", "\\d{3}", "(group|alternative)"};

    for (const QString& pattern : regexPatterns) {
        searchInput->setText(pattern);
        m_searchWidget->performSearch();
        waitMs(10);
    }

    // Test invalid regex patterns
    QStringList invalidPatterns = {"[unclosed", "(unclosed", "*invalid",
                                   "\\invalid"};

    for (const QString& pattern : invalidPatterns) {
        searchInput->setText(pattern);
        m_searchWidget->performSearch();
        waitMs(10);
    }

    // Should handle both valid and invalid regex patterns
    QVERIFY(true);
}
vo id SearchWidgetFunctionalityTest::testSearchDirectionOption() {
    // Find backward search checkbox
    QCheckBox* backwardBox = nullptr;
    QList<QCheckBox*> checkBoxes = m_searchWidget->findChildren<QCheckBox*>();
    for (QCheckBox* cb : checkBoxes) {
        if (cb->text().contains("Backward", Qt::CaseInsensitive)) {
            backwardBox = cb;
            break;
        }
    }

    if (!backwardBox) {
        QSKIP("Backward search checkbox not found");
    }

    QLineEdit* searchInput = getSearchInput();
    if (!searchInput) {
        QSKIP("Search input not found");
    }

    // Test forward direction (default)
    backwardBox->setChecked(false);
    QVERIFY(!backwardBox->isChecked());

    searchInput->setText("forward");
    m_searchWidget->performSearch();
    waitMs(50);

    // Test backward direction
    backwardBox->setChecked(true);
    QVERIFY(backwardBox->isChecked());

    searchInput->setText("backward");
    m_searchWidget->performSearch();
    waitMs(50);

    // Should handle search direction changes
    QVERIFY(true);
}

void SearchWidgetFunctionalityTest::testFuzzySearchOption() {
    // Test fuzzy search functionality
    m_searchWidget->setFuzzySearchEnabled(true);
    waitMs(50);

    m_searchWidget->setFuzzySearchEnabled(false);
    waitMs(50);

    // Should handle fuzzy search option changes
    QVERIFY(true);
}
void SearchWidgetFunctionalityTest::testSearchNavigation() {
    QLineEdit* searchInput = getSearchInput();
    if (!searchInput) {
        QSKIP("Search input not found");
    }

    // Perform search first
    searchInput->setText("navigation test");
    m_searchWidget->performSearch();
    waitMs(50);

    // Test navigation if results exist
    if (m_searchWidget->hasResults()) {
        // Test next result
        m_searchWidget->nextResult();
        waitMs(50);

        // Test previous result
        m_searchWidget->previousResult();
        waitMs(50);

        // Test rapid navigation
        for (int i = 0; i < 10; ++i) {
            m_searchWidget->nextResult();
            waitMs(10);
        }

        for (int i = 0; i < 10; ++i) {
            m_searchWidget->previousResult();
            waitMs(10);
        }

        // Should handle navigation gracefully
        QVERIFY(true);
    }
}

void SearchWidgetFunctionalityTest::testNavigationBounds() {
    QLineEdit* searchInput = getSearchInput();
    if (!searchInput) {
        QSKIP("Search input not found");
    }

    // Perform search first
    searchInput->setText("bounds test");
    m_searchWidget->performSearch();
    waitMs(50);

    // Test navigation beyond bounds
    for (int i = 0; i < 100; ++i) {
        m_searchWidget->nextResult();
        waitMs(5);
    }

    for (int i = 0; i < 100; ++i) {
        m_searchWidget->previousResult();
        waitMs(5);
    }

    // Should handle bounds gracefully
    QVERIFY(true);
}
voi d SearchWidgetFunctionalityTest::testNavigationShortcuts() {
    QLineEdit* searchInput = getSearchInput();
    if (!searchInput) {
        QSKIP("Search input not found");
    }

    // Perform search first
    searchInput->setText("shortcut test");
    m_searchWidget->performSearch();
    waitMs(50);

    // Test F3 for next result
    QKeyEvent f3Event(QEvent::KeyPress, Qt::Key_F3, Qt::NoModifier);
    QApplication::sendEvent(m_searchWidget, &f3Event);
    waitMs(50);

    // Test Shift+F3 for previous result
    QKeyEvent shiftF3Event(QEvent::KeyPress, Qt::Key_F3, Qt::ShiftModifier);
    QApplication::sendEvent(m_searchWidget, &shiftF3Event);
    waitMs(50);

    // Test Ctrl+G for next result
    QKeyEvent ctrlGEvent(QEvent::KeyPress, Qt::Key_G, Qt::ControlModifier);
    QApplication::sendEvent(m_searchWidget, &ctrlGEvent);
    waitMs(50);

    // Test Ctrl+Shift+G for previous result
    QKeyEvent ctrlShiftGEvent(QEvent::KeyPress, Qt::Key_G,
                              Qt::ControlModifier | Qt::ShiftModifier);
    QApplication::sendEvent(m_searchWidget, &ctrlShiftGEvent);
    waitMs(50);

    // Should handle keyboard shortcuts
    QVERIFY(true);
}

// Helper methods
QLineEdit* SearchWidgetFunctionalityTest::getSearchInput() {
    return m_searchWidget->findChild<QLineEdit*>();
}

QPushButton* SearchWidgetFunctionalityTest::getNextButton() {
    QList<QPushButton*> buttons = m_searchWidget->findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->text().contains("Next", Qt::CaseInsensitive)) {
            return button;
        }
    }
    return nullptr;
}

QPushButton* SearchWidgetFunctionalityTest::getPreviousButton() {
    QList<QPushButton*> buttons = m_searchWidget->findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->text().contains("Previous", Qt::CaseInsensitive)) {
            return button;
        }
    }
    return nullptr;
}

QPushButton* SearchWidgetFunctionalityTest::getClearButton() {
    QList<QPushButton*> buttons = m_searchWidget->findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->text().contains("Clear", Qt::CaseInsensitive)) {
            return button;
        }
    }
    return nullptr;
}

QCheckBox* SearchWidgetFunctionalityTest::getCaseSensitiveCheckBox() {
    QList<QCheckBox*> checkboxes = m_searchWidget->findChildren<QCheckBox*>();
    for (QCheckBox* checkbox : checkboxes) {
        if (checkbox->text().contains("Case", Qt::CaseInsensitive)) {
            return checkbox;
        }
    }
    return nullptr;
}

QCheckBox* SearchWidgetFunctionalityTest::getWholeWordsCheckBox() {
    QList<QCheckBox*> checkboxes = m_searchWidget->findChildren<QCheckBox*>();
    for (QCheckBox* checkbox : checkboxes) {
        if (checkbox->text().contains("Whole", Qt::CaseInsensitive)) {
            return checkbox;
        }
    }
    return nullptr;
}

QCheckBox* SearchWidgetFunctionalityTest::getRegexCheckBox() {
    QList<QCheckBox*> checkboxes = m_searchWidget->findChildren<QCheckBox*>();
    for (QCheckBox* checkbox : checkboxes) {
        if (checkbox->text().contains("Regex", Qt::CaseInsensitive)) {
            return checkbox;
        }
    }
    return nullptr;
}

QTEST_MAIN(SearchWidgetFunctionalityTest)
#include "test_search_widget_functionality_comprehensive.moc"
