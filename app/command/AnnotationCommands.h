#pragma once

#include <QColor>
#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QUndoCommand>
#include "../model/AnnotationModel.h"

// Forward declarations
class AnnotationModel;
class AnnotationController;

/**
 * @brief Base class for annotation-related undo/redo commands
 *
 * Provides common functionality for annotation operations following
 * the Command pattern with full undo/redo support.
 */
class AnnotationCommand : public QUndoCommand {
public:
    explicit AnnotationCommand(AnnotationModel* model, const QString& text,
                               QUndoCommand* parent = nullptr);
    ~AnnotationCommand() override = default;

protected:
    AnnotationModel* m_model;
};

/**
 * @brief Command to add a new annotation
 */
class AddAnnotationCommand : public AnnotationCommand {
public:
    explicit AddAnnotationCommand(AnnotationModel* model,
                                  const PDFAnnotation& annotation,
                                  QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2001; }

private:
    PDFAnnotation m_annotation;
    bool m_firstTime;
};

/**
 * @brief Command to remove an annotation
 */
class RemoveAnnotationCommand : public AnnotationCommand {
public:
    explicit RemoveAnnotationCommand(AnnotationModel* model,
                                     const QString& annotationId,
                                     QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2002; }

private:
    QString m_annotationId;
    PDFAnnotation m_removedAnnotation;
    bool m_firstTime;
};

/**
 * @brief Command to update annotation content
 */
class UpdateAnnotationContentCommand : public AnnotationCommand {
public:
    explicit UpdateAnnotationContentCommand(AnnotationModel* model,
                                            const QString& annotationId,
                                            const QString& newContent,
                                            QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2003; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    QString m_annotationId;
    QString m_oldContent;
    QString m_newContent;
    bool m_firstTime;
};

/**
 * @brief Command to move an annotation
 */
class MoveAnnotationCommand : public AnnotationCommand {
public:
    explicit MoveAnnotationCommand(AnnotationModel* model,
                                   const QString& annotationId,
                                   const QPointF& newPosition,
                                   QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2004; }

private:
    QString m_annotationId;
    QPointF m_oldPosition;
    QPointF m_newPosition;
    bool m_firstTime;
};

/**
 * @brief Command to resize an annotation
 */
class ResizeAnnotationCommand : public AnnotationCommand {
public:
    explicit ResizeAnnotationCommand(AnnotationModel* model,
                                     const QString& annotationId,
                                     const QRectF& newBoundary,
                                     QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2005; }

private:
    QString m_annotationId;
    QRectF m_oldBoundary;
    QRectF m_newBoundary;
    bool m_firstTime;
};

/**
 * @brief Command to change annotation color
 */
class ChangeAnnotationColorCommand : public AnnotationCommand {
public:
    explicit ChangeAnnotationColorCommand(AnnotationModel* model,
                                          const QString& annotationId,
                                          const QColor& newColor,
                                          QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2006; }

private:
    QString m_annotationId;
    QColor m_oldColor;
    QColor m_newColor;
    bool m_firstTime;
};

/**
 * @brief Command to change annotation opacity
 */
class ChangeAnnotationOpacityCommand : public AnnotationCommand {
public:
    explicit ChangeAnnotationOpacityCommand(AnnotationModel* model,
                                            const QString& annotationId,
                                            double newOpacity,
                                            QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2007; }

private:
    QString m_annotationId;
    double m_oldOpacity;
    double m_newOpacity;
    bool m_firstTime;
};

/**
 * @brief Command to toggle annotation visibility
 */
class ToggleAnnotationVisibilityCommand : public AnnotationCommand {
public:
    explicit ToggleAnnotationVisibilityCommand(AnnotationModel* model,
                                               const QString& annotationId,
                                               QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2008; }

private:
    QString m_annotationId;
    bool m_oldVisibility;
    bool m_newVisibility;
    bool m_firstTime;
};

/**
 * @brief Command to update entire annotation
 */
class UpdateAnnotationCommand : public AnnotationCommand {
public:
    explicit UpdateAnnotationCommand(AnnotationModel* model,
                                     const QString& annotationId,
                                     const PDFAnnotation& newAnnotation,
                                     QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2009; }

private:
    QString m_annotationId;
    PDFAnnotation m_oldAnnotation;
    PDFAnnotation m_newAnnotation;
    bool m_firstTime;
};

/**
 * @brief Command to clear all annotations
 */
class ClearAllAnnotationsCommand : public AnnotationCommand {
public:
    explicit ClearAllAnnotationsCommand(AnnotationModel* model,
                                        QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2010; }

private:
    QList<PDFAnnotation> m_removedAnnotations;
    bool m_firstTime;
};

/**
 * @brief Command to remove all annotations from a specific page
 */
class RemovePageAnnotationsCommand : public AnnotationCommand {
public:
    explicit RemovePageAnnotationsCommand(AnnotationModel* model,
                                          int pageNumber,
                                          QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2011; }

private:
    int m_pageNumber;
    QList<PDFAnnotation> m_removedAnnotations;
    bool m_firstTime;
};

/**
 * @brief Command to batch add multiple annotations
 */
class BatchAddAnnotationsCommand : public AnnotationCommand {
public:
    explicit BatchAddAnnotationsCommand(AnnotationModel* model,
                                        const QList<PDFAnnotation>& annotations,
                                        QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2012; }

private:
    QList<PDFAnnotation> m_annotations;
    bool m_firstTime;
};

/**
 * @brief Command to batch remove multiple annotations
 */
class BatchRemoveAnnotationsCommand : public AnnotationCommand {
public:
    explicit BatchRemoveAnnotationsCommand(AnnotationModel* model,
                                           const QStringList& annotationIds,
                                           QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 2013; }

private:
    QStringList m_annotationIds;
    QList<PDFAnnotation> m_removedAnnotations;
    bool m_firstTime;
};

/**
 * @brief Factory for creating annotation commands
 */
class AnnotationCommandFactory {
public:
    static AddAnnotationCommand* createAddCommand(
        AnnotationModel* model, const PDFAnnotation& annotation);

    static RemoveAnnotationCommand* createRemoveCommand(
        AnnotationModel* model, const QString& annotationId);

    static UpdateAnnotationContentCommand* createUpdateContentCommand(
        AnnotationModel* model, const QString& annotationId,
        const QString& newContent);

    static MoveAnnotationCommand* createMoveCommand(AnnotationModel* model,
                                                    const QString& annotationId,
                                                    const QPointF& newPosition);

    static ResizeAnnotationCommand* createResizeCommand(
        AnnotationModel* model, const QString& annotationId,
        const QRectF& newBoundary);

    static ChangeAnnotationColorCommand* createChangeColorCommand(
        AnnotationModel* model, const QString& annotationId,
        const QColor& newColor);

    static ChangeAnnotationOpacityCommand* createChangeOpacityCommand(
        AnnotationModel* model, const QString& annotationId, double newOpacity);

    static ToggleAnnotationVisibilityCommand* createToggleVisibilityCommand(
        AnnotationModel* model, const QString& annotationId);

    static UpdateAnnotationCommand* createUpdateCommand(
        AnnotationModel* model, const QString& annotationId,
        const PDFAnnotation& newAnnotation);

    static ClearAllAnnotationsCommand* createClearAllCommand(
        AnnotationModel* model);

    static RemovePageAnnotationsCommand* createRemovePageCommand(
        AnnotationModel* model, int pageNumber);

    static BatchAddAnnotationsCommand* createBatchAddCommand(
        AnnotationModel* model, const QList<PDFAnnotation>& annotations);

    static BatchRemoveAnnotationsCommand* createBatchRemoveCommand(
        AnnotationModel* model, const QStringList& annotationIds);
};
