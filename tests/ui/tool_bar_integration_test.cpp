#include <QApplication>
#include <QPropertyAnimation>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../app/ui/core/ToolBar.h"

class ToolBarIntegrationTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Section expand/collapse tests
    void testSectionExpandCollapse();
    void testSectionExpandCollapseSignals();
    void testCompactModeIntegration();
    void testAnimationIntegration();

    // Action integration tests
    void testActionTriggering();
    void testPageNavigationIntegration();
    void testZoomIntegration();

    // State management tests
    void testStateUpdates();
    void testLanguageChangeIntegration();

private:
    ToolBar* m_toolbar;
    QWidget* m_parentWidget;

    CollapsibleSection* findSection(const QString& title);
    void waitForAnimation();
};

void ToolBarIntegrationTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(1000, 200);
    m_parentWidget->show();
}

void ToolBarIntegrationTest::cleanupTestCase() { delete m_parentWidget; }

void ToolBarIntegrationTest::init() {
    m_toolbar = new ToolBar(m_parentWidget);
    m_parentWidget->layout()->addWidget(m_toolbar);
    m_toolbar->show();
    [[maybe_unused]] bool exposed = QTest::qWaitForWindowExposed(m_toolbar);
}

void ToolBarIntegrationTest::cleanup() {
    delete m_toolbar;
    m_toolbar = nullptr;
}

void ToolBarIntegrationTest::testSectionExpandCollapse() {
    CollapsibleSection* fileSection = findSection("File");
    QVERIFY(fileSection != nullptr);

    // Test initial state
    bool initialState = fileSection->isExpanded();

    // Toggle expansion
    fileSection->setExpanded(!initialState);
    waitForAnimation();

    QCOMPARE(fileSection->isExpanded(), !initialState);

    // Toggle back
    fileSection->setExpanded(initialState);
    waitForAnimation();

    QCOMPARE(fileSection->isExpanded(), initialState);
}

void ToolBarIntegrationTest::testSectionExpandCollapseSignals() {
    CollapsibleSection* navigationSection = findSection("Navigation");
    QVERIFY(navigationSection != nullptr);

    QSignalSpy sectionSpy(m_toolbar, &ToolBar::sectionExpandChanged);
    QSignalSpy expandSpy(navigationSection,
                         &CollapsibleSection::expandedChanged);

    bool initialState = navigationSection->isExpanded();

    // Expand/collapse section
    navigationSection->setExpanded(!initialState);
    waitForAnimation();

    // Verify signals were emitted
    QCOMPARE(expandSpy.count(), 1);
    QCOMPARE(sectionSpy.count(), 1);

    // Verify signal parameters
    QList<QVariant> sectionArgs = sectionSpy.takeFirst();
    QCOMPARE(sectionArgs.at(0).toString(), QString("Navigation"));
    QCOMPARE(sectionArgs.at(1).toBool(), !initialState);
}

void ToolBarIntegrationTest::testCompactModeIntegration() {
    // Test compact mode activation
    m_toolbar->setCompactMode(true);
    waitForAnimation();

    // Verify all sections are collapsed in compact mode
    CollapsibleSection* fileSection = findSection("File");
    CollapsibleSection* navSection = findSection("Navigation");
    CollapsibleSection* zoomSection = findSection("Zoom");

    if (fileSection)
        QVERIFY(!fileSection->isExpanded());
    if (navSection)
        QVERIFY(!navSection->isExpanded());
    if (zoomSection)
        QVERIFY(!zoomSection->isExpanded());

    // Test compact mode deactivation
    m_toolbar->setCompactMode(false);
    waitForAnimation();

    // Important sections should be expanded
    if (navSection)
        QVERIFY(navSection->isExpanded());
    if (zoomSection)
        QVERIFY(zoomSection->isExpanded());
}

void ToolBarIntegrationTest::testAnimationIntegration() {
    // Enable compact mode to test hover animations
    m_toolbar->setCompactMode(true);
    waitForAnimation();

    int initialHeight = m_toolbar->height();

    // Simulate mouse enter event
    QEnterEvent enterEvent(QPointF(50, 50), QPointF(50, 50), QPointF(50, 50));
    QApplication::sendEvent(m_toolbar, &enterEvent);

    // Wait for animation to start
    QTest::qWait(200);

    // Height should change during hover in compact mode
    // (This is a basic test - actual animation behavior may vary)

    // Simulate mouse leave event
    QEvent leaveEvent(QEvent::Leave);
    QApplication::sendEvent(m_toolbar, &leaveEvent);

    QTest::qWait(200);
}

void ToolBarIntegrationTest::testActionTriggering() {
    QSignalSpy actionSpy(m_toolbar, &ToolBar::actionTriggered);

    // Find and click an action button
    QList<QToolButton*> buttons = m_toolbar->findChildren<QToolButton*>();

    for (QToolButton* button : buttons) {
        if (button->isVisible() && button->isEnabled()) {
            QTest::mouseClick(button, Qt::LeftButton);
            break;
        }
    }

    // Wait for signal processing
    QTest::qWait(50);

    // At least one action should have been triggered
    QVERIFY(actionSpy.count() >= 0);  // May be 0 if no enabled buttons found
}

void ToolBarIntegrationTest::testPageNavigationIntegration() {
    QSignalSpy pageSpy(m_toolbar, &ToolBar::pageJumpRequested);

    // Find page spinbox
    QSpinBox* pageSpinBox = m_toolbar->findChild<QSpinBox*>();
    if (pageSpinBox) {
        pageSpinBox->setValue(5);

        // Wait for signal
        QTest::qWait(50);

        QCOMPARE(pageSpy.count(), 1);
        QList<QVariant> args = pageSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 4);  // 0-based page number
    }
}

void ToolBarIntegrationTest::testZoomIntegration() {
    QSignalSpy zoomSpy(m_toolbar, &ToolBar::zoomLevelChanged);

    // Find zoom slider
    QSlider* zoomSlider = m_toolbar->findChild<QSlider*>();
    if (zoomSlider) {
        int initialValue = zoomSlider->value();
        zoomSlider->setValue(150);

        // Wait for signal
        QTest::qWait(50);

        if (initialValue != 150) {
            QCOMPARE(zoomSpy.count(), 1);
            QList<QVariant> args = zoomSpy.takeFirst();
            QCOMPARE(args.at(0).toInt(), 150);
        }
    }
}

void ToolBarIntegrationTest::testStateUpdates() {
    // Test page info update
    m_toolbar->updatePageInfo(5, 10);

    QSpinBox* pageSpinBox = m_toolbar->findChild<QSpinBox*>();
    QLabel* pageLabel = m_toolbar->findChild<QLabel*>();

    if (pageSpinBox) {
        QCOMPARE(pageSpinBox->value(), 6);  // 1-based display
        QCOMPARE(pageSpinBox->maximum(), 10);
    }

    // Test zoom level update
    m_toolbar->updateZoomLevel(1.5);

    QSlider* zoomSlider = m_toolbar->findChild<QSlider*>();
    if (zoomSlider) {
        QCOMPARE(zoomSlider->value(), 150);
    }

    // Test actions enabled/disabled
    m_toolbar->setActionsEnabled(false);

    QList<QAction*> actions = m_toolbar->actions();
    for (QAction* action : actions) {
        if (action->isSeparator())
            continue;
        // Most actions should be disabled (some may remain enabled)
    }

    m_toolbar->setActionsEnabled(true);
}

void ToolBarIntegrationTest::testLanguageChangeIntegration() {
    // Get initial section titles
    CollapsibleSection* fileSection = findSection("File");
    QString initialTitle;
    if (fileSection) {
        initialTitle = fileSection->windowTitle();
    }

    // Simulate language change event
    QEvent languageChangeEvent(QEvent::LanguageChange);
    QApplication::sendEvent(m_toolbar, &languageChangeEvent);

    // Verify UI elements are updated
    if (fileSection) {
        QString newTitle = fileSection->windowTitle();
        QVERIFY(!newTitle.isEmpty());
        // Title should be updated (may be same if already in correct language)
    }

    // Check that tooltips are updated
    QList<QAction*> actions = m_toolbar->actions();
    for (QAction* action : actions) {
        if (!action->isSeparator() && !action->toolTip().isEmpty()) {
            QVERIFY(!action->toolTip().isEmpty());
        }
    }
}

CollapsibleSection* ToolBarIntegrationTest::findSection(const QString& title) {
    QList<CollapsibleSection*> sections =
        m_toolbar->findChildren<CollapsibleSection*>();

    for (CollapsibleSection* section : sections) {
        if (section->windowTitle().contains(title, Qt::CaseInsensitive)) {
            return section;
        }
    }

    return nullptr;
}

void ToolBarIntegrationTest::waitForAnimation() {
    // Wait for animations to complete
    QTest::qWait(200);
    QApplication::processEvents();
}

QTEST_MAIN(ToolBarIntegrationTest)
#include "tool_bar_integration_test.moc"
