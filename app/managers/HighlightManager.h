#pragma once

#include <QObject>
#include <QUndoStack>
#include <memory>
#include "../command/HighlightCommands.h"
#include "../model/HighlightModel.h"

// Forward declarations
class TextSelectionManager;
class DocumentController;
namespace Poppler {
class Document;
}

/**
 * @brief Central manager for all highlight-related operations
 *
 * This class coordinates highlight creation, editing, deletion, and
 * persistence. It integrates with the command pattern for undo/redo support and
 * manages the highlight model and cache.
 */
class HighlightManager : public QObject {
    Q_OBJECT

public:
    explicit HighlightManager(QObject* parent = nullptr);
    ~HighlightManager() override;

    // Singleton access
    static HighlightManager& instance();

    // Initialization
    void setDocument(Poppler::Document* document, const QString& documentPath);
    void setUndoStack(QUndoStack* undoStack);
    void setTextSelectionManager(TextSelectionManager* selectionManager);

    // Model access
    HighlightModel* model() const { return m_model.get(); }
    QUndoStack* undoStack() const { return m_undoStack; }

    // Highlight creation
    bool addHighlight(const TextHighlight& highlight);
    bool addHighlightFromSelection(
        const TextSelection& selection, int pageNumber,
        HighlightColor color = HighlightColor::Yellow, double opacity = 0.4);
    bool addHighlightWithNote(const TextSelection& selection, int pageNumber,
                              const QString& note,
                              HighlightColor color = HighlightColor::Yellow);

    // Highlight editing
    bool removeHighlight(const QString& highlightId);
    bool editHighlightNote(const QString& highlightId, const QString& newNote);
    bool changeHighlightColor(const QString& highlightId,
                              const QColor& newColor);
    bool changeHighlightOpacity(const QString& highlightId, double opacity);
    bool toggleHighlightVisibility(const QString& highlightId);
    bool updateHighlight(const QString& highlightId,
                         const TextHighlight& newHighlight);

    // Batch operations
    bool addMultipleHighlights(const QList<TextHighlight>& highlights);
    bool removeMultipleHighlights(const QStringList& highlightIds);
    bool removeAllHighlights();
    bool removePageHighlights(int pageNumber);

    // Query operations
    TextHighlight getHighlight(const QString& highlightId) const;
    QList<TextHighlight> getAllHighlights() const;
    QList<TextHighlight> getHighlightsForPage(int pageNumber) const;
    QList<TextHighlight> searchHighlights(const QString& query) const;
    TextHighlight findHighlightAtPoint(int pageNumber,
                                       const QPointF& point) const;

    // Persistence
    bool saveHighlights(const QString& filePath = QString());
    bool loadHighlights(const QString& filePath = QString());
    bool autoSaveEnabled() const { return m_autoSaveEnabled; }
    void setAutoSaveEnabled(bool enabled);
    QString getHighlightFilePath() const;

    // Export
    QString exportToMarkdown() const;
    QString exportToPlainText() const;
    QJsonArray exportToJson() const;
    bool exportToFile(const QString& filePath, const QString& format = "json");

    // Statistics
    int getTotalHighlightCount() const;
    int getHighlightCountForPage(int pageNumber) const;
    HighlightStatistics getStatistics() const;
    QMap<HighlightColor, int> getColorDistribution() const;

    // Settings
    void setDefaultColor(HighlightColor color) { m_defaultColor = color; }
    HighlightColor getDefaultColor() const { return m_defaultColor; }
    void setDefaultOpacity(double opacity) { m_defaultOpacity = opacity; }
    double getDefaultOpacity() const { return m_defaultOpacity; }
    void setDefaultAuthor(const QString& author) { m_defaultAuthor = author; }
    QString getDefaultAuthor() const { return m_defaultAuthor; }

signals:
    void highlightAdded(const TextHighlight& highlight);
    void highlightRemoved(const QString& highlightId);
    void highlightUpdated(const TextHighlight& highlight);
    void highlightsLoaded(int count);
    void highlightsSaved(int count);
    void highlightsCleared();
    void highlightVisibilityChanged(const QString& highlightId, bool visible);
    void autoSaveCompleted();
    void autoSaveFailed(const QString& error);

public slots:
    void onSelectionChanged();
    void onDocumentClosed();
    void performAutoSave();

private slots:
    void onModelHighlightAdded(const TextHighlight& highlight);
    void onModelHighlightRemoved(const QString& highlightId);
    void onModelHighlightUpdated(const TextHighlight& highlight);

private:
    void connectModelSignals();
    void disconnectModelSignals();
    QString generateHighlightFilePath() const;
    void scheduleAutoSave();

    std::unique_ptr<HighlightModel> m_model;
    QUndoStack* m_undoStack;
    TextSelectionManager* m_selectionManager;
    Poppler::Document* m_document;
    QString m_documentPath;

    // Settings
    bool m_autoSaveEnabled;
    int m_autoSaveInterval;  // in milliseconds
    HighlightColor m_defaultColor;
    double m_defaultOpacity;
    QString m_defaultAuthor;

    // Auto-save timer
    QTimer* m_autoSaveTimer;
    bool m_hasUnsavedChanges;
};

/**
 * @brief Helper class for highlight rendering
 */
class HighlightRenderer {
public:
    static void renderHighlight(QPainter& painter,
                                const TextHighlight& highlight,
                                double scaleFactor = 1.0);
    static void renderHighlights(QPainter& painter,
                                 const QList<TextHighlight>& highlights,
                                 double scaleFactor = 1.0);
    static void renderHighlightBorder(QPainter& painter,
                                      const TextHighlight& highlight,
                                      double scaleFactor = 1.0);
    static void renderHighlightNote(QPainter& painter,
                                    const TextHighlight& highlight,
                                    const QPointF& position,
                                    double scaleFactor = 1.0);
};

/**
 * @brief Helper class for highlight import/export
 */
class HighlightImportExport {
public:
    enum class Format { JSON, Markdown, PlainText, HTML, CSV };

    static bool exportHighlights(const QList<TextHighlight>& highlights,
                                 const QString& filePath, Format format);
    static QList<TextHighlight> importHighlights(const QString& filePath,
                                                 Format format);

    static QString toMarkdown(const QList<TextHighlight>& highlights,
                              const QString& documentPath);
    static QString toPlainText(const QList<TextHighlight>& highlights,
                               const QString& documentPath);
    static QString toHTML(const QList<TextHighlight>& highlights,
                          const QString& documentPath);
    static QString toCSV(const QList<TextHighlight>& highlights);
    static QJsonArray toJSON(const QList<TextHighlight>& highlights);

    static QList<TextHighlight> fromJSON(const QJsonArray& jsonArray);
};
