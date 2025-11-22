#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTemporaryFile>
#include "../app/controller/AnnotationController.h"
#include "../app/model/AnnotationModel.h"

class AnnotationControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create controller
        controller = new AnnotationController();
        model = controller->model();

        // Create temporary file for testing
        tempFile = new QTemporaryFile();
        tempFile->open();
        tempFilePath = tempFile->fileName();
        tempFile->close();
    }

    void TearDown() override {
        delete controller;
        delete tempFile;
    }

    AnnotationController* controller;
    AnnotationModel* model;
    QTemporaryFile* tempFile;
    QString tempFilePath;
};

TEST_F(AnnotationControllerTest, InitialState) {
    EXPECT_FALSE(controller->hasDocument());
    EXPECT_EQ(controller->getTotalAnnotationCount(), 0);
    EXPECT_TRUE(controller->currentFilePath().isEmpty());
}

TEST_F(AnnotationControllerTest, AddAnnotation) {
    // Create test annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Highlight;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(10, 10, 100, 50);
    annotation.content = "Test highlight";
    annotation.color = Qt::yellow;

    // Add annotation
    bool success = controller->addAnnotation(annotation);
    EXPECT_TRUE(success);
    EXPECT_EQ(controller->getTotalAnnotationCount(), 1);

    // Verify annotation was added
    QList<PDFAnnotation> annotations = controller->getAnnotationsForPage(0);
    EXPECT_EQ(annotations.size(), 1);
    EXPECT_EQ(annotations.first().content, "Test highlight");
}

TEST_F(AnnotationControllerTest, RemoveAnnotation) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Note;
    annotation.pageNumber = 1;
    annotation.boundingRect = QRectF(20, 20, 30, 30);
    annotation.content = "Test note";

    controller->addAnnotation(annotation);
    QString annotationId = annotation.id;

    EXPECT_EQ(controller->getTotalAnnotationCount(), 1);

    // Remove annotation
    bool success = controller->removeAnnotation(annotationId);
    EXPECT_TRUE(success);
    EXPECT_EQ(controller->getTotalAnnotationCount(), 0);
}

TEST_F(AnnotationControllerTest, UpdateAnnotation) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Rectangle;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(50, 50, 100, 100);
    annotation.content = "Original content";
    annotation.color = Qt::blue;

    controller->addAnnotation(annotation);
    QString annotationId = annotation.id;

    // Update annotation
    PDFAnnotation updated = controller->getAnnotation(annotationId);
    updated.content = "Updated content";
    updated.color = Qt::red;

    bool success = controller->updateAnnotation(annotationId, updated);
    EXPECT_TRUE(success);

    // Verify update
    PDFAnnotation retrieved = controller->getAnnotation(annotationId);
    EXPECT_EQ(retrieved.content, "Updated content");
    EXPECT_EQ(retrieved.color, Qt::red);
}

TEST_F(AnnotationControllerTest, MoveAnnotation) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Note;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(10, 10, 20, 20);

    controller->addAnnotation(annotation);
    QString annotationId = annotation.id;

    // Move annotation
    QPointF newPosition(100, 100);
    bool success = controller->moveAnnotation(annotationId, newPosition);
    EXPECT_TRUE(success);

    // Verify new position
    PDFAnnotation moved = controller->getAnnotation(annotationId);
    EXPECT_EQ(moved.boundingRect.topLeft(), newPosition);
}

TEST_F(AnnotationControllerTest, ResizeAnnotation) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Rectangle;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(10, 10, 50, 50);

    controller->addAnnotation(annotation);
    QString annotationId = annotation.id;

    // Resize annotation
    QRectF newBoundary(10, 10, 100, 100);
    bool success = controller->resizeAnnotation(annotationId, newBoundary);
    EXPECT_TRUE(success);

    // Verify new size
    PDFAnnotation resized = controller->getAnnotation(annotationId);
    EXPECT_EQ(resized.boundingRect, newBoundary);
}

TEST_F(AnnotationControllerTest, ChangeColor) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Highlight;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(10, 10, 100, 20);
    annotation.color = Qt::yellow;

    controller->addAnnotation(annotation);
    QString annotationId = annotation.id;

    // Change color
    QColor newColor = Qt::green;
    bool success = controller->changeAnnotationColor(annotationId, newColor);
    EXPECT_TRUE(success);

    // Verify color change
    PDFAnnotation updated = controller->getAnnotation(annotationId);
    EXPECT_EQ(updated.color, newColor);
}

TEST_F(AnnotationControllerTest, ChangeOpacity) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Highlight;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(10, 10, 100, 20);
    annotation.opacity = 0.5;

    controller->addAnnotation(annotation);
    QString annotationId = annotation.id;

    // Change opacity
    double newOpacity = 0.8;
    bool success =
        controller->changeAnnotationOpacity(annotationId, newOpacity);
    EXPECT_TRUE(success);

    // Verify opacity change
    PDFAnnotation updated = controller->getAnnotation(annotationId);
    EXPECT_DOUBLE_EQ(updated.opacity, newOpacity);
}

TEST_F(AnnotationControllerTest, GetAnnotationsForPage) {
    // Add annotations on different pages
    for (int page = 0; page < 3; ++page) {
        for (int i = 0; i < 2; ++i) {
            PDFAnnotation annotation;
            annotation.type = AnnotationType::Highlight;
            annotation.pageNumber = page;
            annotation.boundingRect = QRectF(10 * i, 10 * i, 50, 20);
            controller->addAnnotation(annotation);
        }
    }

    // Test page 0
    QList<PDFAnnotation> page0Annotations =
        controller->getAnnotationsForPage(0);
    EXPECT_EQ(page0Annotations.size(), 2);

    // Test page 1
    QList<PDFAnnotation> page1Annotations =
        controller->getAnnotationsForPage(1);
    EXPECT_EQ(page1Annotations.size(), 2);

    // Test page 2
    QList<PDFAnnotation> page2Annotations =
        controller->getAnnotationsForPage(2);
    EXPECT_EQ(page2Annotations.size(), 2);

    // Test non-existent page
    QList<PDFAnnotation> page3Annotations =
        controller->getAnnotationsForPage(3);
    EXPECT_EQ(page3Annotations.size(), 0);
}

TEST_F(AnnotationControllerTest, SearchAnnotations) {
    // Add annotations with different content
    PDFAnnotation ann1;
    ann1.type = AnnotationType::Note;
    ann1.pageNumber = 0;
    ann1.boundingRect = QRectF(10, 10, 20, 20);
    ann1.content = "Important note about testing";
    controller->addAnnotation(ann1);

    PDFAnnotation ann2;
    ann2.type = AnnotationType::Highlight;
    ann2.pageNumber = 1;
    ann2.boundingRect = QRectF(30, 30, 50, 20);
    ann2.content = "Code review feedback";
    controller->addAnnotation(ann2);

    PDFAnnotation ann3;
    ann3.type = AnnotationType::FreeText;
    ann3.pageNumber = 2;
    ann3.boundingRect = QRectF(40, 40, 100, 50);
    ann3.content = "Testing is essential";
    controller->addAnnotation(ann3);

    // Search for "testing"
    QList<PDFAnnotation> results = controller->searchAnnotations("testing");
    EXPECT_EQ(results.size(), 2);

    // Search for "feedback"
    results = controller->searchAnnotations("feedback");
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results.first().content, "Code review feedback");

    // Search for non-existent term
    results = controller->searchAnnotations("nonexistent");
    EXPECT_EQ(results.size(), 0);
}

TEST_F(AnnotationControllerTest, GetAnnotationsByType) {
    // Add annotations of different types
    controller->addHighlight(0, QRectF(10, 10, 100, 20), "Highlight 1");
    controller->addHighlight(1, QRectF(20, 20, 100, 20), "Highlight 2");
    controller->addNote(0, QPointF(50, 50), "Note 1");
    controller->addShape(AnnotationType::Rectangle, 0,
                         QRectF(100, 100, 50, 50));

    // Get highlights
    QList<PDFAnnotation> highlights =
        controller->getAnnotationsByType(AnnotationType::Highlight);
    EXPECT_EQ(highlights.size(), 2);

    // Get notes
    QList<PDFAnnotation> notes =
        controller->getAnnotationsByType(AnnotationType::Note);
    EXPECT_EQ(notes.size(), 1);

    // Get rectangles
    QList<PDFAnnotation> rectangles =
        controller->getAnnotationsByType(AnnotationType::Rectangle);
    EXPECT_EQ(rectangles.size(), 1);
}

TEST_F(AnnotationControllerTest, ClearAllAnnotations) {
    // Add multiple annotations
    for (int i = 0; i < 5; ++i) {
        PDFAnnotation annotation;
        annotation.type = AnnotationType::Highlight;
        annotation.pageNumber = i % 3;
        annotation.boundingRect = QRectF(10 * i, 10 * i, 50, 20);
        controller->addAnnotation(annotation);
    }

    EXPECT_EQ(controller->getTotalAnnotationCount(), 5);

    // Clear all
    bool success = controller->clearAllAnnotations();
    EXPECT_TRUE(success);
    EXPECT_EQ(controller->getTotalAnnotationCount(), 0);
}

TEST_F(AnnotationControllerTest, RemoveAnnotationsForPage) {
    // Add annotations on multiple pages
    for (int page = 0; page < 3; ++page) {
        for (int i = 0; i < 3; ++i) {
            PDFAnnotation annotation;
            annotation.type = AnnotationType::Highlight;
            annotation.pageNumber = page;
            annotation.boundingRect = QRectF(10 * i, 10 * i, 50, 20);
            controller->addAnnotation(annotation);
        }
    }

    EXPECT_EQ(controller->getTotalAnnotationCount(), 9);

    // Remove annotations for page 1
    bool success = controller->removeAnnotationsForPage(1);
    EXPECT_TRUE(success);
    EXPECT_EQ(controller->getTotalAnnotationCount(), 6);
    EXPECT_EQ(controller->getAnnotationCountForPage(1), 0);
    EXPECT_EQ(controller->getAnnotationCountForPage(0), 3);
    EXPECT_EQ(controller->getAnnotationCountForPage(2), 3);
}

TEST_F(AnnotationControllerTest, QuickAnnotationCreation) {
    // Test addHighlight
    bool success = controller->addHighlight(0, QRectF(10, 10, 100, 20),
                                            "Test highlight", Qt::yellow);
    EXPECT_TRUE(success);

    // Test addNote
    success = controller->addNote(1, QPointF(50, 50), "Test note", Qt::pink);
    EXPECT_TRUE(success);

    // Test addShape
    success = controller->addShape(AnnotationType::Rectangle, 2,
                                   QRectF(100, 100, 50, 50), Qt::blue);
    EXPECT_TRUE(success);

    EXPECT_EQ(controller->getTotalAnnotationCount(), 3);
}

TEST_F(AnnotationControllerTest, DefaultAuthor) {
    controller->setDefaultAuthor("Test User");
    EXPECT_EQ(controller->defaultAuthor(), "Test User");

    // Add annotation without author
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Note;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(10, 10, 20, 20);
    annotation.author = "";  // Empty author

    controller->addAnnotation(annotation);

    // Verify default author was set
    QList<PDFAnnotation> annotations = controller->getAnnotationsForPage(0);
    EXPECT_EQ(annotations.size(), 1);
    EXPECT_EQ(annotations.first().author, "Test User");
}

// Main function for running tests
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
