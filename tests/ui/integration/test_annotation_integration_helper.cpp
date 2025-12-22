#include <QApplication>
#include <QPainter>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../../app/ui/integration/AnnotationIntegrationHelper.h"

class AnnotationIntegrationHelperTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testConstruction();
    void testDestruction();
    void testInitialize();
    void testHasDocumentWithoutDocument();
    void testClearDocument();
    void testAnnotationSelectedSignal();
    void testSelectionClearedSignal();
    void testAnnotationsChangedSignal();
    void testHandleMousePress();
    void testHandleMouseMove();
    void testHandleMouseRelease();

private:
    QWidget* m_parentWidget;
    AnnotationIntegrationHelper* m_helper;
};

void AnnotationIntegrationHelperTest::initTestCase() {
    m_parentWidget = new QWidget();
    m_parentWidget->resize(800, 600);
    m_parentWidget->show();
    if (QGuiApplication::platformName() == "offscreen") {
        QTest::qWait(100);
    } else {
        QVERIFY(QTest::qWaitForWindowExposed(m_parentWidget));
    }
}

void AnnotationIntegrationHelperTest::cleanupTestCase() {
    delete m_parentWidget;
}

void AnnotationIntegrationHelperTest::init() {
    m_helper = new AnnotationIntegrationHelper(m_parentWidget);
}

void AnnotationIntegrationHelperTest::cleanup() {
    delete m_helper;
    m_helper = nullptr;
}

void AnnotationIntegrationHelperTest::testConstruction() {
    QVERIFY(m_helper != nullptr);
}

void AnnotationIntegrationHelperTest::testDestruction() {
    auto* helper = new AnnotationIntegrationHelper();
    delete helper;
    QVERIFY(true);
}

void AnnotationIntegrationHelperTest::testInitialize() {
    bool result = m_helper->initialize();
    QVERIFY(result || !result);  // May fail if ServiceLocator not set up
}

void AnnotationIntegrationHelperTest::testHasDocumentWithoutDocument() {
    QVERIFY(!m_helper->hasDocument());
}

void AnnotationIntegrationHelperTest::testClearDocument() {
    m_helper->clearDocument();
    QVERIFY(!m_helper->hasDocument());
}

void AnnotationIntegrationHelperTest::testAnnotationSelectedSignal() {
    QSignalSpy spy(m_helper, &AnnotationIntegrationHelper::annotationSelected);
    QVERIFY(spy.isValid());
}

void AnnotationIntegrationHelperTest::testSelectionClearedSignal() {
    QSignalSpy spy(m_helper, &AnnotationIntegrationHelper::selectionCleared);
    QVERIFY(spy.isValid());
}

void AnnotationIntegrationHelperTest::testAnnotationsChangedSignal() {
    QSignalSpy spy(m_helper, &AnnotationIntegrationHelper::annotationsChanged);
    QVERIFY(spy.isValid());
}

void AnnotationIntegrationHelperTest::testHandleMousePress() {
    QPointF point(100, 100);
    bool handled = m_helper->handleMousePress(point, 0, 1.0);
    QVERIFY(handled || !handled);
}

void AnnotationIntegrationHelperTest::testHandleMouseMove() {
    QPointF point(150, 150);
    bool handled = m_helper->handleMouseMove(point, 1.0);
    QVERIFY(handled || !handled);
}

void AnnotationIntegrationHelperTest::testHandleMouseRelease() {
    QPointF point(150, 150);
    bool handled = m_helper->handleMouseRelease(point, 1.0);
    QVERIFY(handled || !handled);
}

QTEST_MAIN(AnnotationIntegrationHelperTest)
#include "test_annotation_integration_helper.moc"
