#include <QColor>
#include <QPainter>
#include <QPixmap>
#include <QTest>
#include "../../app/delegate/AnnotationRenderDelegate.h"
#include "../../app/model/AnnotationModel.h"
#include "../TestUtilities.h"

class TestAnnotationRenderDelegate : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_delegate = new AnnotationRenderDelegate(); }

    void cleanup() {
        delete m_delegate;
        m_delegate = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_delegate != nullptr);
        QVERIFY(m_delegate->controller() == nullptr);
        QVERIFY(m_delegate->selectedAnnotationId().isEmpty());
        QVERIFY(!m_delegate->showSelectionHandles());
        QVERIFY(!m_delegate->highlightSelected());
    }

    void testSelectionManagement() {
        QVERIFY(m_delegate->selectedAnnotationId().isEmpty());

        m_delegate->setSelectedAnnotationId("test-annotation-1");
        QCOMPARE(m_delegate->selectedAnnotationId(),
                 QString("test-annotation-1"));

        m_delegate->setSelectedAnnotationId("test-annotation-2");
        QCOMPARE(m_delegate->selectedAnnotationId(),
                 QString("test-annotation-2"));

        m_delegate->clearSelection();
        QVERIFY(m_delegate->selectedAnnotationId().isEmpty());
    }

    void testRenderingOptions() {
        QVERIFY(!m_delegate->showSelectionHandles());
        m_delegate->setShowSelectionHandles(true);
        QVERIFY(m_delegate->showSelectionHandles());
        m_delegate->setShowSelectionHandles(false);
        QVERIFY(!m_delegate->showSelectionHandles());

        QVERIFY(!m_delegate->highlightSelected());
        m_delegate->setHighlightSelected(true);
        QVERIFY(m_delegate->highlightSelected());
        m_delegate->setHighlightSelected(false);
        QVERIFY(!m_delegate->highlightSelected());
    }

    void testRenderAnnotationWithHighlight() {
        QPixmap pixmap(200, 200);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        PDFAnnotation annotation;
        annotation.id = "highlight-1";
        annotation.type = PDFAnnotation::Highlight;
        annotation.pageNumber = 1;
        annotation.rect = QRectF(10, 10, 100, 20);
        annotation.color = QColor(255, 255, 0, 128);
        annotation.opacity = 0.5;

        m_delegate->renderAnnotation(&painter, annotation,
                                     QRectF(0, 0, 200, 200), 1.0);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testRenderAnnotationWithUnderline() {
        QPixmap pixmap(200, 200);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        PDFAnnotation annotation;
        annotation.id = "underline-1";
        annotation.type = PDFAnnotation::Underline;
        annotation.pageNumber = 1;
        annotation.rect = QRectF(10, 10, 100, 20);
        annotation.color = QColor(0, 0, 255);
        annotation.lineWidth = 2.0;

        m_delegate->renderAnnotation(&painter, annotation,
                                     QRectF(0, 0, 200, 200), 1.0);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testRenderAnnotationWithStrikeOut() {
        QPixmap pixmap(200, 200);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        PDFAnnotation annotation;
        annotation.id = "strikeout-1";
        annotation.type = PDFAnnotation::StrikeOut;
        annotation.pageNumber = 1;
        annotation.rect = QRectF(10, 10, 100, 20);
        annotation.color = QColor(255, 0, 0);

        m_delegate->renderAnnotation(&painter, annotation,
                                     QRectF(0, 0, 200, 200), 1.0);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testRenderAnnotationWithRectangle() {
        QPixmap pixmap(200, 200);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        PDFAnnotation annotation;
        annotation.id = "rect-1";
        annotation.type = PDFAnnotation::Rectangle;
        annotation.pageNumber = 1;
        annotation.rect = QRectF(20, 20, 80, 60);
        annotation.color = QColor(0, 128, 0);
        annotation.lineWidth = 2.0;

        m_delegate->renderAnnotation(&painter, annotation,
                                     QRectF(0, 0, 200, 200), 1.0);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testRenderAnnotationWithCircle() {
        QPixmap pixmap(200, 200);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        PDFAnnotation annotation;
        annotation.id = "circle-1";
        annotation.type = PDFAnnotation::Circle;
        annotation.pageNumber = 1;
        annotation.rect = QRectF(50, 50, 50, 50);
        annotation.color = QColor(128, 0, 128);
        annotation.lineWidth = 1.5;

        m_delegate->renderAnnotation(&painter, annotation,
                                     QRectF(0, 0, 200, 200), 1.0);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testRenderAnnotationWithZoom() {
        QPixmap pixmap(400, 400);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        PDFAnnotation annotation;
        annotation.id = "zoomed-1";
        annotation.type = PDFAnnotation::Highlight;
        annotation.pageNumber = 1;
        annotation.rect = QRectF(10, 10, 100, 20);
        annotation.color = QColor(255, 255, 0, 128);

        m_delegate->renderAnnotation(&painter, annotation,
                                     QRectF(0, 0, 400, 400), 2.0);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testRenderingCompletedSignal() {
        QSignalSpy spy(m_delegate,
                       &AnnotationRenderDelegate::renderingCompleted);

        QPixmap pixmap(200, 200);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        m_delegate->renderAnnotations(&painter, 1, QRectF(0, 0, 200, 200), 1.0);
        painter.end();

        QCOMPARE(spy.count(), 1);
        QList<QVariant> args = spy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 1);
    }

private:
    AnnotationRenderDelegate* m_delegate = nullptr;
};

QTEST_MAIN(TestAnnotationRenderDelegate)
#include "test_annotation_render_delegate.moc"
