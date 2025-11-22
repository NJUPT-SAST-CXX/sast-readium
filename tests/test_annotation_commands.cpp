#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QUndoStack>
#include "../app/command/AnnotationCommands.h"
#include "../app/model/AnnotationModel.h"

class AnnotationCommandsTest : public ::testing::Test {
protected:
    void SetUp() override {
        model = new AnnotationModel();
        undoStack = new QUndoStack();
    }

    void TearDown() override {
        delete undoStack;
        delete model;
    }

    AnnotationModel* model;
    QUndoStack* undoStack;
};

TEST_F(AnnotationCommandsTest, AddAnnotationCommand) {
    // Create annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Highlight;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(10, 10, 100, 50);
    annotation.content = "Test highlight";

    // Execute command
    auto* cmd = new AddAnnotationCommand(model, annotation);
    undoStack->push(cmd);

    // Verify addition
    EXPECT_EQ(model->getTotalAnnotationCount(), 1);
    EXPECT_TRUE(model->getAnnotation(annotation.id).id == annotation.id);

    // Undo
    undoStack->undo();
    EXPECT_EQ(model->getTotalAnnotationCount(), 0);

    // Redo
    undoStack->redo();
    EXPECT_EQ(model->getTotalAnnotationCount(), 1);
}

TEST_F(AnnotationCommandsTest, RemoveAnnotationCommand) {
    // Add annotation first
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Note;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(20, 20, 30, 30);
    annotation.content = "Test note";
    model->addAnnotation(annotation);

    QString annotationId = annotation.id;
    EXPECT_EQ(model->getTotalAnnotationCount(), 1);

    // Execute remove command
    auto* cmd = new RemoveAnnotationCommand(model, annotationId);
    undoStack->push(cmd);

    // Verify removal
    EXPECT_EQ(model->getTotalAnnotationCount(), 0);

    // Undo - annotation should be restored
    undoStack->undo();
    EXPECT_EQ(model->getTotalAnnotationCount(), 1);
    EXPECT_EQ(model->getAnnotation(annotationId).content, "Test note");

    // Redo - remove again
    undoStack->redo();
    EXPECT_EQ(model->getTotalAnnotationCount(), 0);
}

TEST_F(AnnotationCommandsTest, UpdateAnnotationContentCommand) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::FreeText;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(50, 50, 100, 50);
    annotation.content = "Original content";
    model->addAnnotation(annotation);

    QString annotationId = annotation.id;

    // Execute update command
    auto* cmd =
        new UpdateAnnotationContentCommand(model, annotationId, "New content");
    undoStack->push(cmd);

    // Verify update
    EXPECT_EQ(model->getAnnotation(annotationId).content, "New content");

    // Undo
    undoStack->undo();
    EXPECT_EQ(model->getAnnotation(annotationId).content, "Original content");

    // Redo
    undoStack->redo();
    EXPECT_EQ(model->getAnnotation(annotationId).content, "New content");
}

TEST_F(AnnotationCommandsTest, UpdateContentCommandMerge) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::FreeText;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(50, 50, 100, 50);
    annotation.content = "Original";
    model->addAnnotation(annotation);

    QString annotationId = annotation.id;

    // Execute multiple content updates
    undoStack->push(
        new UpdateAnnotationContentCommand(model, annotationId, "A"));
    undoStack->push(
        new UpdateAnnotationContentCommand(model, annotationId, "AB"));
    undoStack->push(
        new UpdateAnnotationContentCommand(model, annotationId, "ABC"));

    // Content should be "ABC"
    EXPECT_EQ(model->getAnnotation(annotationId).content, "ABC");

    // One undo should revert to original due to command merging
    undoStack->undo();
    EXPECT_EQ(model->getAnnotation(annotationId).content, "Original");
}

TEST_F(AnnotationCommandsTest, MoveAnnotationCommand) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Note;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(10, 10, 20, 20);
    model->addAnnotation(annotation);

    QString annotationId = annotation.id;
    QPointF originalPos = annotation.boundingRect.topLeft();
    QPointF newPos(100, 100);

    // Execute move command
    auto* cmd = new MoveAnnotationCommand(model, annotationId, newPos);
    undoStack->push(cmd);

    // Verify move
    EXPECT_EQ(model->getAnnotation(annotationId).boundingRect.topLeft(),
              newPos);

    // Undo
    undoStack->undo();
    EXPECT_EQ(model->getAnnotation(annotationId).boundingRect.topLeft(),
              originalPos);

    // Redo
    undoStack->redo();
    EXPECT_EQ(model->getAnnotation(annotationId).boundingRect.topLeft(),
              newPos);
}

TEST_F(AnnotationCommandsTest, ResizeAnnotationCommand) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Rectangle;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(10, 10, 50, 50);
    model->addAnnotation(annotation);

    QString annotationId = annotation.id;
    QRectF originalRect = annotation.boundingRect;
    QRectF newRect(10, 10, 100, 100);

    // Execute resize command
    auto* cmd = new ResizeAnnotationCommand(model, annotationId, newRect);
    undoStack->push(cmd);

    // Verify resize
    EXPECT_EQ(model->getAnnotation(annotationId).boundingRect, newRect);

    // Undo
    undoStack->undo();
    EXPECT_EQ(model->getAnnotation(annotationId).boundingRect, originalRect);

    // Redo
    undoStack->redo();
    EXPECT_EQ(model->getAnnotation(annotationId).boundingRect, newRect);
}

TEST_F(AnnotationCommandsTest, ChangeColorCommand) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Highlight;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(10, 10, 100, 20);
    annotation.color = Qt::yellow;
    model->addAnnotation(annotation);

    QString annotationId = annotation.id;
    QColor originalColor = Qt::yellow;
    QColor newColor = Qt::green;

    // Execute change color command
    auto* cmd = new ChangeAnnotationColorCommand(model, annotationId, newColor);
    undoStack->push(cmd);

    // Verify color change
    EXPECT_EQ(model->getAnnotation(annotationId).color, newColor);

    // Undo
    undoStack->undo();
    EXPECT_EQ(model->getAnnotation(annotationId).color, originalColor);

    // Redo
    undoStack->redo();
    EXPECT_EQ(model->getAnnotation(annotationId).color, newColor);
}

TEST_F(AnnotationCommandsTest, ChangeOpacityCommand) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Highlight;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(10, 10, 100, 20);
    annotation.opacity = 0.5;
    model->addAnnotation(annotation);

    QString annotationId = annotation.id;
    double originalOpacity = 0.5;
    double newOpacity = 0.8;

    // Execute change opacity command
    auto* cmd =
        new ChangeAnnotationOpacityCommand(model, annotationId, newOpacity);
    undoStack->push(cmd);

    // Verify opacity change
    EXPECT_DOUBLE_EQ(model->getAnnotation(annotationId).opacity, newOpacity);

    // Undo
    undoStack->undo();
    EXPECT_DOUBLE_EQ(model->getAnnotation(annotationId).opacity,
                     originalOpacity);

    // Redo
    undoStack->redo();
    EXPECT_DOUBLE_EQ(model->getAnnotation(annotationId).opacity, newOpacity);
}

TEST_F(AnnotationCommandsTest, ToggleVisibilityCommand) {
    // Add annotation
    PDFAnnotation annotation;
    annotation.type = AnnotationType::Note;
    annotation.pageNumber = 0;
    annotation.boundingRect = QRectF(10, 10, 20, 20);
    annotation.isVisible = true;
    model->addAnnotation(annotation);

    QString annotationId = annotation.id;

    // Execute toggle visibility command
    auto* cmd = new ToggleAnnotationVisibilityCommand(model, annotationId);
    undoStack->push(cmd);

    // Verify visibility toggled
    EXPECT_FALSE(model->getAnnotation(annotationId).isVisible);

    // Undo
    undoStack->undo();
    EXPECT_TRUE(model->getAnnotation(annotationId).isVisible);

    // Redo
    undoStack->redo();
    EXPECT_FALSE(model->getAnnotation(annotationId).isVisible);
}

TEST_F(AnnotationCommandsTest, ClearAllAnnotationsCommand) {
    // Add multiple annotations
    for (int i = 0; i < 5; ++i) {
        PDFAnnotation annotation;
        annotation.type = AnnotationType::Highlight;
        annotation.pageNumber = i % 3;
        annotation.boundingRect = QRectF(10 * i, 10 * i, 50, 20);
        model->addAnnotation(annotation);
    }

    EXPECT_EQ(model->getTotalAnnotationCount(), 5);

    // Execute clear all command
    auto* cmd = new ClearAllAnnotationsCommand(model);
    undoStack->push(cmd);

    // Verify all cleared
    EXPECT_EQ(model->getTotalAnnotationCount(), 0);

    // Undo - all annotations should be restored
    undoStack->undo();
    EXPECT_EQ(model->getTotalAnnotationCount(), 5);

    // Redo - clear again
    undoStack->redo();
    EXPECT_EQ(model->getTotalAnnotationCount(), 0);
}

TEST_F(AnnotationCommandsTest, RemovePageAnnotationsCommand) {
    // Add annotations on multiple pages
    for (int page = 0; page < 3; ++page) {
        for (int i = 0; i < 2; ++i) {
            PDFAnnotation annotation;
            annotation.type = AnnotationType::Highlight;
            annotation.pageNumber = page;
            annotation.boundingRect = QRectF(10 * i, 10 * i, 50, 20);
            model->addAnnotation(annotation);
        }
    }

    EXPECT_EQ(model->getTotalAnnotationCount(), 6);

    // Execute remove page annotations command
    auto* cmd = new RemovePageAnnotationsCommand(model, 1);
    undoStack->push(cmd);

    // Verify page 1 annotations removed
    EXPECT_EQ(model->getTotalAnnotationCount(), 4);
    EXPECT_EQ(model->getAnnotationCountForPage(1), 0);
    EXPECT_EQ(model->getAnnotationCountForPage(0), 2);
    EXPECT_EQ(model->getAnnotationCountForPage(2), 2);

    // Undo - page 1 annotations should be restored
    undoStack->undo();
    EXPECT_EQ(model->getTotalAnnotationCount(), 6);
    EXPECT_EQ(model->getAnnotationCountForPage(1), 2);

    // Redo
    undoStack->redo();
    EXPECT_EQ(model->getTotalAnnotationCount(), 4);
    EXPECT_EQ(model->getAnnotationCountForPage(1), 0);
}

TEST_F(AnnotationCommandsTest, BatchAddAnnotationsCommand) {
    // Create multiple annotations
    QList<PDFAnnotation> annotations;
    for (int i = 0; i < 3; ++i) {
        PDFAnnotation annotation;
        annotation.type = AnnotationType::Highlight;
        annotation.pageNumber = i;
        annotation.boundingRect = QRectF(10 * i, 10 * i, 50, 20);
        annotations.append(annotation);
    }

    // Execute batch add command
    auto* cmd = new BatchAddAnnotationsCommand(model, annotations);
    undoStack->push(cmd);

    // Verify all added
    EXPECT_EQ(model->getTotalAnnotationCount(), 3);

    // Undo - all should be removed
    undoStack->undo();
    EXPECT_EQ(model->getTotalAnnotationCount(), 0);

    // Redo - all should be added back
    undoStack->redo();
    EXPECT_EQ(model->getTotalAnnotationCount(), 3);
}

TEST_F(AnnotationCommandsTest, BatchRemoveAnnotationsCommand) {
    // Add annotations
    QStringList annotationIds;
    for (int i = 0; i < 4; ++i) {
        PDFAnnotation annotation;
        annotation.type = AnnotationType::Note;
        annotation.pageNumber = 0;
        annotation.boundingRect = QRectF(10 * i, 10 * i, 20, 20);
        model->addAnnotation(annotation);
        annotationIds.append(annotation.id);
    }

    EXPECT_EQ(model->getTotalAnnotationCount(), 4);

    // Execute batch remove command (remove first 2)
    QStringList toRemove = {annotationIds[0], annotationIds[1]};
    auto* cmd = new BatchRemoveAnnotationsCommand(model, toRemove);
    undoStack->push(cmd);

    // Verify removal
    EXPECT_EQ(model->getTotalAnnotationCount(), 2);

    // Undo - annotations should be restored
    undoStack->undo();
    EXPECT_EQ(model->getTotalAnnotationCount(), 4);

    // Redo - remove again
    undoStack->redo();
    EXPECT_EQ(model->getTotalAnnotationCount(), 2);
}

TEST_F(AnnotationCommandsTest, MultipleUndoRedo) {
    // Perform multiple operations
    PDFAnnotation ann1;
    ann1.type = AnnotationType::Highlight;
    ann1.pageNumber = 0;
    ann1.boundingRect = QRectF(10, 10, 100, 20);
    undoStack->push(new AddAnnotationCommand(model, ann1));

    PDFAnnotation ann2;
    ann2.type = AnnotationType::Note;
    ann2.pageNumber = 1;
    ann2.boundingRect = QRectF(20, 20, 30, 30);
    undoStack->push(new AddAnnotationCommand(model, ann2));

    undoStack->push(
        new ChangeAnnotationColorCommand(model, ann1.id, Qt::green));
    undoStack->push(new MoveAnnotationCommand(model, ann2.id, QPointF(50, 50)));

    EXPECT_EQ(model->getTotalAnnotationCount(), 2);

    // Undo all
    undoStack->undo();  // Undo move
    undoStack->undo();  // Undo color change
    undoStack->undo();  // Undo add ann2
    undoStack->undo();  // Undo add ann1

    EXPECT_EQ(model->getTotalAnnotationCount(), 0);

    // Redo all
    undoStack->redo();  // Redo add ann1
    undoStack->redo();  // Redo add ann2
    undoStack->redo();  // Redo color change
    undoStack->redo();  // Redo move

    EXPECT_EQ(model->getTotalAnnotationCount(), 2);
}

// Main function for running tests
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
