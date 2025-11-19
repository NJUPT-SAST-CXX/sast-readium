#pragma once

#include <QColor>
#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QUndoCommand>
#include "../model/HighlightModel.h"

// Forward declarations
class HighlightModel;
class TextSelectionManager;
struct TextSelection;

/**
 * @brief Base class for highlight-related undo/redo commands
 */
class HighlightCommand : public QUndoCommand {
public:
    explicit HighlightCommand(HighlightModel* model, const QString& text,
                              QUndoCommand* parent = nullptr);
    ~HighlightCommand() override = default;

protected:
    HighlightModel* m_model;
};

/**
 * @brief Command to add a new highlight
 */
class AddHighlightCommand : public HighlightCommand {
public:
    explicit AddHighlightCommand(HighlightModel* model,
                                 const TextHighlight& highlight,
                                 QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1001; }

private:
    TextHighlight m_highlight;
    bool m_firstTime;
};

/**
 * @brief Command to remove a highlight
 */
class RemoveHighlightCommand : public HighlightCommand {
public:
    explicit RemoveHighlightCommand(HighlightModel* model,
                                    const QString& highlightId,
                                    QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1002; }

private:
    QString m_highlightId;
    TextHighlight m_removedHighlight;
    bool m_firstTime;
};

/**
 * @brief Command to edit highlight note/comment
 */
class EditHighlightNoteCommand : public HighlightCommand {
public:
    explicit EditHighlightNoteCommand(HighlightModel* model,
                                      const QString& highlightId,
                                      const QString& newNote,
                                      QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1003; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    QString m_highlightId;
    QString m_oldNote;
    QString m_newNote;
    bool m_firstTime;
};

/**
 * @brief Command to change highlight color
 */
class ChangeHighlightColorCommand : public HighlightCommand {
public:
    explicit ChangeHighlightColorCommand(HighlightModel* model,
                                         const QString& highlightId,
                                         const QColor& newColor,
                                         QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1004; }

private:
    QString m_highlightId;
    QColor m_oldColor;
    QColor m_newColor;
    bool m_firstTime;
};

/**
 * @brief Command to change highlight opacity
 */
class ChangeHighlightOpacityCommand : public HighlightCommand {
public:
    explicit ChangeHighlightOpacityCommand(HighlightModel* model,
                                           const QString& highlightId,
                                           double newOpacity,
                                           QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1005; }

private:
    QString m_highlightId;
    double m_oldOpacity;
    double m_newOpacity;
    bool m_firstTime;
};

/**
 * @brief Command to toggle highlight visibility
 */
class ToggleHighlightVisibilityCommand : public HighlightCommand {
public:
    explicit ToggleHighlightVisibilityCommand(HighlightModel* model,
                                              const QString& highlightId,
                                              QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1006; }

private:
    QString m_highlightId;
    bool m_oldVisibility;
    bool m_newVisibility;
    bool m_firstTime;
};

/**
 * @brief Command to remove all highlights
 */
class ClearAllHighlightsCommand : public HighlightCommand {
public:
    explicit ClearAllHighlightsCommand(HighlightModel* model,
                                       QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1007; }

private:
    QList<TextHighlight> m_removedHighlights;
    bool m_firstTime;
};

/**
 * @brief Command to remove highlights for a specific page
 */
class RemovePageHighlightsCommand : public HighlightCommand {
public:
    explicit RemovePageHighlightsCommand(HighlightModel* model, int pageNumber,
                                         QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1008; }

private:
    int m_pageNumber;
    QList<TextHighlight> m_removedHighlights;
    bool m_firstTime;
};

/**
 * @brief Command to batch add multiple highlights
 */
class BatchAddHighlightsCommand : public HighlightCommand {
public:
    explicit BatchAddHighlightsCommand(HighlightModel* model,
                                       const QList<TextHighlight>& highlights,
                                       QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1009; }

private:
    QList<TextHighlight> m_highlights;
    bool m_firstTime;
};

/**
 * @brief Command to batch remove multiple highlights
 */
class BatchRemoveHighlightsCommand : public HighlightCommand {
public:
    explicit BatchRemoveHighlightsCommand(HighlightModel* model,
                                          const QStringList& highlightIds,
                                          QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1010; }

private:
    QStringList m_highlightIds;
    QList<TextHighlight> m_removedHighlights;
    bool m_firstTime;
};

/**
 * @brief Command to update highlight (comprehensive edit)
 */
class UpdateHighlightCommand : public HighlightCommand {
public:
    explicit UpdateHighlightCommand(HighlightModel* model,
                                    const QString& highlightId,
                                    const TextHighlight& newHighlight,
                                    QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    int id() const override { return 1011; }

private:
    QString m_highlightId;
    TextHighlight m_oldHighlight;
    TextHighlight m_newHighlight;
    bool m_firstTime;
};

/**
 * @brief Factory for creating highlight commands
 */
class HighlightCommandFactory {
public:
    static AddHighlightCommand* createAddCommand(
        HighlightModel* model, const TextHighlight& highlight);
    static RemoveHighlightCommand* createRemoveCommand(
        HighlightModel* model, const QString& highlightId);
    static EditHighlightNoteCommand* createEditNoteCommand(
        HighlightModel* model, const QString& highlightId,
        const QString& newNote);
    static ChangeHighlightColorCommand* createChangeColorCommand(
        HighlightModel* model, const QString& highlightId,
        const QColor& newColor);
    static ChangeHighlightOpacityCommand* createChangeOpacityCommand(
        HighlightModel* model, const QString& highlightId, double newOpacity);
    static ToggleHighlightVisibilityCommand* createToggleVisibilityCommand(
        HighlightModel* model, const QString& highlightId);
    static ClearAllHighlightsCommand* createClearAllCommand(
        HighlightModel* model);
    static RemovePageHighlightsCommand* createRemovePageCommand(
        HighlightModel* model, int pageNumber);
    static BatchAddHighlightsCommand* createBatchAddCommand(
        HighlightModel* model, const QList<TextHighlight>& highlights);
    static BatchRemoveHighlightsCommand* createBatchRemoveCommand(
        HighlightModel* model, const QStringList& highlightIds);
    static UpdateHighlightCommand* createUpdateCommand(
        HighlightModel* model, const QString& highlightId,
        const TextHighlight& newHighlight);
};

/**
 * @brief Helper class to create highlights from text selection
 */
class HighlightCreator {
public:
    static TextHighlight createFromSelection(
        const TextSelection& selection, int pageNumber,
        HighlightColor color = HighlightColor::Yellow, double opacity = 0.4);

    static TextHighlight createFromRects(
        const QList<QRectF>& rects, const QString& text, int pageNumber,
        HighlightColor color = HighlightColor::Yellow);

    static TextHighlight createWithNote(
        const TextSelection& selection, int pageNumber, const QString& note,
        HighlightColor color = HighlightColor::Yellow);
};
