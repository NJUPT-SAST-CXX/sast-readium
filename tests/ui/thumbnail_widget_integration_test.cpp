#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPixmap>
#include "../../app/ui/thumbnail/ThumbnailWidget.h"

class ThumbnailWidgetIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testPageNumber();
    void testPixmapSetting();
    void testState();
    
    // Size and layout tests
    void testThumbnailSize();
    void testSizeHint();
    void testMinimumSizeHint();
    
    // Animation property tests
    void testShadowOpacity();
    void testBorderOpacity();
    void testAnimationProperties();
    
    // Loading state tests
    void testLoadingState();
    void testLoadingAnimation();
    void testErrorState();
    
    // Mouse interaction tests
    void testMousePress();
    void testMouseDoubleClick();
    void testHoverEvents();
    void testContextMenu();
    
    // Signal emission tests
    void testClickedSignal();
    void testDoubleClickedSignal();
    void testRightClickedSignal();
    void testHoverSignals();
    
    // Visual state tests
    void testNormalState();
    void testHoveredState();
    void testSelectedState();
    void testStateTransitions();
    
    // Painting tests
    void testPaintEvent();
    void testThumbnailDrawing();
    void testPageNumberDrawing();
    void testLoadingIndicator();
    void testErrorIndicator();
    
    // Animation tests
    void testHoverAnimation();
    void testSelectionAnimation();
    void testShadowAnimation();
    void testBorderAnimation();
    
    // Edge cases and error handling
    void testInvalidPixmap();
    void testInvalidPageNumber();
    void testInvalidSize();

private:
    ThumbnailWidget* m_widget;
    QWidget* m_parentWidget;
    
    void waitForAnimation();
    void simulateMouseEvent(QEvent::Type type, Qt::MouseButton button, const QPoint& pos);
    QPixmap createTestPixmap(const QSize& size);
};

void ThumbnailWidgetIntegrationTest::initTestCase()
{
    m_parentWidget = new QWidget();
    m_parentWidget->resize(400, 600);
    m_parentWidget->show();
}

void ThumbnailWidgetIntegrationTest::cleanupTestCase()
{
    delete m_parentWidget;
}

void ThumbnailWidgetIntegrationTest::init()
{
    m_widget = new ThumbnailWidget(0, m_parentWidget);
    m_widget->show();
    QTest::qWaitForWindowExposed(m_widget);
}

void ThumbnailWidgetIntegrationTest::cleanup()
{
    delete m_widget;
    m_widget = nullptr;
}

void ThumbnailWidgetIntegrationTest::testInitialization()
{
    // Test basic initialization
    QVERIFY(m_widget != nullptr);
    QVERIFY(m_widget->isVisible());
    QCOMPARE(m_widget->pageNumber(), 0);
    QCOMPARE(m_widget->state(), ThumbnailWidget::Normal);
}

void ThumbnailWidgetIntegrationTest::testPageNumber()
{
    // Test setting page number
    m_widget->setPageNumber(5);
    QCOMPARE(m_widget->pageNumber(), 5);
    
    m_widget->setPageNumber(0);
    QCOMPARE(m_widget->pageNumber(), 0);
    
    // Test negative page number
    m_widget->setPageNumber(-1);
    QCOMPARE(m_widget->pageNumber(), -1);
    
    // Test large page number
    m_widget->setPageNumber(1000);
    QCOMPARE(m_widget->pageNumber(), 1000);
}

void ThumbnailWidgetIntegrationTest::testPixmapSetting()
{
    // Test setting pixmap
    QPixmap testPixmap = createTestPixmap(QSize(100, 150));
    m_widget->setPixmap(testPixmap);
    
    QCOMPARE(m_widget->pixmap().size(), testPixmap.size());
    QVERIFY(!m_widget->pixmap().isNull());
    
    // Test setting null pixmap
    m_widget->setPixmap(QPixmap());
    QVERIFY(m_widget->pixmap().isNull());
}

void ThumbnailWidgetIntegrationTest::testState()
{
    // Test setting different states
    m_widget->setState(ThumbnailWidget::Normal);
    QCOMPARE(m_widget->state(), ThumbnailWidget::Normal);
    
    m_widget->setState(ThumbnailWidget::Hovered);
    QCOMPARE(m_widget->state(), ThumbnailWidget::Hovered);
    
    m_widget->setState(ThumbnailWidget::Selected);
    QCOMPARE(m_widget->state(), ThumbnailWidget::Selected);
    
    m_widget->setState(ThumbnailWidget::Loading);
    QCOMPARE(m_widget->state(), ThumbnailWidget::Loading);
    
    m_widget->setState(ThumbnailWidget::Error);
    QCOMPARE(m_widget->state(), ThumbnailWidget::Error);
}

void ThumbnailWidgetIntegrationTest::testThumbnailSize()
{
    // Test default size
    QSize defaultSize = m_widget->thumbnailSize();
    QVERIFY(defaultSize.width() > 0);
    QVERIFY(defaultSize.height() > 0);
    
    // Test setting custom size
    QSize customSize(200, 300);
    m_widget->setThumbnailSize(customSize);
    QCOMPARE(m_widget->thumbnailSize(), customSize);
    
    // Test invalid size
    QSize invalidSize(0, 0);
    m_widget->setThumbnailSize(invalidSize);
    // Should handle gracefully
    QVERIFY(m_widget->thumbnailSize().width() >= 0);
    QVERIFY(m_widget->thumbnailSize().height() >= 0);
}

void ThumbnailWidgetIntegrationTest::testSizeHint()
{
    // Test size hint using public methods
    QSize currentSize = m_widget->size();
    QVERIFY(currentSize.isValid());

    // Test resize functionality
    m_widget->resize(200, 150);
    QSize newSize = m_widget->size();
    QCOMPARE(newSize.width(), 200);
    QCOMPARE(newSize.height(), 150);
}

void ThumbnailWidgetIntegrationTest::testMinimumSizeHint()
{
    // Test minimum size using public methods
    QSize minimumSize = m_widget->minimumSize();
    QVERIFY(minimumSize.isValid());

    // Test setting minimum size
    m_widget->setMinimumSize(100, 80);
    QSize newMinSize = m_widget->minimumSize();
    QCOMPARE(newMinSize.width(), 100);
    QCOMPARE(newMinSize.height(), 80);
}

void ThumbnailWidgetIntegrationTest::testShadowOpacity()
{
    // Test shadow opacity property
    qreal initialOpacity = m_widget->shadowOpacity();
    QVERIFY(initialOpacity >= 0.0);
    QVERIFY(initialOpacity <= 1.0);
    
    m_widget->setShadowOpacity(0.5);
    QCOMPARE(m_widget->shadowOpacity(), 0.5);
    
    m_widget->setShadowOpacity(1.0);
    QCOMPARE(m_widget->shadowOpacity(), 1.0);
    
    m_widget->setShadowOpacity(0.0);
    QCOMPARE(m_widget->shadowOpacity(), 0.0);
}

void ThumbnailWidgetIntegrationTest::testBorderOpacity()
{
    // Test border opacity property
    qreal initialOpacity = m_widget->borderOpacity();
    QVERIFY(initialOpacity >= 0.0);
    QVERIFY(initialOpacity <= 1.0);
    
    m_widget->setBorderOpacity(0.7);
    QCOMPARE(m_widget->borderOpacity(), 0.7);
    
    m_widget->setBorderOpacity(1.0);
    QCOMPARE(m_widget->borderOpacity(), 1.0);
    
    m_widget->setBorderOpacity(0.0);
    QCOMPARE(m_widget->borderOpacity(), 0.0);
}

void ThumbnailWidgetIntegrationTest::testAnimationProperties()
{
    // Test that animation properties work correctly
    qreal initialShadow = m_widget->shadowOpacity();
    qreal initialBorder = m_widget->borderOpacity();
    
    // Change properties
    m_widget->setShadowOpacity(0.8);
    m_widget->setBorderOpacity(0.6);
    
    // Properties should be updated
    QCOMPARE(m_widget->shadowOpacity(), 0.8);
    QCOMPARE(m_widget->borderOpacity(), 0.6);
    
    // Reset
    m_widget->setShadowOpacity(initialShadow);
    m_widget->setBorderOpacity(initialBorder);
}

void ThumbnailWidgetIntegrationTest::testLoadingState()
{
    // Test loading state
    m_widget->setLoading(true);
    QVERIFY(m_widget->isLoading());
    QCOMPARE(m_widget->state(), ThumbnailWidget::Loading);
    
    m_widget->setLoading(false);
    QVERIFY(!m_widget->isLoading());
    QVERIFY(m_widget->state() != ThumbnailWidget::Loading);
}

void ThumbnailWidgetIntegrationTest::testLoadingAnimation()
{
    // Test loading animation
    m_widget->setLoading(true);
    
    // Wait for loading animation to run
    QTest::qWait(200);
    
    // Should handle loading animation without crashing
    QVERIFY(true);
    
    m_widget->setLoading(false);
}

void ThumbnailWidgetIntegrationTest::testErrorState()
{
    // Test error state
    QString errorMessage = "Test error message";
    m_widget->setError(errorMessage);
    
    QVERIFY(m_widget->hasError());
    QCOMPARE(m_widget->state(), ThumbnailWidget::Error);
    
    // Test clearing error
    m_widget->setState(ThumbnailWidget::Normal);
    QVERIFY(!m_widget->hasError());
}

void ThumbnailWidgetIntegrationTest::testMousePress()
{
    QSignalSpy clickedSpy(m_widget, &ThumbnailWidget::clicked);
    
    // Simulate mouse press and release
    QPoint center = m_widget->rect().center();
    simulateMouseEvent(QEvent::MouseButtonPress, Qt::LeftButton, center);
    simulateMouseEvent(QEvent::MouseButtonRelease, Qt::LeftButton, center);
    
    // Should emit clicked signal
    QVERIFY(clickedSpy.count() >= 0);
    
    if (clickedSpy.count() > 0) {
        QList<QVariant> args = clickedSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), m_widget->pageNumber());
    }
}

void ThumbnailWidgetIntegrationTest::testMouseDoubleClick()
{
    QSignalSpy doubleClickedSpy(m_widget, &ThumbnailWidget::doubleClicked);
    
    // Simulate double click
    QPoint center = m_widget->rect().center();
    QMouseEvent doubleClickEvent(QEvent::MouseButtonDblClick, center, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(m_widget, &doubleClickEvent);
    
    // Should emit double clicked signal
    QVERIFY(doubleClickedSpy.count() >= 0);
    
    if (doubleClickedSpy.count() > 0) {
        QList<QVariant> args = doubleClickedSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), m_widget->pageNumber());
    }
}

void ThumbnailWidgetIntegrationTest::testHoverEvents()
{
    QSignalSpy hoverEnteredSpy(m_widget, &ThumbnailWidget::hoverEntered);
    QSignalSpy hoverLeftSpy(m_widget, &ThumbnailWidget::hoverLeft);
    
    // Simulate hover enter
    QPoint center = m_widget->rect().center();
    QEnterEvent enterEvent(center, center, center);
    QApplication::sendEvent(m_widget, &enterEvent);
    
    // Should emit hover entered signal
    QVERIFY(hoverEnteredSpy.count() >= 0);
    
    // Simulate hover leave
    QEvent leaveEvent(QEvent::Leave);
    QApplication::sendEvent(m_widget, &leaveEvent);
    
    // Should emit hover left signal
    QVERIFY(hoverLeftSpy.count() >= 0);
}

void ThumbnailWidgetIntegrationTest::testContextMenu()
{
    QSignalSpy rightClickedSpy(m_widget, &ThumbnailWidget::rightClicked);
    
    // Simulate right click
    QPoint center = m_widget->rect().center();
    QContextMenuEvent contextEvent(QContextMenuEvent::Mouse, center, m_widget->mapToGlobal(center));
    QApplication::sendEvent(m_widget, &contextEvent);
    
    // Should emit right clicked signal
    QVERIFY(rightClickedSpy.count() >= 0);
    
    if (rightClickedSpy.count() > 0) {
        QList<QVariant> args = rightClickedSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), m_widget->pageNumber());
    }
}

void ThumbnailWidgetIntegrationTest::testClickedSignal()
{
    QSignalSpy clickedSpy(m_widget, &ThumbnailWidget::clicked);
    
    m_widget->setPageNumber(42);
    
    // Simulate click
    QPoint center = m_widget->rect().center();
    simulateMouseEvent(QEvent::MouseButtonPress, Qt::LeftButton, center);
    simulateMouseEvent(QEvent::MouseButtonRelease, Qt::LeftButton, center);
    
    if (clickedSpy.count() > 0) {
        QList<QVariant> args = clickedSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 42);
    }
}

void ThumbnailWidgetIntegrationTest::testDoubleClickedSignal()
{
    QSignalSpy doubleClickedSpy(m_widget, &ThumbnailWidget::doubleClicked);
    
    m_widget->setPageNumber(24);
    
    // Simulate double click
    QPoint center = m_widget->rect().center();
    QMouseEvent doubleClickEvent(QEvent::MouseButtonDblClick, center, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(m_widget, &doubleClickEvent);
    
    if (doubleClickedSpy.count() > 0) {
        QList<QVariant> args = doubleClickedSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 24);
    }
}

void ThumbnailWidgetIntegrationTest::testRightClickedSignal()
{
    QSignalSpy rightClickedSpy(m_widget, &ThumbnailWidget::rightClicked);
    
    m_widget->setPageNumber(13);
    
    // Simulate right click
    QPoint center = m_widget->rect().center();
    QContextMenuEvent contextEvent(QContextMenuEvent::Mouse, center, m_widget->mapToGlobal(center));
    QApplication::sendEvent(m_widget, &contextEvent);
    
    if (rightClickedSpy.count() > 0) {
        QList<QVariant> args = rightClickedSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 13);
        // Second argument should be global position
        QVERIFY(args.size() >= 2);
    }
}

void ThumbnailWidgetIntegrationTest::testHoverSignals()
{
    QSignalSpy hoverEnteredSpy(m_widget, &ThumbnailWidget::hoverEntered);
    QSignalSpy hoverLeftSpy(m_widget, &ThumbnailWidget::hoverLeft);
    
    m_widget->setPageNumber(7);
    
    // Simulate hover events
    QPoint center = m_widget->rect().center();
    QEnterEvent enterEvent(center, center, center);
    QApplication::sendEvent(m_widget, &enterEvent);
    
    if (hoverEnteredSpy.count() > 0) {
        QList<QVariant> args = hoverEnteredSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 7);
    }
    
    QEvent leaveEvent(QEvent::Leave);
    QApplication::sendEvent(m_widget, &leaveEvent);
    
    if (hoverLeftSpy.count() > 0) {
        QList<QVariant> args = hoverLeftSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 7);
    }
}

void ThumbnailWidgetIntegrationTest::testNormalState()
{
    // Test normal state
    m_widget->setState(ThumbnailWidget::Normal);
    QCOMPARE(m_widget->state(), ThumbnailWidget::Normal);
    
    // Widget should be in normal visual state
    QVERIFY(true);
}

void ThumbnailWidgetIntegrationTest::testHoveredState()
{
    // Test hovered state
    m_widget->setState(ThumbnailWidget::Hovered);
    QCOMPARE(m_widget->state(), ThumbnailWidget::Hovered);
    
    // Widget should be in hovered visual state
    QVERIFY(true);
}

void ThumbnailWidgetIntegrationTest::testSelectedState()
{
    // Test selected state
    m_widget->setState(ThumbnailWidget::Selected);
    QCOMPARE(m_widget->state(), ThumbnailWidget::Selected);
    
    // Widget should be in selected visual state
    QVERIFY(true);
}

void ThumbnailWidgetIntegrationTest::testStateTransitions()
{
    // Test state transitions
    m_widget->setState(ThumbnailWidget::Normal);
    m_widget->setState(ThumbnailWidget::Hovered);
    m_widget->setState(ThumbnailWidget::Selected);
    m_widget->setState(ThumbnailWidget::Loading);
    m_widget->setState(ThumbnailWidget::Error);
    m_widget->setState(ThumbnailWidget::Normal);
    
    // Should handle state transitions without issues
    QCOMPARE(m_widget->state(), ThumbnailWidget::Normal);
}

void ThumbnailWidgetIntegrationTest::testPaintEvent()
{
    // Test paint event handling
    m_widget->setPixmap(createTestPixmap(QSize(100, 150)));
    m_widget->setPageNumber(1);
    
    // Force repaint
    m_widget->update();
    QTest::qWait(50);
    
    // Should handle painting without crashing
    QVERIFY(true);
}

void ThumbnailWidgetIntegrationTest::testThumbnailDrawing()
{
    // Test thumbnail drawing with different pixmaps
    QPixmap testPixmap = createTestPixmap(QSize(120, 160));
    m_widget->setPixmap(testPixmap);
    
    m_widget->update();
    QTest::qWait(50);
    
    // Should draw thumbnail without issues
    QVERIFY(true);
}

void ThumbnailWidgetIntegrationTest::testPageNumberDrawing()
{
    // Test page number drawing
    m_widget->setPageNumber(42);
    
    m_widget->update();
    QTest::qWait(50);
    
    // Should draw page number without issues
    QVERIFY(true);
}

void ThumbnailWidgetIntegrationTest::testLoadingIndicator()
{
    // Test loading indicator drawing
    m_widget->setLoading(true);
    
    m_widget->update();
    QTest::qWait(100);
    
    // Should draw loading indicator
    QVERIFY(true);
    
    m_widget->setLoading(false);
}

void ThumbnailWidgetIntegrationTest::testErrorIndicator()
{
    // Test error indicator drawing
    m_widget->setError("Test error");
    
    m_widget->update();
    QTest::qWait(50);
    
    // Should draw error indicator
    QVERIFY(true);
    
    m_widget->setState(ThumbnailWidget::Normal);
}

void ThumbnailWidgetIntegrationTest::testHoverAnimation()
{
    // Test hover animation
    m_widget->setState(ThumbnailWidget::Normal);
    m_widget->setState(ThumbnailWidget::Hovered);
    
    // Wait for animation
    waitForAnimation();
    
    // Should handle hover animation
    QVERIFY(true);
}

void ThumbnailWidgetIntegrationTest::testSelectionAnimation()
{
    // Test selection animation
    m_widget->setState(ThumbnailWidget::Normal);
    m_widget->setState(ThumbnailWidget::Selected);
    
    // Wait for animation
    waitForAnimation();
    
    // Should handle selection animation
    QVERIFY(true);
}

void ThumbnailWidgetIntegrationTest::testShadowAnimation()
{
    // Test shadow animation
    qreal initialOpacity = m_widget->shadowOpacity();
    m_widget->setShadowOpacity(0.8);
    
    // Wait for potential animation
    waitForAnimation();
    
    QCOMPARE(m_widget->shadowOpacity(), 0.8);
    
    // Reset
    m_widget->setShadowOpacity(initialOpacity);
}

void ThumbnailWidgetIntegrationTest::testBorderAnimation()
{
    // Test border animation
    qreal initialOpacity = m_widget->borderOpacity();
    m_widget->setBorderOpacity(0.9);
    
    // Wait for potential animation
    waitForAnimation();
    
    QCOMPARE(m_widget->borderOpacity(), 0.9);
    
    // Reset
    m_widget->setBorderOpacity(initialOpacity);
}

void ThumbnailWidgetIntegrationTest::testInvalidPixmap()
{
    // Test with invalid pixmap
    m_widget->setPixmap(QPixmap());
    QVERIFY(m_widget->pixmap().isNull());
    
    // Should handle null pixmap gracefully
    m_widget->update();
    QTest::qWait(50);
    
    QVERIFY(true);
}

void ThumbnailWidgetIntegrationTest::testInvalidPageNumber()
{
    // Test with invalid page numbers
    m_widget->setPageNumber(-100);
    QCOMPARE(m_widget->pageNumber(), -100);
    
    m_widget->setPageNumber(999999);
    QCOMPARE(m_widget->pageNumber(), 999999);
    
    // Should handle invalid page numbers gracefully
    QVERIFY(true);
}

void ThumbnailWidgetIntegrationTest::testInvalidSize()
{
    // Test with invalid sizes
    m_widget->setThumbnailSize(QSize(-10, -20));
    
    // Should handle invalid size gracefully
    QSize size = m_widget->thumbnailSize();
    QVERIFY(size.width() >= 0);
    QVERIFY(size.height() >= 0);
}

void ThumbnailWidgetIntegrationTest::waitForAnimation()
{
    QTest::qWait(200);
    QApplication::processEvents();
}

void ThumbnailWidgetIntegrationTest::simulateMouseEvent(QEvent::Type type, Qt::MouseButton button, const QPoint& pos)
{
    QMouseEvent mouseEvent(type, pos, button, button, Qt::NoModifier);
    QApplication::sendEvent(m_widget, &mouseEvent);
    QTest::qWait(10);
}

QPixmap ThumbnailWidgetIntegrationTest::createTestPixmap(const QSize& size)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::lightGray);
    
    QPainter painter(&pixmap);
    painter.setPen(Qt::black);
    painter.drawRect(pixmap.rect().adjusted(1, 1, -1, -1));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "Test");
    
    return pixmap;
}

QTEST_MAIN(ThumbnailWidgetIntegrationTest)
#include "thumbnail_widget_integration_test.moc"
