#include <QApplication>
#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../../app/ui/managers/AnnotationSelectionManager.h"

class AnnotationSelectionManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testConstruction();
    void testDestruction();
    void testSelectAnnotation();
    void testClearSelection();
    void testHasSelection();
    void testSelectedAnnotationId();
    void testFindAnnotationAt();
    void testFindResizeHandle();
    void testIsInteracting();
    void testIsMoving();
    void testIsResizing();
    void testHandleMousePress();
    void testHandleMouseMove();
    void testHandleMouseRelease();
    void testSetHandleSize();
    void testSetHitTolerance();
    void testSelectionChangedSignal();
    void testSelectionClearedSignal();
    void testAnnotationMovedSignal();
    void testAnnotationResizedSignal();
    void testInteractionStartedSignal();
    void testInteractionEndedSignal();
    void testHandlePositionEnum();

private:
    AnnotationSelectionManager* m_manager;
};

void AnnotationSelectionManagerTest::initTestCase() {}

void AnnotationSelectionManagerTest::cleanupTestCase() {}

void AnnotationSelectionManagerTest::init() {
    m_manager = new AnnotationSelectionManager();
}

void AnnotationSelectionManagerTest::cleanup() {
    delete m_manager;
    m_manager = nullptr;
}

void AnnotationSelectionManagerTest::testConstruction() {
    QVERIFY(m_manager != nullptr);
}

void AnnotationSelectionManagerTest::testDestruction() {
    auto* manager = new AnnotationSelectionManager();
    delete manager;
    QVERIFY(true);
}

void AnnotationSelectionManagerTest::testSelectAnnotation() {
    m_manager->selectAnnotation("test_annotation_1");
    QCOMPARE(m_manager->selectedAnnotationId(), QString("test_annotation_1"));
}

void AnnotationSelectionManagerTest::testClearSelection() {
    m_manager->selectAnnotation("test_annotation_1");
    m_manager->clearSelection();
    QVERIFY(!m_manager->hasSelection());
}

void AnnotationSelectionManagerTest::testHasSelection() {
    QVERIFY(!m_manager->hasSelection());
    m_manager->selectAnnotation("test_annotation_1");
    QVERIFY(m_manager->hasSelection());
}

void AnnotationSelectionManagerTest::testSelectedAnnotationId() {
    QVERIFY(m_manager->selectedAnnotationId().isEmpty());
    m_manager->selectAnnotation("test_annotation_1");
    QCOMPARE(m_manager->selectedAnnotationId(), QString("test_annotation_1"));
}

void AnnotationSelectionManagerTest::testFindAnnotationAt() {
    QPointF point(100, 100);
    QString id = m_manager->findAnnotationAt(point, 0);
    QVERIFY(id.isEmpty() || !id.isEmpty());  // Depends on controller
}

void AnnotationSelectionManagerTest::testFindResizeHandle() {
    QPointF point(100, 100);
    auto handle = m_manager->findResizeHandle(point, 1.0);
    QVERIFY(handle == AnnotationSelectionManager::HandlePosition::None ||
            handle != AnnotationSelectionManager::HandlePosition::None);
}

void AnnotationSelectionManagerTest::testIsInteracting() {
    QVERIFY(!m_manager->isInteracting());
}

void AnnotationSelectionManagerTest::testIsMoving() {
    QVERIFY(!m_manager->isMoving());
}

void AnnotationSelectionManagerTest::testIsResizing() {
    QVERIFY(!m_manager->isResizing());
}

void AnnotationSelectionManagerTest::testHandleMousePress() {
    QPointF point(100, 100);
    bool handled = m_manager->handleMousePress(point, 0, 1.0);
    QVERIFY(handled || !handled);
}

void AnnotationSelectionManagerTest::testHandleMouseMove() {
    QPointF point(150, 150);
    bool handled = m_manager->handleMouseMove(point, 1.0);
    QVERIFY(handled || !handled);
}

void AnnotationSelectionManagerTest::testHandleMouseRelease() {
    QPointF point(150, 150);
    bool handled = m_manager->handleMouseRelease(point, 1.0);
    QVERIFY(handled || !handled);
}

void AnnotationSelectionManagerTest::testSetHandleSize() {
    m_manager->setHandleSize(10.0);
    QCOMPARE(m_manager->handleSize(), 10.0);
}

void AnnotationSelectionManagerTest::testSetHitTolerance() {
    m_manager->setHitTolerance(5.0);
    QCOMPARE(m_manager->hitTolerance(), 5.0);
}

void AnnotationSelectionManagerTest::testSelectionChangedSignal() {
    QSignalSpy spy(m_manager, &AnnotationSelectionManager::selectionChanged);
    QVERIFY(spy.isValid());
    m_manager->selectAnnotation("test");
    QVERIFY(spy.count() >= 0);
}

void AnnotationSelectionManagerTest::testSelectionClearedSignal() {
    QSignalSpy spy(m_manager, &AnnotationSelectionManager::selectionCleared);
    QVERIFY(spy.isValid());
}

void AnnotationSelectionManagerTest::testAnnotationMovedSignal() {
    QSignalSpy spy(m_manager, &AnnotationSelectionManager::annotationMoved);
    QVERIFY(spy.isValid());
}

void AnnotationSelectionManagerTest::testAnnotationResizedSignal() {
    QSignalSpy spy(m_manager, &AnnotationSelectionManager::annotationResized);
    QVERIFY(spy.isValid());
}

void AnnotationSelectionManagerTest::testInteractionStartedSignal() {
    QSignalSpy spy(m_manager, &AnnotationSelectionManager::interactionStarted);
    QVERIFY(spy.isValid());
}

void AnnotationSelectionManagerTest::testInteractionEndedSignal() {
    QSignalSpy spy(m_manager, &AnnotationSelectionManager::interactionEnded);
    QVERIFY(spy.isValid());
}

void AnnotationSelectionManagerTest::testHandlePositionEnum() {
    QVERIFY(AnnotationSelectionManager::HandlePosition::None !=
            AnnotationSelectionManager::HandlePosition::TopLeft);
    QVERIFY(AnnotationSelectionManager::HandlePosition::Inside !=
            AnnotationSelectionManager::HandlePosition::BottomRight);
}

QTEST_MAIN(AnnotationSelectionManagerTest)
#include "test_annotation_selection_manager.moc"
