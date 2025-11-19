#include "HighlightCommands.h"
#include "../interaction/TextSelectionManager.h"
#include "../logging/SimpleLogging.h"

// HighlightCommand base class
HighlightCommand::HighlightCommand(HighlightModel* model, const QString& text,
                                   QUndoCommand* parent)
    : QUndoCommand(text, parent), m_model(model) {}

// AddHighlightCommand
AddHighlightCommand::AddHighlightCommand(HighlightModel* model,
                                         const TextHighlight& highlight,
                                         QUndoCommand* parent)
    : HighlightCommand(model, QObject::tr("Add Highlight"), parent),
      m_highlight(highlight),
      m_firstTime(true) {}

void AddHighlightCommand::undo() {
    if (m_model) {
        m_model->removeHighlight(m_highlight.id);
        SLOG_INFO(QString("Undo add highlight: %1").arg(m_highlight.id));
    }
}

void AddHighlightCommand::redo() {
    if (m_model) {
        if (m_firstTime) {
            m_model->addHighlight(m_highlight);
            m_firstTime = false;
            SLOG_INFO(QString("Add highlight: %1 on page %2")
                          .arg(m_highlight.id)
                          .arg(m_highlight.pageNumber));
        } else {
            m_model->addHighlight(m_highlight);
            SLOG_INFO(QString("Redo add highlight: %1").arg(m_highlight.id));
        }
    }
}

// RemoveHighlightCommand
RemoveHighlightCommand::RemoveHighlightCommand(HighlightModel* model,
                                               const QString& highlightId,
                                               QUndoCommand* parent)
    : HighlightCommand(model, QObject::tr("Remove Highlight"), parent),
      m_highlightId(highlightId),
      m_firstTime(true) {
    if (m_model) {
        m_removedHighlight = m_model->getHighlight(highlightId);
    }
}

void RemoveHighlightCommand::undo() {
    if (m_model && !m_removedHighlight.isEmpty()) {
        m_model->addHighlight(m_removedHighlight);
        SLOG_INFO(QString("Undo remove highlight: %1").arg(m_highlightId));
    }
}

void RemoveHighlightCommand::redo() {
    if (m_model) {
        if (m_firstTime) {
            m_removedHighlight = m_model->getHighlight(m_highlightId);
            m_firstTime = false;
        }
        m_model->removeHighlight(m_highlightId);
        SLOG_INFO(QString("Remove highlight: %1").arg(m_highlightId));
    }
}

// EditHighlightNoteCommand
EditHighlightNoteCommand::EditHighlightNoteCommand(HighlightModel* model,
                                                   const QString& highlightId,
                                                   const QString& newNote,
                                                   QUndoCommand* parent)
    : HighlightCommand(model, QObject::tr("Edit Highlight Note"), parent),
      m_highlightId(highlightId),
      m_newNote(newNote),
      m_firstTime(true) {
    if (m_model) {
        TextHighlight highlight = m_model->getHighlight(highlightId);
        m_oldNote = highlight.note;
    }
}

void EditHighlightNoteCommand::undo() {
    if (m_model) {
        m_model->editHighlightNote(m_highlightId, m_oldNote);
        SLOG_INFO(
            QString("Undo edit note for highlight: %1").arg(m_highlightId));
    }
}

void EditHighlightNoteCommand::redo() {
    if (m_model) {
        if (m_firstTime) {
            TextHighlight highlight = m_model->getHighlight(m_highlightId);
            m_oldNote = highlight.note;
            m_firstTime = false;
        }
        m_model->editHighlightNote(m_highlightId, m_newNote);
        SLOG_INFO(QString("Edit note for highlight: %1").arg(m_highlightId));
    }
}

bool EditHighlightNoteCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != id()) {
        return false;
    }

    const auto* otherCmd = static_cast<const EditHighlightNoteCommand*>(other);
    if (otherCmd->m_highlightId != m_highlightId) {
        return false;
    }

    m_newNote = otherCmd->m_newNote;
    return true;
}

// ChangeHighlightColorCommand
ChangeHighlightColorCommand::ChangeHighlightColorCommand(
    HighlightModel* model, const QString& highlightId, const QColor& newColor,
    QUndoCommand* parent)
    : HighlightCommand(model, QObject::tr("Change Highlight Color"), parent),
      m_highlightId(highlightId),
      m_newColor(newColor),
      m_firstTime(true) {
    if (m_model) {
        TextHighlight highlight = m_model->getHighlight(highlightId);
        m_oldColor = highlight.color;
    }
}

void ChangeHighlightColorCommand::undo() {
    if (m_model) {
        m_model->changeHighlightColor(m_highlightId, m_oldColor);
        SLOG_INFO(
            QString("Undo color change for highlight: %1").arg(m_highlightId));
    }
}

void ChangeHighlightColorCommand::redo() {
    if (m_model) {
        if (m_firstTime) {
            TextHighlight highlight = m_model->getHighlight(m_highlightId);
            m_oldColor = highlight.color;
            m_firstTime = false;
        }
        m_model->changeHighlightColor(m_highlightId, m_newColor);
        SLOG_INFO(QString("Change color for highlight: %1").arg(m_highlightId));
    }
}

// ChangeHighlightOpacityCommand
ChangeHighlightOpacityCommand::ChangeHighlightOpacityCommand(
    HighlightModel* model, const QString& highlightId, double newOpacity,
    QUndoCommand* parent)
    : HighlightCommand(model, QObject::tr("Change Highlight Opacity"), parent),
      m_highlightId(highlightId),
      m_newOpacity(newOpacity),
      m_firstTime(true) {
    if (m_model) {
        TextHighlight highlight = m_model->getHighlight(highlightId);
        m_oldOpacity = highlight.opacity;
    }
}

void ChangeHighlightOpacityCommand::undo() {
    if (m_model) {
        m_model->changeHighlightOpacity(m_highlightId, m_oldOpacity);
        SLOG_INFO(QString("Undo opacity change for highlight: %1")
                      .arg(m_highlightId));
    }
}

void ChangeHighlightOpacityCommand::redo() {
    if (m_model) {
        if (m_firstTime) {
            TextHighlight highlight = m_model->getHighlight(m_highlightId);
            m_oldOpacity = highlight.opacity;
            m_firstTime = false;
        }
        m_model->changeHighlightOpacity(m_highlightId, m_newOpacity);
        SLOG_INFO(
            QString("Change opacity for highlight: %1").arg(m_highlightId));
    }
}

// ToggleHighlightVisibilityCommand
ToggleHighlightVisibilityCommand::ToggleHighlightVisibilityCommand(
    HighlightModel* model, const QString& highlightId, QUndoCommand* parent)
    : HighlightCommand(model, QObject::tr("Toggle Highlight Visibility"),
                       parent),
      m_highlightId(highlightId),
      m_firstTime(true) {
    if (m_model) {
        TextHighlight highlight = m_model->getHighlight(highlightId);
        m_oldVisibility = highlight.isVisible;
        m_newVisibility = !m_oldVisibility;
    }
}

void ToggleHighlightVisibilityCommand::undo() {
    if (m_model) {
        TextHighlight highlight = m_model->getHighlight(m_highlightId);
        if (highlight.isVisible != m_oldVisibility) {
            m_model->toggleHighlightVisibility(m_highlightId);
        }
        SLOG_INFO(QString("Undo visibility toggle for highlight: %1")
                      .arg(m_highlightId));
    }
}

void ToggleHighlightVisibilityCommand::redo() {
    if (m_model) {
        if (m_firstTime) {
            TextHighlight highlight = m_model->getHighlight(m_highlightId);
            m_oldVisibility = highlight.isVisible;
            m_newVisibility = !m_oldVisibility;
            m_firstTime = false;
        }
        m_model->toggleHighlightVisibility(m_highlightId);
        SLOG_INFO(
            QString("Toggle visibility for highlight: %1").arg(m_highlightId));
    }
}

// ClearAllHighlightsCommand
ClearAllHighlightsCommand::ClearAllHighlightsCommand(HighlightModel* model,
                                                     QUndoCommand* parent)
    : HighlightCommand(model, QObject::tr("Clear All Highlights"), parent),
      m_firstTime(true) {}

void ClearAllHighlightsCommand::undo() {
    if (m_model) {
        for (const auto& highlight : m_removedHighlights) {
            m_model->addHighlight(highlight);
        }
        SLOG_INFO(QString("Undo clear all highlights: restored %1 highlights")
                      .arg(m_removedHighlights.size()));
    }
}

void ClearAllHighlightsCommand::redo() {
    if (m_model) {
        if (m_firstTime) {
            m_removedHighlights = m_model->getAllHighlights();
            m_firstTime = false;
        }
        m_model->removeAllHighlights();
        SLOG_INFO(QString("Clear all highlights: removed %1 highlights")
                      .arg(m_removedHighlights.size()));
    }
}

// RemovePageHighlightsCommand
RemovePageHighlightsCommand::RemovePageHighlightsCommand(HighlightModel* model,
                                                         int pageNumber,
                                                         QUndoCommand* parent)
    : HighlightCommand(model, QObject::tr("Remove Page Highlights"), parent),
      m_pageNumber(pageNumber),
      m_firstTime(true) {}

void RemovePageHighlightsCommand::undo() {
    if (m_model) {
        for (const auto& highlight : m_removedHighlights) {
            m_model->addHighlight(highlight);
        }
        SLOG_INFO(QString("Undo remove page highlights: restored %1 highlights "
                          "on page %2")
                      .arg(m_removedHighlights.size())
                      .arg(m_pageNumber));
    }
}

void RemovePageHighlightsCommand::redo() {
    if (m_model) {
        if (m_firstTime) {
            m_removedHighlights = m_model->getHighlightsForPage(m_pageNumber);
            m_firstTime = false;
        }
        m_model->removeHighlightsForPage(m_pageNumber);
        SLOG_INFO(
            QString(
                "Remove page highlights: removed %1 highlights from page %2")
                .arg(m_removedHighlights.size())
                .arg(m_pageNumber));
    }
}

// BatchAddHighlightsCommand
BatchAddHighlightsCommand::BatchAddHighlightsCommand(
    HighlightModel* model, const QList<TextHighlight>& highlights,
    QUndoCommand* parent)
    : HighlightCommand(model, QObject::tr("Add Multiple Highlights"), parent),
      m_highlights(highlights),
      m_firstTime(true) {}

void BatchAddHighlightsCommand::undo() {
    if (m_model) {
        for (const auto& highlight : m_highlights) {
            m_model->removeHighlight(highlight.id);
        }
        SLOG_INFO(QString("Undo batch add: removed %1 highlights")
                      .arg(m_highlights.size()));
    }
}

void BatchAddHighlightsCommand::redo() {
    if (m_model) {
        for (const auto& highlight : m_highlights) {
            m_model->addHighlight(highlight);
        }
        SLOG_INFO(
            QString("Batch add: added %1 highlights").arg(m_highlights.size()));
        m_firstTime = false;
    }
}

// BatchRemoveHighlightsCommand
BatchRemoveHighlightsCommand::BatchRemoveHighlightsCommand(
    HighlightModel* model, const QStringList& highlightIds,
    QUndoCommand* parent)
    : HighlightCommand(model, QObject::tr("Remove Multiple Highlights"),
                       parent),
      m_highlightIds(highlightIds),
      m_firstTime(true) {}

void BatchRemoveHighlightsCommand::undo() {
    if (m_model) {
        for (const auto& highlight : m_removedHighlights) {
            m_model->addHighlight(highlight);
        }
        SLOG_INFO(QString("Undo batch remove: restored %1 highlights")
                      .arg(m_removedHighlights.size()));
    }
}

void BatchRemoveHighlightsCommand::redo() {
    if (m_model) {
        if (m_firstTime) {
            for (const auto& highlightId : m_highlightIds) {
                TextHighlight highlight = m_model->getHighlight(highlightId);
                if (!highlight.isEmpty()) {
                    m_removedHighlights.append(highlight);
                }
            }
            m_firstTime = false;
        }

        for (const auto& highlightId : m_highlightIds) {
            m_model->removeHighlight(highlightId);
        }
        SLOG_INFO(QString("Batch remove: removed %1 highlights")
                      .arg(m_highlightIds.size()));
    }
}

// UpdateHighlightCommand
UpdateHighlightCommand::UpdateHighlightCommand(
    HighlightModel* model, const QString& highlightId,
    const TextHighlight& newHighlight, QUndoCommand* parent)
    : HighlightCommand(model, QObject::tr("Update Highlight"), parent),
      m_highlightId(highlightId),
      m_newHighlight(newHighlight),
      m_firstTime(true) {
    if (m_model) {
        m_oldHighlight = m_model->getHighlight(highlightId);
    }
}

void UpdateHighlightCommand::undo() {
    if (m_model && !m_oldHighlight.isEmpty()) {
        m_model->updateHighlight(m_highlightId, m_oldHighlight);
        SLOG_INFO(QString("Undo update highlight: %1").arg(m_highlightId));
    }
}

void UpdateHighlightCommand::redo() {
    if (m_model) {
        if (m_firstTime) {
            m_oldHighlight = m_model->getHighlight(m_highlightId);
            m_firstTime = false;
        }
        m_model->updateHighlight(m_highlightId, m_newHighlight);
        SLOG_INFO(QString("Update highlight: %1").arg(m_highlightId));
    }
}

// HighlightCommandFactory
AddHighlightCommand* HighlightCommandFactory::createAddCommand(
    HighlightModel* model, const TextHighlight& highlight) {
    return new AddHighlightCommand(model, highlight);
}

RemoveHighlightCommand* HighlightCommandFactory::createRemoveCommand(
    HighlightModel* model, const QString& highlightId) {
    return new RemoveHighlightCommand(model, highlightId);
}

EditHighlightNoteCommand* HighlightCommandFactory::createEditNoteCommand(
    HighlightModel* model, const QString& highlightId, const QString& newNote) {
    return new EditHighlightNoteCommand(model, highlightId, newNote);
}

ChangeHighlightColorCommand* HighlightCommandFactory::createChangeColorCommand(
    HighlightModel* model, const QString& highlightId, const QColor& newColor) {
    return new ChangeHighlightColorCommand(model, highlightId, newColor);
}

ChangeHighlightOpacityCommand*
HighlightCommandFactory::createChangeOpacityCommand(HighlightModel* model,
                                                    const QString& highlightId,
                                                    double newOpacity) {
    return new ChangeHighlightOpacityCommand(model, highlightId, newOpacity);
}

ToggleHighlightVisibilityCommand*
HighlightCommandFactory::createToggleVisibilityCommand(
    HighlightModel* model, const QString& highlightId) {
    return new ToggleHighlightVisibilityCommand(model, highlightId);
}

ClearAllHighlightsCommand* HighlightCommandFactory::createClearAllCommand(
    HighlightModel* model) {
    return new ClearAllHighlightsCommand(model);
}

RemovePageHighlightsCommand* HighlightCommandFactory::createRemovePageCommand(
    HighlightModel* model, int pageNumber) {
    return new RemovePageHighlightsCommand(model, pageNumber);
}

BatchAddHighlightsCommand* HighlightCommandFactory::createBatchAddCommand(
    HighlightModel* model, const QList<TextHighlight>& highlights) {
    return new BatchAddHighlightsCommand(model, highlights);
}

BatchRemoveHighlightsCommand* HighlightCommandFactory::createBatchRemoveCommand(
    HighlightModel* model, const QStringList& highlightIds) {
    return new BatchRemoveHighlightsCommand(model, highlightIds);
}

UpdateHighlightCommand* HighlightCommandFactory::createUpdateCommand(
    HighlightModel* model, const QString& highlightId,
    const TextHighlight& newHighlight) {
    return new UpdateHighlightCommand(model, highlightId, newHighlight);
}

// HighlightCreator
TextHighlight HighlightCreator::createFromSelection(
    const TextSelection& selection, int pageNumber, HighlightColor color,
    double opacity) {
    TextHighlight highlight;
    highlight.pageNumber = pageNumber;
    highlight.rects = selection.rects;
    highlight.text = selection.text;
    highlight.startCharIndex = selection.startCharIndex;
    highlight.endCharIndex = selection.endCharIndex;
    highlight.startPoint = selection.startPoint;
    highlight.endPoint = selection.endPoint;
    highlight.colorPreset = color;
    highlight.color = TextHighlight::getColorFromPreset(color);
    highlight.color.setAlphaF(opacity);
    highlight.opacity = opacity;
    highlight.author = qgetenv("USER");  // Get system username
    if (highlight.author.isEmpty()) {
        highlight.author = qgetenv("USERNAME");  // Windows fallback
    }
    return highlight;
}

TextHighlight HighlightCreator::createFromRects(const QList<QRectF>& rects,
                                                const QString& text,
                                                int pageNumber,
                                                HighlightColor color) {
    TextHighlight highlight;
    highlight.pageNumber = pageNumber;
    highlight.rects = rects;
    highlight.text = text;
    highlight.colorPreset = color;
    highlight.color = TextHighlight::getColorFromPreset(color);
    highlight.author = qgetenv("USER");
    if (highlight.author.isEmpty()) {
        highlight.author = qgetenv("USERNAME");
    }
    return highlight;
}

TextHighlight HighlightCreator::createWithNote(const TextSelection& selection,
                                               int pageNumber,
                                               const QString& note,
                                               HighlightColor color) {
    TextHighlight highlight = createFromSelection(selection, pageNumber, color);
    highlight.note = note;
    return highlight;
}
