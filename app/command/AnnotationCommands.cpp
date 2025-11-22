#include "AnnotationCommands.h"
#include "../logging/SimpleLogging.h"

// AnnotationCommand base class implementation
AnnotationCommand::AnnotationCommand(AnnotationModel* model,
                                     const QString& text, QUndoCommand* parent)
    : QUndoCommand(text, parent), m_model(model) {
    if (!m_model) {
        SLOG_ERROR("AnnotationCommand created with null model");
    }
}

// AddAnnotationCommand implementation
AddAnnotationCommand::AddAnnotationCommand(AnnotationModel* model,
                                           const PDFAnnotation& annotation,
                                           QUndoCommand* parent)
    : AnnotationCommand(model, QObject::tr("Add Annotation"), parent),
      m_annotation(annotation),
      m_firstTime(true) {}

void AddAnnotationCommand::undo() {
    if (m_model) {
        m_model->removeAnnotation(m_annotation.id);
        SLOG_DEBUG_F("Undo: Removed annotation {}", m_annotation.id);
    }
}

void AddAnnotationCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        // Skip on first redo as it's handled by command execution
        return;
    }

    m_model->addAnnotation(m_annotation);
    SLOG_DEBUG_F("Redo: Added annotation {}", m_annotation.id);
}

// RemoveAnnotationCommand implementation
RemoveAnnotationCommand::RemoveAnnotationCommand(AnnotationModel* model,
                                                 const QString& annotationId,
                                                 QUndoCommand* parent)
    : AnnotationCommand(model, QObject::tr("Remove Annotation"), parent),
      m_annotationId(annotationId),
      m_firstTime(true) {
    // Store the annotation before removing it
    if (m_model) {
        m_removedAnnotation = m_model->getAnnotation(annotationId);
    }
}

void RemoveAnnotationCommand::undo() {
    if (m_model && !m_removedAnnotation.id.isEmpty()) {
        m_model->addAnnotation(m_removedAnnotation);
        SLOG_DEBUG_F("Undo: Restored annotation {}", m_annotationId);
    }
}

void RemoveAnnotationCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        // Skip on first redo as it's handled by command execution
        return;
    }

    m_model->removeAnnotation(m_annotationId);
    SLOG_DEBUG_F("Redo: Removed annotation {}", m_annotationId);
}

// UpdateAnnotationContentCommand implementation
UpdateAnnotationContentCommand::UpdateAnnotationContentCommand(
    AnnotationModel* model, const QString& annotationId,
    const QString& newContent, QUndoCommand* parent)
    : AnnotationCommand(model, QObject::tr("Edit Annotation Content"), parent),
      m_annotationId(annotationId),
      m_newContent(newContent),
      m_firstTime(true) {
    if (m_model) {
        PDFAnnotation annotation = m_model->getAnnotation(annotationId);
        m_oldContent = annotation.content;
    }
}

void UpdateAnnotationContentCommand::undo() {
    if (m_model) {
        m_model->editAnnotationContent(m_annotationId, m_oldContent);
        SLOG_DEBUG_F("Undo: Reverted annotation {} content", m_annotationId);
    }
}

void UpdateAnnotationContentCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        return;
    }

    m_model->editAnnotationContent(m_annotationId, m_newContent);
    SLOG_DEBUG_F("Redo: Updated annotation {} content", m_annotationId);
}

bool UpdateAnnotationContentCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != id()) {
        return false;
    }

    const auto* otherCmd =
        static_cast<const UpdateAnnotationContentCommand*>(other);
    if (otherCmd->m_annotationId != m_annotationId) {
        return false;
    }

    m_newContent = otherCmd->m_newContent;
    return true;
}

// MoveAnnotationCommand implementation
MoveAnnotationCommand::MoveAnnotationCommand(AnnotationModel* model,
                                             const QString& annotationId,
                                             const QPointF& newPosition,
                                             QUndoCommand* parent)
    : AnnotationCommand(model, QObject::tr("Move Annotation"), parent),
      m_annotationId(annotationId),
      m_newPosition(newPosition),
      m_firstTime(true) {
    if (m_model) {
        PDFAnnotation annotation = m_model->getAnnotation(annotationId);
        m_oldPosition = annotation.boundingRect.topLeft();
    }
}

void MoveAnnotationCommand::undo() {
    if (m_model) {
        m_model->moveAnnotation(m_annotationId, m_oldPosition);
        SLOG_DEBUG_F("Undo: Moved annotation {} back", m_annotationId);
    }
}

void MoveAnnotationCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        return;
    }

    m_model->moveAnnotation(m_annotationId, m_newPosition);
    SLOG_DEBUG_F("Redo: Moved annotation {} to new position", m_annotationId);
}

// ResizeAnnotationCommand implementation
ResizeAnnotationCommand::ResizeAnnotationCommand(AnnotationModel* model,
                                                 const QString& annotationId,
                                                 const QRectF& newBoundary,
                                                 QUndoCommand* parent)
    : AnnotationCommand(model, QObject::tr("Resize Annotation"), parent),
      m_annotationId(annotationId),
      m_newBoundary(newBoundary),
      m_firstTime(true) {
    if (m_model) {
        PDFAnnotation annotation = m_model->getAnnotation(annotationId);
        m_oldBoundary = annotation.boundingRect;
    }
}

void ResizeAnnotationCommand::undo() {
    if (m_model) {
        m_model->resizeAnnotation(m_annotationId, m_oldBoundary);
        SLOG_DEBUG_F("Undo: Resized annotation {} back", m_annotationId);
    }
}

void ResizeAnnotationCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        return;
    }

    m_model->resizeAnnotation(m_annotationId, m_newBoundary);
    SLOG_DEBUG_F("Redo: Resized annotation {}", m_annotationId);
}

// ChangeAnnotationColorCommand implementation
ChangeAnnotationColorCommand::ChangeAnnotationColorCommand(
    AnnotationModel* model, const QString& annotationId, const QColor& newColor,
    QUndoCommand* parent)
    : AnnotationCommand(model, QObject::tr("Change Annotation Color"), parent),
      m_annotationId(annotationId),
      m_newColor(newColor),
      m_firstTime(true) {
    if (m_model) {
        PDFAnnotation annotation = m_model->getAnnotation(annotationId);
        m_oldColor = annotation.color;
    }
}

void ChangeAnnotationColorCommand::undo() {
    if (m_model) {
        m_model->changeAnnotationColor(m_annotationId, m_oldColor);
        SLOG_DEBUG_F("Undo: Changed annotation {} color back", m_annotationId);
    }
}

void ChangeAnnotationColorCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        return;
    }

    m_model->changeAnnotationColor(m_annotationId, m_newColor);
    SLOG_DEBUG_F("Redo: Changed annotation {} color", m_annotationId);
}

// ChangeAnnotationOpacityCommand implementation
ChangeAnnotationOpacityCommand::ChangeAnnotationOpacityCommand(
    AnnotationModel* model, const QString& annotationId, double newOpacity,
    QUndoCommand* parent)
    : AnnotationCommand(model, QObject::tr("Change Annotation Opacity"),
                        parent),
      m_annotationId(annotationId),
      m_newOpacity(newOpacity),
      m_firstTime(true) {
    if (m_model) {
        PDFAnnotation annotation = m_model->getAnnotation(annotationId);
        m_oldOpacity = annotation.opacity;
    }
}

void ChangeAnnotationOpacityCommand::undo() {
    if (m_model) {
        m_model->changeAnnotationOpacity(m_annotationId, m_oldOpacity);
        SLOG_DEBUG_F("Undo: Changed annotation {} opacity back",
                     m_annotationId);
    }
}

void ChangeAnnotationOpacityCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        return;
    }

    m_model->changeAnnotationOpacity(m_annotationId, m_newOpacity);
    SLOG_DEBUG_F("Redo: Changed annotation {} opacity", m_annotationId);
}

// ToggleAnnotationVisibilityCommand implementation
ToggleAnnotationVisibilityCommand::ToggleAnnotationVisibilityCommand(
    AnnotationModel* model, const QString& annotationId, QUndoCommand* parent)
    : AnnotationCommand(model, QObject::tr("Toggle Annotation Visibility"),
                        parent),
      m_annotationId(annotationId),
      m_firstTime(true) {
    if (m_model) {
        PDFAnnotation annotation = m_model->getAnnotation(annotationId);
        m_oldVisibility = annotation.isVisible;
        m_newVisibility = !m_oldVisibility;
    }
}

void ToggleAnnotationVisibilityCommand::undo() {
    if (m_model) {
        PDFAnnotation annotation = m_model->getAnnotation(m_annotationId);
        annotation.isVisible = m_oldVisibility;
        m_model->updateAnnotation(m_annotationId, annotation);
        SLOG_DEBUG_F("Undo: Toggled annotation {} visibility", m_annotationId);
    }
}

void ToggleAnnotationVisibilityCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        return;
    }

    PDFAnnotation annotation = m_model->getAnnotation(m_annotationId);
    annotation.isVisible = m_newVisibility;
    m_model->updateAnnotation(m_annotationId, annotation);
    SLOG_DEBUG_F("Redo: Toggled annotation {} visibility", m_annotationId);
}

// UpdateAnnotationCommand implementation
UpdateAnnotationCommand::UpdateAnnotationCommand(
    AnnotationModel* model, const QString& annotationId,
    const PDFAnnotation& newAnnotation, QUndoCommand* parent)
    : AnnotationCommand(model, QObject::tr("Update Annotation"), parent),
      m_annotationId(annotationId),
      m_newAnnotation(newAnnotation),
      m_firstTime(true) {
    if (m_model) {
        m_oldAnnotation = m_model->getAnnotation(annotationId);
    }
}

void UpdateAnnotationCommand::undo() {
    if (m_model && !m_oldAnnotation.id.isEmpty()) {
        m_model->updateAnnotation(m_annotationId, m_oldAnnotation);
        SLOG_DEBUG_F("Undo: Reverted annotation {} update", m_annotationId);
    }
}

void UpdateAnnotationCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        return;
    }

    m_model->updateAnnotation(m_annotationId, m_newAnnotation);
    SLOG_DEBUG_F("Redo: Updated annotation {}", m_annotationId);
}

// ClearAllAnnotationsCommand implementation
ClearAllAnnotationsCommand::ClearAllAnnotationsCommand(AnnotationModel* model,
                                                       QUndoCommand* parent)
    : AnnotationCommand(model, QObject::tr("Clear All Annotations"), parent),
      m_firstTime(true) {
    if (m_model) {
        m_removedAnnotations = m_model->getAllAnnotations();
    }
}

void ClearAllAnnotationsCommand::undo() {
    if (m_model) {
        for (const PDFAnnotation& annotation : m_removedAnnotations) {
            m_model->addAnnotation(annotation);
        }
        SLOG_DEBUG_F("Undo: Restored {} annotations",
                     m_removedAnnotations.size());
    }
}

void ClearAllAnnotationsCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        return;
    }

    m_model->clearAnnotations();
    SLOG_DEBUG("Redo: Cleared all annotations");
}

// RemovePageAnnotationsCommand implementation
RemovePageAnnotationsCommand::RemovePageAnnotationsCommand(
    AnnotationModel* model, int pageNumber, QUndoCommand* parent)
    : AnnotationCommand(
          model,
          QObject::tr("Remove Page Annotations (Page %1)").arg(pageNumber + 1),
          parent),
      m_pageNumber(pageNumber),
      m_firstTime(true) {
    if (m_model) {
        m_removedAnnotations = m_model->getAnnotationsForPage(pageNumber);
    }
}

void RemovePageAnnotationsCommand::undo() {
    if (m_model) {
        for (const PDFAnnotation& annotation : m_removedAnnotations) {
            m_model->addAnnotation(annotation);
        }
        SLOG_DEBUG_F("Undo: Restored {} annotations for page {}",
                     m_removedAnnotations.size(), m_pageNumber);
    }
}

void RemovePageAnnotationsCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        return;
    }

    m_model->removeAnnotationsForPage(m_pageNumber);
    SLOG_DEBUG_F("Redo: Removed annotations for page {}", m_pageNumber);
}

// BatchAddAnnotationsCommand implementation
BatchAddAnnotationsCommand::BatchAddAnnotationsCommand(
    AnnotationModel* model, const QList<PDFAnnotation>& annotations,
    QUndoCommand* parent)
    : AnnotationCommand(
          model, QObject::tr("Add %1 Annotations").arg(annotations.size()),
          parent),
      m_annotations(annotations),
      m_firstTime(true) {}

void BatchAddAnnotationsCommand::undo() {
    if (m_model) {
        for (const PDFAnnotation& annotation : m_annotations) {
            m_model->removeAnnotation(annotation.id);
        }
        SLOG_DEBUG_F("Undo: Removed {} annotations", m_annotations.size());
    }
}

void BatchAddAnnotationsCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        return;
    }

    for (const PDFAnnotation& annotation : m_annotations) {
        m_model->addAnnotation(annotation);
    }
    SLOG_DEBUG_F("Redo: Added {} annotations", m_annotations.size());
}

// BatchRemoveAnnotationsCommand implementation
BatchRemoveAnnotationsCommand::BatchRemoveAnnotationsCommand(
    AnnotationModel* model, const QStringList& annotationIds,
    QUndoCommand* parent)
    : AnnotationCommand(
          model, QObject::tr("Remove %1 Annotations").arg(annotationIds.size()),
          parent),
      m_annotationIds(annotationIds),
      m_firstTime(true) {
    if (m_model) {
        for (const QString& id : annotationIds) {
            PDFAnnotation annotation = m_model->getAnnotation(id);
            if (!annotation.id.isEmpty()) {
                m_removedAnnotations.append(annotation);
            }
        }
    }
}

void BatchRemoveAnnotationsCommand::undo() {
    if (m_model) {
        for (const PDFAnnotation& annotation : m_removedAnnotations) {
            m_model->addAnnotation(annotation);
        }
        SLOG_DEBUG_F("Undo: Restored {} annotations",
                     m_removedAnnotations.size());
    }
}

void BatchRemoveAnnotationsCommand::redo() {
    if (!m_model) {
        return;
    }

    if (m_firstTime) {
        m_firstTime = false;
        return;
    }

    for (const QString& id : m_annotationIds) {
        m_model->removeAnnotation(id);
    }
    SLOG_DEBUG_F("Redo: Removed {} annotations", m_annotationIds.size());
}

// AnnotationCommandFactory implementation
AddAnnotationCommand* AnnotationCommandFactory::createAddCommand(
    AnnotationModel* model, const PDFAnnotation& annotation) {
    return new AddAnnotationCommand(model, annotation);
}

RemoveAnnotationCommand* AnnotationCommandFactory::createRemoveCommand(
    AnnotationModel* model, const QString& annotationId) {
    return new RemoveAnnotationCommand(model, annotationId);
}

UpdateAnnotationContentCommand*
AnnotationCommandFactory::createUpdateContentCommand(
    AnnotationModel* model, const QString& annotationId,
    const QString& newContent) {
    return new UpdateAnnotationContentCommand(model, annotationId, newContent);
}

MoveAnnotationCommand* AnnotationCommandFactory::createMoveCommand(
    AnnotationModel* model, const QString& annotationId,
    const QPointF& newPosition) {
    return new MoveAnnotationCommand(model, annotationId, newPosition);
}

ResizeAnnotationCommand* AnnotationCommandFactory::createResizeCommand(
    AnnotationModel* model, const QString& annotationId,
    const QRectF& newBoundary) {
    return new ResizeAnnotationCommand(model, annotationId, newBoundary);
}

ChangeAnnotationColorCommand*
AnnotationCommandFactory::createChangeColorCommand(AnnotationModel* model,
                                                   const QString& annotationId,
                                                   const QColor& newColor) {
    return new ChangeAnnotationColorCommand(model, annotationId, newColor);
}

ChangeAnnotationOpacityCommand*
AnnotationCommandFactory::createChangeOpacityCommand(
    AnnotationModel* model, const QString& annotationId, double newOpacity) {
    return new ChangeAnnotationOpacityCommand(model, annotationId, newOpacity);
}

ToggleAnnotationVisibilityCommand*
AnnotationCommandFactory::createToggleVisibilityCommand(
    AnnotationModel* model, const QString& annotationId) {
    return new ToggleAnnotationVisibilityCommand(model, annotationId);
}

UpdateAnnotationCommand* AnnotationCommandFactory::createUpdateCommand(
    AnnotationModel* model, const QString& annotationId,
    const PDFAnnotation& newAnnotation) {
    return new UpdateAnnotationCommand(model, annotationId, newAnnotation);
}

ClearAllAnnotationsCommand* AnnotationCommandFactory::createClearAllCommand(
    AnnotationModel* model) {
    return new ClearAllAnnotationsCommand(model);
}

RemovePageAnnotationsCommand* AnnotationCommandFactory::createRemovePageCommand(
    AnnotationModel* model, int pageNumber) {
    return new RemovePageAnnotationsCommand(model, pageNumber);
}

BatchAddAnnotationsCommand* AnnotationCommandFactory::createBatchAddCommand(
    AnnotationModel* model, const QList<PDFAnnotation>& annotations) {
    return new BatchAddAnnotationsCommand(model, annotations);
}

BatchRemoveAnnotationsCommand*
AnnotationCommandFactory::createBatchRemoveCommand(
    AnnotationModel* model, const QStringList& annotationIds) {
    return new BatchRemoveAnnotationsCommand(model, annotationIds);
}
