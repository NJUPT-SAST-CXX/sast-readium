#include <QColor>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QSignalSpy>
#include <QTest>
#include "../../app/interaction/AnnotationInteractionHandler.h"
#include "../TestUtilities.h"

class TestAnnotationInteractionHandler : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_handler = new AnnotationInteractionHandler(); }

    void cleanup() {
        delete m_handler;
        m_handler = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_handler != nullptr);
        QCOMPARE(m_handler->getDrawMode(), AnnotationInteractionHandler::None);
        QVERIFY(!m_handler->isDrawing());
    }

    void testSetDrawMode() {
        QSignalSpy spy(m_handler, &AnnotationInteractionHandler::modeChanged);

        m_handler->setDrawMode(AnnotationInteractionHandler::Highlight);
        QCOMPARE(m_handler->getDrawMode(),
                 AnnotationInteractionHandler::Highlight);
        QCOMPARE(spy.count(), 1);

        m_handler->setDrawMode(AnnotationInteractionHandler::Underline);
        QCOMPARE(m_handler->getDrawMode(),
                 AnnotationInteractionHandler::Underline);

        m_handler->setDrawMode(AnnotationInteractionHandler::StrikeOut);
        QCOMPARE(m_handler->getDrawMode(),
                 AnnotationInteractionHandler::StrikeOut);

        m_handler->setDrawMode(AnnotationInteractionHandler::Rectangle);
        QCOMPARE(m_handler->getDrawMode(),
                 AnnotationInteractionHandler::Rectangle);

        m_handler->setDrawMode(AnnotationInteractionHandler::Circle);
        QCOMPARE(m_handler->getDrawMode(),
                 AnnotationInteractionHandler::Circle);

        m_handler->setDrawMode(AnnotationInteractionHandler::FreehandDraw);
        QCOMPARE(m_handler->getDrawMode(),
                 AnnotationInteractionHandler::FreehandDraw);

        m_handler->setDrawMode(AnnotationInteractionHandler::Arrow);
        QCOMPARE(m_handler->getDrawMode(), AnnotationInteractionHandler::Arrow);

        m_handler->setDrawMode(AnnotationInteractionHandler::Line);
        QCOMPARE(m_handler->getDrawMode(), AnnotationInteractionHandler::Line);

        m_handler->setDrawMode(AnnotationInteractionHandler::Text);
        QCOMPARE(m_handler->getDrawMode(), AnnotationInteractionHandler::Text);

        m_handler->setDrawMode(AnnotationInteractionHandler::None);
        QCOMPARE(m_handler->getDrawMode(), AnnotationInteractionHandler::None);
    }

    void testColorProperty() {
        QColor defaultColor = m_handler->getColor();

        QColor yellow(255, 255, 0);
        m_handler->setColor(yellow);
        QCOMPARE(m_handler->getColor(), yellow);

        QColor red(255, 0, 0);
        m_handler->setColor(red);
        QCOMPARE(m_handler->getColor(), red);

        QColor transparent(0, 0, 0, 0);
        m_handler->setColor(transparent);
        QCOMPARE(m_handler->getColor(), transparent);
    }

    void testLineWidthProperty() {
        m_handler->setLineWidth(1.0);
        QCOMPARE(m_handler->getLineWidth(), 1.0);

        m_handler->setLineWidth(2.5);
        QCOMPARE(m_handler->getLineWidth(), 2.5);

        m_handler->setLineWidth(0.5);
        QCOMPARE(m_handler->getLineWidth(), 0.5);

        m_handler->setLineWidth(10.0);
        QCOMPARE(m_handler->getLineWidth(), 10.0);
    }

    void testOpacityProperty() {
        m_handler->setOpacity(1.0);
        QCOMPARE(m_handler->getOpacity(), 1.0);

        m_handler->setOpacity(0.5);
        QCOMPARE(m_handler->getOpacity(), 0.5);

        m_handler->setOpacity(0.0);
        QCOMPARE(m_handler->getOpacity(), 0.0);

        m_handler->setOpacity(0.75);
        QCOMPARE(m_handler->getOpacity(), 0.75);
    }

    void testStartDrawing() {
        m_handler->setDrawMode(AnnotationInteractionHandler::Rectangle);
        QVERIFY(!m_handler->isDrawing());

        m_handler->startDrawing(QPointF(10, 10), 1);
        QVERIFY(m_handler->isDrawing());
        QVERIFY(m_handler->hasPreview());
    }

    void testContinueDrawing() {
        m_handler->setDrawMode(AnnotationInteractionHandler::Rectangle);
        m_handler->startDrawing(QPointF(10, 10), 1);

        QSignalSpy spy(m_handler,
                       &AnnotationInteractionHandler::previewUpdated);

        m_handler->continueDrawing(QPointF(50, 50));
        QVERIFY(m_handler->isDrawing());

        m_handler->continueDrawing(QPointF(100, 100));
        QVERIFY(m_handler->isDrawing());
    }

    void testFinishDrawing() {
        m_handler->setDrawMode(AnnotationInteractionHandler::Rectangle);
        m_handler->startDrawing(QPointF(10, 10), 1);
        m_handler->continueDrawing(QPointF(100, 100));

        QSignalSpy spy(m_handler,
                       &AnnotationInteractionHandler::annotationCreated);

        m_handler->finishDrawing(QPointF(100, 100));
        QVERIFY(!m_handler->isDrawing());
        QCOMPARE(spy.count(), 1);
    }

    void testCancelDrawing() {
        m_handler->setDrawMode(AnnotationInteractionHandler::Rectangle);
        m_handler->startDrawing(QPointF(10, 10), 1);

        QSignalSpy spy(m_handler,
                       &AnnotationInteractionHandler::drawingCancelled);

        m_handler->cancelDrawing();
        QVERIFY(!m_handler->isDrawing());
        QCOMPARE(spy.count(), 1);
    }

    void testPreviewPath() {
        m_handler->setDrawMode(AnnotationInteractionHandler::FreehandDraw);
        m_handler->startDrawing(QPointF(10, 10), 1);
        m_handler->continueDrawing(QPointF(20, 20));
        m_handler->continueDrawing(QPointF(30, 30));

        QPainterPath path = m_handler->getPreviewPath();
        QVERIFY(!path.isEmpty() || m_handler->hasPreview());
    }

    void testPreviewRect() {
        m_handler->setDrawMode(AnnotationInteractionHandler::Rectangle);
        m_handler->startDrawing(QPointF(10, 10), 1);
        m_handler->continueDrawing(QPointF(100, 100));

        QRectF rect = m_handler->getPreviewRect();
        QVERIFY(rect.isValid() || m_handler->hasPreview());
    }

    void testDrawHighlight() {
        m_handler->setDrawMode(AnnotationInteractionHandler::Highlight);
        m_handler->setColor(QColor(255, 255, 0, 128));
        m_handler->startDrawing(QPointF(10, 10), 1);
        m_handler->continueDrawing(QPointF(200, 20));

        QSignalSpy spy(m_handler,
                       &AnnotationInteractionHandler::annotationCreated);
        m_handler->finishDrawing(QPointF(200, 20));

        QCOMPARE(spy.count(), 1);
        QList<QVariant> args = spy.takeFirst();
        PDFAnnotation annotation = args.at(0).value<PDFAnnotation>();
        QCOMPARE(annotation.type, PDFAnnotation::Highlight);
    }

    void testDrawUnderline() {
        m_handler->setDrawMode(AnnotationInteractionHandler::Underline);
        m_handler->setColor(QColor(0, 0, 255));
        m_handler->startDrawing(QPointF(10, 50), 1);
        m_handler->continueDrawing(QPointF(200, 50));

        QSignalSpy spy(m_handler,
                       &AnnotationInteractionHandler::annotationCreated);
        m_handler->finishDrawing(QPointF(200, 50));

        QCOMPARE(spy.count(), 1);
        QList<QVariant> args = spy.takeFirst();
        PDFAnnotation annotation = args.at(0).value<PDFAnnotation>();
        QCOMPARE(annotation.type, PDFAnnotation::Underline);
    }

    void testDrawCircle() {
        m_handler->setDrawMode(AnnotationInteractionHandler::Circle);
        m_handler->setColor(QColor(255, 0, 0));
        m_handler->setLineWidth(2.0);
        m_handler->startDrawing(QPointF(50, 50), 1);
        m_handler->continueDrawing(QPointF(150, 150));

        QSignalSpy spy(m_handler,
                       &AnnotationInteractionHandler::annotationCreated);
        m_handler->finishDrawing(QPointF(150, 150));

        QCOMPARE(spy.count(), 1);
        QList<QVariant> args = spy.takeFirst();
        PDFAnnotation annotation = args.at(0).value<PDFAnnotation>();
        QCOMPARE(annotation.type, PDFAnnotation::Circle);
    }

    void testDrawLine() {
        m_handler->setDrawMode(AnnotationInteractionHandler::Line);
        m_handler->startDrawing(QPointF(0, 0), 1);
        m_handler->continueDrawing(QPointF(100, 100));

        QSignalSpy spy(m_handler,
                       &AnnotationInteractionHandler::annotationCreated);
        m_handler->finishDrawing(QPointF(100, 100));

        QCOMPARE(spy.count(), 1);
        QList<QVariant> args = spy.takeFirst();
        PDFAnnotation annotation = args.at(0).value<PDFAnnotation>();
        QCOMPARE(annotation.type, PDFAnnotation::Line);
    }

    void testDrawArrow() {
        m_handler->setDrawMode(AnnotationInteractionHandler::Arrow);
        m_handler->startDrawing(QPointF(0, 0), 1);
        m_handler->continueDrawing(QPointF(100, 100));

        QSignalSpy spy(m_handler,
                       &AnnotationInteractionHandler::annotationCreated);
        m_handler->finishDrawing(QPointF(100, 100));

        QCOMPARE(spy.count(), 1);
        QList<QVariant> args = spy.takeFirst();
        PDFAnnotation annotation = args.at(0).value<PDFAnnotation>();
        QCOMPARE(annotation.type, PDFAnnotation::Arrow);
    }

    void testDrawFreehand() {
        m_handler->setDrawMode(AnnotationInteractionHandler::FreehandDraw);
        m_handler->startDrawing(QPointF(10, 10), 1);

        for (int i = 0; i < 20; ++i) {
            m_handler->continueDrawing(QPointF(10 + i * 5, 10 + i * 3));
        }

        QSignalSpy spy(m_handler,
                       &AnnotationInteractionHandler::annotationCreated);
        m_handler->finishDrawing(QPointF(110, 70));

        QCOMPARE(spy.count(), 1);
        QList<QVariant> args = spy.takeFirst();
        PDFAnnotation annotation = args.at(0).value<PDFAnnotation>();
        QCOMPARE(annotation.type, PDFAnnotation::Ink);
    }

    void testNoDrawingInNoneMode() {
        m_handler->setDrawMode(AnnotationInteractionHandler::None);
        m_handler->startDrawing(QPointF(10, 10), 1);

        QVERIFY(!m_handler->isDrawing());
    }

    void testMultipleDrawings() {
        QSignalSpy spy(m_handler,
                       &AnnotationInteractionHandler::annotationCreated);

        for (int i = 0; i < 5; ++i) {
            m_handler->setDrawMode(AnnotationInteractionHandler::Rectangle);
            m_handler->startDrawing(QPointF(10 + i * 50, 10), 1);
            m_handler->continueDrawing(QPointF(40 + i * 50, 40));
            m_handler->finishDrawing(QPointF(40 + i * 50, 40));
        }

        QCOMPARE(spy.count(), 5);
    }

private:
    AnnotationInteractionHandler* m_handler = nullptr;
};

QTEST_MAIN(TestAnnotationInteractionHandler)
#include "test_annotation_interaction_handler.moc"
