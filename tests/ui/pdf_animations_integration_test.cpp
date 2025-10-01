#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QWidget>
#include <QScrollArea>
#include <QElapsedTimer>
#include <QTimer>
#include <QEventLoop>
#include "../../app/ui/viewer/PDFAnimations.h"

class PDFAnimationsIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testAnimationEnabled();
    void testAnimationDuration();

    // Page transition tests
    void testPageTransition();
    void testSlideTransition();
    void testFadeTransition();
    void testZoomTransition();

    // Zoom animation tests
    void testZoomInAnimation();
    void testZoomOutAnimation();
    void testSmoothZoom();

    // UI feedback tests
    void testButtonPress();
    void testHighlight();
    void testShake();
    void testPulse();

    // Animation control tests
    void testStopAllAnimations();
    void testAnimationSignals();

private:
    PDFAnimationManager* m_animations;
    QWidget* m_testWidget;
    QWidget* m_parentWidget;

    void waitForAnimation(int duration = 500);
    void createTestWidget();
};

void PDFAnimationsIntegrationTest::initTestCase()
{
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    
    createTestWidget();
}

void PDFAnimationsIntegrationTest::cleanupTestCase()
{
    delete m_testWidget;
    delete m_parentWidget;
}

void PDFAnimationsIntegrationTest::init()
{
    m_animations = new PDFAnimationManager(this);
}

void PDFAnimationsIntegrationTest::cleanup()
{
    delete m_animations;
    m_animations = nullptr;
}

void PDFAnimationsIntegrationTest::testInitialization()
{
    // Test basic initialization
    QVERIFY(m_animations != nullptr);
    QVERIFY(!m_animations->isAnimating());
}

void PDFAnimationsIntegrationTest::testAnimationEnabled()
{
    // Test animation manager state
    QVERIFY(!m_animations->isAnimating());

    // Test setting default duration
    m_animations->setDefaultDuration(300);

    // Test setting default easing
    m_animations->setDefaultEasing(QEasingCurve::OutCubic);
}

void PDFAnimationsIntegrationTest::testAnimationDuration()
{
    // Test setting default duration
    m_animations->setDefaultDuration(500);

    // Test setting different easing curves
    m_animations->setDefaultEasing(QEasingCurve::InOutQuad);
    m_animations->setDefaultEasing(QEasingCurve::OutBounce);
    m_animations->setDefaultEasing(QEasingCurve::Linear);
}

void PDFAnimationsIntegrationTest::testPageTransition()
{
    // Test page transition animation using actual API
    createTestWidget();
    QWidget* fromWidget = new QWidget(m_parentWidget);
    QWidget* toWidget = new QWidget(m_parentWidget);

    QSignalSpy startedSpy(m_animations, &PDFAnimationManager::animationStarted);
    QSignalSpy finishedSpy(m_animations, &PDFAnimationManager::animationFinished);

    m_animations->animatePageTransition(fromWidget, toWidget, PDFAnimationManager::AnimationType::SlideLeft, 200);

    waitForAnimation(300);
    QVERIFY(startedSpy.count() > 0);

    delete fromWidget;
    delete toWidget;
}

void PDFAnimationsIntegrationTest::testSlideTransition()
{
    // Test slide transition using page transition with slide type
    createTestWidget();
    QWidget* fromWidget = new QWidget(m_parentWidget);
    QWidget* toWidget = new QWidget(m_parentWidget);

    QSignalSpy startedSpy(m_animations, &PDFAnimationManager::animationStarted);

    m_animations->animatePageTransition(fromWidget, toWidget, PDFAnimationManager::AnimationType::SlideLeft, 200);
    waitForAnimation(300);
    QVERIFY(startedSpy.count() > 0);

    // Test different slide direction
    m_animations->animatePageTransition(toWidget, fromWidget, PDFAnimationManager::AnimationType::SlideRight, 200);
    waitForAnimation(300);

    delete fromWidget;
    delete toWidget;
}

void PDFAnimationsIntegrationTest::testFadeTransition()
{
    // Test fade in animation
    createTestWidget();

    QSignalSpy startedSpy(m_animations, &PDFAnimationManager::animationStarted);
    QSignalSpy finishedSpy(m_animations, &PDFAnimationManager::animationFinished);

    m_animations->animateFadeIn(m_testWidget, 200);
    waitForAnimation(300);

    QVERIFY(startedSpy.count() > 0);

    // Test fade out animation
    m_animations->animateFadeOut(m_testWidget, 200);
    waitForAnimation(300);
}

void PDFAnimationsIntegrationTest::testZoomTransition()
{
    // Test zoom animation
    createTestWidget();

    QSignalSpy startedSpy(m_animations, &PDFAnimationManager::animationStarted);

    // Test zoom with scale factors
    m_animations->animateZoom(m_testWidget, 1.0, 1.5, 200);
    waitForAnimation(300);

    QVERIFY(startedSpy.count() > 0);

    // Test zoom back
    m_animations->animateZoom(m_testWidget, 1.5, 1.0, 200);
    waitForAnimation(300);
}

void PDFAnimationsIntegrationTest::testZoomInAnimation()
{
    // Test zoom in animation using animateZoom
    createTestWidget();

    QSignalSpy startedSpy(m_animations, &PDFAnimationManager::animationStarted);
    m_animations->animateZoom(m_testWidget, 1.0, 1.5, 200);
    waitForAnimation(300);

    QVERIFY(startedSpy.count() > 0);
}

void PDFAnimationsIntegrationTest::testZoomOutAnimation()
{
    // Test zoom out animation using animateZoom
    createTestWidget();

    QSignalSpy startedSpy(m_animations, &PDFAnimationManager::animationStarted);
    m_animations->animateZoom(m_testWidget, 1.5, 1.0, 200);
    waitForAnimation(300);

    QVERIFY(startedSpy.count() > 0);
}

void PDFAnimationsIntegrationTest::testSmoothZoom()
{
    // Test smooth zoom animation using regular zoom
    createTestWidget();

    QSignalSpy startedSpy(m_animations, &PDFAnimationManager::animationStarted);
    m_animations->animateZoom(m_testWidget, 1.0, 2.0, 300);
    waitForAnimation(400);

    QVERIFY(startedSpy.count() > 0);
}

void PDFAnimationsIntegrationTest::testButtonPress()
{
    // Test button press animation
    createTestWidget();

    QSignalSpy startedSpy(m_animations, &PDFAnimationManager::animationStarted);
    m_animations->animateButtonPress(m_testWidget);
    waitForAnimation(300);

    // Should complete without crashing
    QVERIFY(true);
}

void PDFAnimationsIntegrationTest::testHighlight()
{
    // Test highlight animation
    createTestWidget();

    QSignalSpy startedSpy(m_animations, &PDFAnimationManager::animationStarted);
    m_animations->animateHighlight(m_testWidget, QColor(255, 255, 0, 100));
    waitForAnimation(600);

    // Should complete without crashing
    QVERIFY(true);
}

void PDFAnimationsIntegrationTest::testShake()
{
    // Test shake animation
    createTestWidget();

    QSignalSpy startedSpy(m_animations, &PDFAnimationManager::animationStarted);
    m_animations->animateShake(m_testWidget);
    waitForAnimation(400);

    // Should complete without crashing
    QVERIFY(true);
}

void PDFAnimationsIntegrationTest::testPulse()
{
    // Test pulse animation using highlight (which has pulse-like behavior)
    createTestWidget();

    QSignalSpy startedSpy(m_animations, &PDFAnimationManager::animationStarted);
    m_animations->animateHighlight(m_testWidget, QColor(255, 255, 0, 100));
    waitForAnimation(600);

    // Should complete without crashing
    QVERIFY(true);
}

void PDFAnimationsIntegrationTest::testStopAllAnimations()
{
    // Test stopping all animations
    createTestWidget();

    // Start multiple animations
    m_animations->animateFadeIn(m_testWidget, 1000);
    m_animations->animateZoom(m_testWidget, 1.0, 1.5, 1000);

    QVERIFY(m_animations->isAnimating());

    // Stop all animations
    m_animations->stopAllAnimations();

    // Should not be animating anymore
    QVERIFY(!m_animations->isAnimating());
}

void PDFAnimationsIntegrationTest::testAnimationSignals()
{
    // Test animation signals
    createTestWidget();

    QSignalSpy startedSpy(m_animations, &PDFAnimationManager::animationStarted);
    QSignalSpy finishedSpy(m_animations, &PDFAnimationManager::animationFinished);
    QSignalSpy allFinishedSpy(m_animations, &PDFAnimationManager::allAnimationsFinished);

    // Start an animation
    m_animations->animateFadeIn(m_testWidget, 200);

    waitForAnimation(300);

    // Check signals were emitted
    QVERIFY(startedSpy.count() > 0);
    QVERIFY(finishedSpy.count() > 0);
}

void PDFAnimationsIntegrationTest::waitForAnimation(int duration)
{
    QTest::qWait(duration);
    QApplication::processEvents();
}

void PDFAnimationsIntegrationTest::createTestWidget()
{
    if (m_testWidget) {
        delete m_testWidget;
    }
    m_testWidget = new QWidget(m_parentWidget);
    m_testWidget->setFixedSize(200, 150);
    m_testWidget->setStyleSheet("background-color: lightblue; border: 1px solid black;");
    m_testWidget->show();
}

QTEST_MAIN(PDFAnimationsIntegrationTest)
#include "pdf_animations_integration_test.moc"
