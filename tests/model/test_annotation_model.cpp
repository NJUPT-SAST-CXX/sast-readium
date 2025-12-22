#include <QColor>
#include <QSignalSpy>
#include <QTest>
#include "../../app/model/AnnotationModel.h"
#include "../TestUtilities.h"

class TestAnnotationModel : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_model = new AnnotationModel(); }

    void cleanup() {
        delete m_model;
        m_model = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_model != nullptr);
        QCOMPARE(m_model->rowCount(), 0);
        QCOMPARE(m_model->getTotalAnnotationCount(), 0);
    }

    void testAddAnnotation() {
        QSignalSpy spy(m_model, &AnnotationModel::annotationAdded);

        PDFAnnotation annotation;
        annotation.type = AnnotationType::Highlight;
        annotation.pageNumber = 0;
        annotation.boundingRect = QRectF(10, 10, 100, 20);
        annotation.color = Qt::yellow;

        bool result = m_model->addAnnotation(annotation);
        QVERIFY(result);
        QCOMPARE(m_model->rowCount(), 1);
        QCOMPARE(spy.count(), 1);
    }

    void testRemoveAnnotation() {
        PDFAnnotation annotation;
        annotation.type = AnnotationType::Highlight;
        annotation.pageNumber = 0;
        m_model->addAnnotation(annotation);

        QString id = m_model->getAllAnnotations().first().id;

        QSignalSpy spy(m_model, &AnnotationModel::annotationRemoved);

        bool result = m_model->removeAnnotation(id);
        QVERIFY(result);
        QCOMPARE(m_model->rowCount(), 0);
        QCOMPARE(spy.count(), 1);
    }

    void testRemoveNonExistentAnnotation() {
        bool result = m_model->removeAnnotation("non-existent-id");
        QVERIFY(!result);
    }

    void testUpdateAnnotation() {
        PDFAnnotation annotation;
        annotation.type = AnnotationType::Highlight;
        annotation.pageNumber = 0;
        annotation.content = "Original";
        m_model->addAnnotation(annotation);

        QString id = m_model->getAllAnnotations().first().id;

        QSignalSpy spy(m_model, &AnnotationModel::annotationUpdated);

        PDFAnnotation updated = m_model->getAnnotation(id);
        updated.content = "Updated";

        bool result = m_model->updateAnnotation(id, updated);
        QVERIFY(result);
        QCOMPARE(spy.count(), 1);

        PDFAnnotation retrieved = m_model->getAnnotation(id);
        QCOMPARE(retrieved.content, QString("Updated"));
    }

    void testGetAnnotationsForPage() {
        for (int i = 0; i < 5; ++i) {
            PDFAnnotation annotation;
            annotation.type = AnnotationType::Highlight;
            annotation.pageNumber = i % 2;
            m_model->addAnnotation(annotation);
        }

        QList<PDFAnnotation> page0Annotations =
            m_model->getAnnotationsForPage(0);
        QList<PDFAnnotation> page1Annotations =
            m_model->getAnnotationsForPage(1);

        QCOMPARE(page0Annotations.size(), 3);
        QCOMPARE(page1Annotations.size(), 2);
    }

    void testRemoveAnnotationsForPage() {
        for (int i = 0; i < 5; ++i) {
            PDFAnnotation annotation;
            annotation.type = AnnotationType::Highlight;
            annotation.pageNumber = i % 2;
            m_model->addAnnotation(annotation);
        }

        bool result = m_model->removeAnnotationsForPage(0);
        QVERIFY(result);
        QCOMPARE(m_model->getAnnotationsForPage(0).size(), 0);
        QCOMPARE(m_model->getAnnotationsForPage(1).size(), 2);
    }

    void testGetAnnotationCountForPage() {
        for (int i = 0; i < 10; ++i) {
            PDFAnnotation annotation;
            annotation.type = AnnotationType::Highlight;
            annotation.pageNumber = i % 3;
            m_model->addAnnotation(annotation);
        }

        QCOMPARE(m_model->getAnnotationCountForPage(0), 4);
        QCOMPARE(m_model->getAnnotationCountForPage(1), 3);
        QCOMPARE(m_model->getAnnotationCountForPage(2), 3);
    }

    void testClearAnnotations() {
        for (int i = 0; i < 5; ++i) {
            PDFAnnotation annotation;
            annotation.type = AnnotationType::Highlight;
            annotation.pageNumber = 0;
            m_model->addAnnotation(annotation);
        }

        QSignalSpy spy(m_model, &AnnotationModel::annotationsCleared);

        m_model->clearAnnotations();

        QCOMPARE(m_model->rowCount(), 0);
        QCOMPARE(spy.count(), 1);
    }

    void testSearchAnnotations() {
        PDFAnnotation annotation1;
        annotation1.type = AnnotationType::Note;
        annotation1.pageNumber = 0;
        annotation1.content = "This is a test note";
        m_model->addAnnotation(annotation1);

        PDFAnnotation annotation2;
        annotation2.type = AnnotationType::Note;
        annotation2.pageNumber = 1;
        annotation2.content = "Another annotation";
        m_model->addAnnotation(annotation2);

        QList<PDFAnnotation> results = m_model->searchAnnotations("test");
        QCOMPARE(results.size(), 1);
        QVERIFY(results.first().content.contains("test"));
    }

    void testGetAnnotationsByType() {
        PDFAnnotation highlight;
        highlight.type = AnnotationType::Highlight;
        highlight.pageNumber = 0;
        m_model->addAnnotation(highlight);

        PDFAnnotation note;
        note.type = AnnotationType::Note;
        note.pageNumber = 0;
        m_model->addAnnotation(note);

        PDFAnnotation underline;
        underline.type = AnnotationType::Underline;
        underline.pageNumber = 0;
        m_model->addAnnotation(underline);

        QList<PDFAnnotation> highlights =
            m_model->getAnnotationsByType(AnnotationType::Highlight);
        QCOMPARE(highlights.size(), 1);

        QList<PDFAnnotation> notes =
            m_model->getAnnotationsByType(AnnotationType::Note);
        QCOMPARE(notes.size(), 1);
    }

    void testGetAnnotationsByAuthor() {
        PDFAnnotation annotation1;
        annotation1.type = AnnotationType::Note;
        annotation1.pageNumber = 0;
        annotation1.author = "Alice";
        m_model->addAnnotation(annotation1);

        PDFAnnotation annotation2;
        annotation2.type = AnnotationType::Note;
        annotation2.pageNumber = 0;
        annotation2.author = "Bob";
        m_model->addAnnotation(annotation2);

        PDFAnnotation annotation3;
        annotation3.type = AnnotationType::Note;
        annotation3.pageNumber = 0;
        annotation3.author = "Alice";
        m_model->addAnnotation(annotation3);

        QList<PDFAnnotation> aliceAnnotations =
            m_model->getAnnotationsByAuthor("Alice");
        QCOMPARE(aliceAnnotations.size(), 2);
    }

    void testEditAnnotationContent() {
        PDFAnnotation annotation;
        annotation.type = AnnotationType::Note;
        annotation.pageNumber = 0;
        annotation.content = "Original content";
        m_model->addAnnotation(annotation);

        QString id = m_model->getAllAnnotations().first().id;

        bool result = m_model->editAnnotationContent(id, "New content");
        QVERIFY(result);

        PDFAnnotation retrieved = m_model->getAnnotation(id);
        QCOMPARE(retrieved.content, QString("New content"));
    }

    void testChangeAnnotationColor() {
        PDFAnnotation annotation;
        annotation.type = AnnotationType::Highlight;
        annotation.pageNumber = 0;
        annotation.color = Qt::yellow;
        m_model->addAnnotation(annotation);

        QString id = m_model->getAllAnnotations().first().id;

        bool result = m_model->changeAnnotationColor(id, Qt::red);
        QVERIFY(result);

        PDFAnnotation retrieved = m_model->getAnnotation(id);
        QCOMPARE(retrieved.color, QColor(Qt::red));
    }

    void testChangeAnnotationOpacity() {
        PDFAnnotation annotation;
        annotation.type = AnnotationType::Highlight;
        annotation.pageNumber = 0;
        annotation.opacity = 1.0;
        m_model->addAnnotation(annotation);

        QString id = m_model->getAllAnnotations().first().id;

        bool result = m_model->changeAnnotationOpacity(id, 0.5);
        QVERIFY(result);

        PDFAnnotation retrieved = m_model->getAnnotation(id);
        QCOMPARE(retrieved.opacity, 0.5);
    }

    void testAddStickyNote() {
        bool result = m_model->addStickyNote(0, QPointF(100, 100),
                                             "Sticky note content", Qt::yellow);
        QVERIFY(result);

        QList<PDFAnnotation> stickyNotes = m_model->getStickyNotesForPage(0);
        QCOMPARE(stickyNotes.size(), 1);
        QCOMPARE(stickyNotes.first().content, QString("Sticky note content"));
    }

    void testGetAnnotationCountByType() {
        PDFAnnotation highlight1, highlight2, note1;
        highlight1.type = AnnotationType::Highlight;
        highlight1.pageNumber = 0;
        highlight2.type = AnnotationType::Highlight;
        highlight2.pageNumber = 0;
        note1.type = AnnotationType::Note;
        note1.pageNumber = 0;

        m_model->addAnnotation(highlight1);
        m_model->addAnnotation(highlight2);
        m_model->addAnnotation(note1);

        QMap<AnnotationType, int> counts = m_model->getAnnotationCountByType();
        QCOMPARE(counts[AnnotationType::Highlight], 2);
        QCOMPARE(counts[AnnotationType::Note], 1);
    }

    void testGetAuthors() {
        PDFAnnotation annotation1, annotation2, annotation3;
        annotation1.type = AnnotationType::Note;
        annotation1.pageNumber = 0;
        annotation1.author = "Alice";
        annotation2.type = AnnotationType::Note;
        annotation2.pageNumber = 0;
        annotation2.author = "Bob";
        annotation3.type = AnnotationType::Note;
        annotation3.pageNumber = 0;
        annotation3.author = "Alice";

        m_model->addAnnotation(annotation1);
        m_model->addAnnotation(annotation2);
        m_model->addAnnotation(annotation3);

        QStringList authors = m_model->getAuthors();
        QVERIFY(authors.contains("Alice"));
        QVERIFY(authors.contains("Bob"));
    }

    void testPDFAnnotationStruct() {
        PDFAnnotation annotation;
        QVERIFY(!annotation.id.isEmpty());
        QCOMPARE(annotation.type, AnnotationType::Highlight);
        QCOMPARE(annotation.opacity, 1.0);
        QVERIFY(annotation.isVisible);

        QJsonObject json = annotation.toJson();
        QVERIFY(!json.isEmpty());

        PDFAnnotation loaded = PDFAnnotation::fromJson(json);
        QCOMPARE(loaded.id, annotation.id);
        QCOMPARE(loaded.type, annotation.type);
    }

    void testModelRoles() {
        PDFAnnotation annotation;
        annotation.type = AnnotationType::Note;
        annotation.pageNumber = 5;
        annotation.content = "Test content";
        annotation.author = "Test Author";
        annotation.color = Qt::blue;
        annotation.opacity = 0.8;
        m_model->addAnnotation(annotation);

        QModelIndex index = m_model->index(0, 0);

        QVERIFY(
            m_model->data(index, AnnotationModel::IdRole).toString().length() >
            0);
        QCOMPARE(m_model->data(index, AnnotationModel::PageNumberRole).toInt(),
                 5);
        QCOMPARE(m_model->data(index, AnnotationModel::ContentRole).toString(),
                 QString("Test content"));
        QCOMPARE(m_model->data(index, AnnotationModel::AuthorRole).toString(),
                 QString("Test Author"));
    }

private:
    AnnotationModel* m_model = nullptr;
};

QTEST_MAIN(TestAnnotationModel)
#include "test_annotation_model.moc"
