#include "HighlightManager.h"
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include "../controller/EventBus.h"
#include "../interaction/TextSelectionManager.h"
#include "../logging/SimpleLogging.h"

// Singleton instance
HighlightManager& HighlightManager::instance() {
    static HighlightManager instance;
    return instance;
}

HighlightManager::HighlightManager(QObject* parent)
    : QObject(parent),
      m_model(std::make_unique<HighlightModel>()),
      m_undoStack(nullptr),
      m_selectionManager(nullptr),
      m_document(nullptr),
      m_autoSaveEnabled(true),
      m_autoSaveInterval(30000),  // 30 seconds
      m_defaultColor(HighlightColor::Yellow),
      m_defaultOpacity(0.4),
      m_hasUnsavedChanges(false) {
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(m_autoSaveInterval);
    connect(m_autoSaveTimer, &QTimer::timeout, this,
            &HighlightManager::performAutoSave);

    connectModelSignals();

    SLOG_INFO("HighlightManager initialized");
}

HighlightManager::~HighlightManager() {
    if (m_autoSaveEnabled && m_hasUnsavedChanges) {
        performAutoSave();
    }
    SLOG_INFO("HighlightManager destroyed");
}

void HighlightManager::setDocument(Poppler::Document* document,
                                   const QString& documentPath) {
    // Save current document's highlights if needed
    if (m_document && m_hasUnsavedChanges) {
        performAutoSave();
    }

    m_document = document;
    m_documentPath = documentPath;
    m_model->setDocumentPath(documentPath);

    // Load highlights for new document
    if (!documentPath.isEmpty()) {
        loadHighlights();
    }

    // Start auto-save timer
    if (m_autoSaveEnabled) {
        m_autoSaveTimer->start();
    }

    SLOG_INFO(QString("Document set: %1").arg(documentPath));
}

void HighlightManager::setUndoStack(QUndoStack* undoStack) {
    m_undoStack = undoStack;
    SLOG_DEBUG("UndoStack set");
}

void HighlightManager::setTextSelectionManager(
    TextSelectionManager* selectionManager) {
    m_selectionManager = selectionManager;
    if (m_selectionManager) {
        connect(m_selectionManager, &TextSelectionManager::selectionChanged,
                this, &HighlightManager::onSelectionChanged);
    }
    SLOG_DEBUG("TextSelectionManager set");
}

// Highlight creation
bool HighlightManager::addHighlight(const TextHighlight& highlight) {
    if (!m_undoStack) {
        SLOG_WARN("No undo stack set, adding highlight directly");
        return m_model->addHighlight(highlight);
    }

    auto* cmd =
        HighlightCommandFactory::createAddCommand(m_model.get(), highlight);
    m_undoStack->push(cmd);
    m_hasUnsavedChanges = true;

    SLOG_INFO(QString("Highlight added: %1").arg(highlight.id));
    return true;
}

bool HighlightManager::addHighlightFromSelection(const TextSelection& selection,
                                                 int pageNumber,
                                                 HighlightColor color,
                                                 double opacity) {
    if (selection.isEmpty()) {
        SLOG_WARN("Cannot create highlight from empty selection");
        return false;
    }

    TextHighlight highlight = HighlightCreator::createFromSelection(
        selection, pageNumber, color, opacity);
    highlight.author =
        m_defaultAuthor.isEmpty() ? qgetenv("USER") : m_defaultAuthor;
    if (highlight.author.isEmpty()) {
        highlight.author = qgetenv("USERNAME");
    }

    return addHighlight(highlight);
}

bool HighlightManager::addHighlightWithNote(const TextSelection& selection,
                                            int pageNumber, const QString& note,
                                            HighlightColor color) {
    TextHighlight highlight =
        HighlightCreator::createWithNote(selection, pageNumber, note, color);
    highlight.author =
        m_defaultAuthor.isEmpty() ? qgetenv("USER") : m_defaultAuthor;
    if (highlight.author.isEmpty()) {
        highlight.author = qgetenv("USERNAME");
    }

    return addHighlight(highlight);
}

// Highlight editing
bool HighlightManager::removeHighlight(const QString& highlightId) {
    if (!m_undoStack) {
        return m_model->removeHighlight(highlightId);
    }

    auto* cmd = HighlightCommandFactory::createRemoveCommand(m_model.get(),
                                                             highlightId);
    m_undoStack->push(cmd);
    m_hasUnsavedChanges = true;

    SLOG_INFO(QString("Highlight removed: %1").arg(highlightId));
    return true;
}

bool HighlightManager::editHighlightNote(const QString& highlightId,
                                         const QString& newNote) {
    if (!m_undoStack) {
        return m_model->editHighlightNote(highlightId, newNote);
    }

    auto* cmd = HighlightCommandFactory::createEditNoteCommand(
        m_model.get(), highlightId, newNote);
    m_undoStack->push(cmd);
    m_hasUnsavedChanges = true;

    return true;
}

bool HighlightManager::changeHighlightColor(const QString& highlightId,
                                            const QColor& newColor) {
    if (!m_undoStack) {
        return m_model->changeHighlightColor(highlightId, newColor);
    }

    auto* cmd = HighlightCommandFactory::createChangeColorCommand(
        m_model.get(), highlightId, newColor);
    m_undoStack->push(cmd);
    m_hasUnsavedChanges = true;

    return true;
}

bool HighlightManager::changeHighlightOpacity(const QString& highlightId,
                                              double opacity) {
    if (!m_undoStack) {
        return m_model->changeHighlightOpacity(highlightId, opacity);
    }

    auto* cmd = HighlightCommandFactory::createChangeOpacityCommand(
        m_model.get(), highlightId, opacity);
    m_undoStack->push(cmd);
    m_hasUnsavedChanges = true;

    return true;
}

bool HighlightManager::toggleHighlightVisibility(const QString& highlightId) {
    if (!m_undoStack) {
        return m_model->toggleHighlightVisibility(highlightId);
    }

    auto* cmd = HighlightCommandFactory::createToggleVisibilityCommand(
        m_model.get(), highlightId);
    m_undoStack->push(cmd);
    m_hasUnsavedChanges = true;

    return true;
}

bool HighlightManager::updateHighlight(const QString& highlightId,
                                       const TextHighlight& newHighlight) {
    if (!m_undoStack) {
        return m_model->updateHighlight(highlightId, newHighlight);
    }

    auto* cmd = HighlightCommandFactory::createUpdateCommand(
        m_model.get(), highlightId, newHighlight);
    m_undoStack->push(cmd);
    m_hasUnsavedChanges = true;

    return true;
}

// Batch operations
bool HighlightManager::addMultipleHighlights(
    const QList<TextHighlight>& highlights) {
    if (!m_undoStack) {
        for (const auto& highlight : highlights) {
            m_model->addHighlight(highlight);
        }
        return true;
    }

    auto* cmd = HighlightCommandFactory::createBatchAddCommand(m_model.get(),
                                                               highlights);
    m_undoStack->push(cmd);
    m_hasUnsavedChanges = true;

    SLOG_INFO(QString("Batch add: %1 highlights").arg(highlights.size()));
    return true;
}

bool HighlightManager::removeMultipleHighlights(
    const QStringList& highlightIds) {
    if (!m_undoStack) {
        for (const auto& id : highlightIds) {
            m_model->removeHighlight(id);
        }
        return true;
    }

    auto* cmd = HighlightCommandFactory::createBatchRemoveCommand(m_model.get(),
                                                                  highlightIds);
    m_undoStack->push(cmd);
    m_hasUnsavedChanges = true;

    SLOG_INFO(QString("Batch remove: %1 highlights").arg(highlightIds.size()));
    return true;
}

bool HighlightManager::removeAllHighlights() {
    if (!m_undoStack) {
        return m_model->removeAllHighlights();
    }

    auto* cmd = HighlightCommandFactory::createClearAllCommand(m_model.get());
    m_undoStack->push(cmd);
    m_hasUnsavedChanges = true;

    SLOG_INFO("All highlights removed");
    return true;
}

bool HighlightManager::removePageHighlights(int pageNumber) {
    if (!m_undoStack) {
        return m_model->removeHighlightsForPage(pageNumber);
    }

    auto* cmd = HighlightCommandFactory::createRemovePageCommand(m_model.get(),
                                                                 pageNumber);
    m_undoStack->push(cmd);
    m_hasUnsavedChanges = true;

    SLOG_INFO(QString("Page %1 highlights removed").arg(pageNumber));
    return true;
}

// Query operations
TextHighlight HighlightManager::getHighlight(const QString& highlightId) const {
    return m_model->getHighlight(highlightId);
}

QList<TextHighlight> HighlightManager::getAllHighlights() const {
    return m_model->getAllHighlights();
}

QList<TextHighlight> HighlightManager::getHighlightsForPage(
    int pageNumber) const {
    return m_model->getHighlightsForPage(pageNumber);
}

QList<TextHighlight> HighlightManager::searchHighlights(
    const QString& query) const {
    return m_model->searchHighlights(query);
}

TextHighlight HighlightManager::findHighlightAtPoint(
    int pageNumber, const QPointF& point) const {
    return m_model->findHighlightAtPoint(pageNumber, point);
}

// Persistence
bool HighlightManager::saveHighlights(const QString& filePath) {
    QString path = filePath.isEmpty() ? getHighlightFilePath() : filePath;

    if (path.isEmpty()) {
        SLOG_ERROR("Cannot save highlights: no file path");
        return false;
    }

    bool success = m_model->saveToFile(path);
    if (success) {
        m_hasUnsavedChanges = false;
        emit highlightsSaved(m_model->getTotalHighlightCount());
        SLOG_INFO(QString("Highlights saved to: %1").arg(path));
    } else {
        SLOG_ERROR(QString("Failed to save highlights to: %1").arg(path));
    }

    return success;
}

bool HighlightManager::loadHighlights(const QString& filePath) {
    QString path = filePath.isEmpty() ? getHighlightFilePath() : filePath;

    if (path.isEmpty() || !QFile::exists(path)) {
        SLOG_DEBUG(QString("No highlight file found: %1").arg(path));
        return false;
    }

    bool success = m_model->loadFromFile(path);
    if (success) {
        m_hasUnsavedChanges = false;
        SLOG_INFO(QString("Highlights loaded from: %1").arg(path));
    } else {
        SLOG_ERROR(QString("Failed to load highlights from: %1").arg(path));
    }

    return success;
}

void HighlightManager::setAutoSaveEnabled(bool enabled) {
    m_autoSaveEnabled = enabled;
    if (enabled) {
        m_autoSaveTimer->start();
        SLOG_INFO("Auto-save enabled");
    } else {
        m_autoSaveTimer->stop();
        SLOG_INFO("Auto-save disabled");
    }
}

QString HighlightManager::getHighlightFilePath() const {
    if (m_documentPath.isEmpty()) {
        return QString();
    }

    QFileInfo fileInfo(m_documentPath);
    QString baseName = fileInfo.completeBaseName();
    QString dirPath = fileInfo.absolutePath();

    return QDir(dirPath).filePath(baseName + "_highlights.json");
}

// Export
QString HighlightManager::exportToMarkdown() const {
    return m_model->exportToMarkdown();
}

QString HighlightManager::exportToPlainText() const {
    return m_model->exportToPlainText();
}

QJsonArray HighlightManager::exportToJson() const {
    return m_model->exportToJson();
}

bool HighlightManager::exportToFile(const QString& filePath,
                                    const QString& format) {
    QString content;

    if (format == "markdown" || format == "md") {
        content = exportToMarkdown();
    } else if (format == "text" || format == "txt") {
        content = exportToPlainText();
    } else if (format == "json") {
        QJsonDocument doc(exportToJson());
        content = doc.toJson(QJsonDocument::Indented);
    } else {
        SLOG_ERROR(QString("Unknown export format: %1").arg(format));
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        SLOG_ERROR(QString("Cannot open file for writing: %1").arg(filePath));
        return false;
    }

    file.write(content.toUtf8());
    file.close();

    SLOG_INFO(QString("Highlights exported to: %1 (format: %2)")
                  .arg(filePath)
                  .arg(format));
    return true;
}

// Statistics
int HighlightManager::getTotalHighlightCount() const {
    return m_model->getTotalHighlightCount();
}

int HighlightManager::getHighlightCountForPage(int pageNumber) const {
    return m_model->getHighlightCountForPage(pageNumber);
}

HighlightStatistics HighlightManager::getStatistics() const {
    return HighlightStatistics::fromHighlights(m_model->getAllHighlights());
}

QMap<HighlightColor, int> HighlightManager::getColorDistribution() const {
    return m_model->getHighlightCountByColor();
}

// Slots
void HighlightManager::onSelectionChanged() { SLOG_DEBUG("Selection changed"); }

void HighlightManager::onDocumentClosed() {
    if (m_hasUnsavedChanges) {
        performAutoSave();
    }

    m_document = nullptr;
    m_documentPath.clear();
    m_model->removeAllHighlights();
    m_autoSaveTimer->stop();

    SLOG_INFO("Document closed, highlights cleared");
}

void HighlightManager::performAutoSave() {
    if (!m_hasUnsavedChanges || m_documentPath.isEmpty()) {
        return;
    }

    if (saveHighlights()) {
        emit autoSaveCompleted();
        SLOG_DEBUG("Auto-save completed");
    } else {
        emit autoSaveFailed("Failed to auto-save highlights");
        SLOG_ERROR("Auto-save failed");
    }
}

// Private slots
void HighlightManager::onModelHighlightAdded(const TextHighlight& highlight) {
    emit highlightAdded(highlight);

    QVariantMap data;
    data["id"] = highlight.id;
    data["pageNumber"] = highlight.pageNumber;
    data["text"] = highlight.text;
    EventBus::instance().publish("highlight_added", data);

    scheduleAutoSave();
}

void HighlightManager::onModelHighlightRemoved(const QString& highlightId) {
    emit highlightRemoved(highlightId);
    EventBus::instance().publish("highlight_removed", highlightId);
    scheduleAutoSave();
}

void HighlightManager::onModelHighlightUpdated(const TextHighlight& highlight) {
    emit highlightUpdated(highlight);

    QVariantMap data;
    data["id"] = highlight.id;
    data["pageNumber"] = highlight.pageNumber;
    EventBus::instance().publish("highlight_updated", data);

    scheduleAutoSave();
}

// Private methods
void HighlightManager::connectModelSignals() {
    connect(m_model.get(), &HighlightModel::highlightAdded, this,
            &HighlightManager::onModelHighlightAdded);
    connect(m_model.get(), &HighlightModel::highlightRemoved, this,
            &HighlightManager::onModelHighlightRemoved);
    connect(m_model.get(), &HighlightModel::highlightUpdated, this,
            &HighlightManager::onModelHighlightUpdated);
    connect(m_model.get(), &HighlightModel::highlightsLoaded, this,
            &HighlightManager::highlightsLoaded);
    connect(m_model.get(), &HighlightModel::highlightsSaved, this,
            &HighlightManager::highlightsSaved);
    connect(m_model.get(), &HighlightModel::highlightsCleared, this,
            &HighlightManager::highlightsCleared);
    connect(m_model.get(), &HighlightModel::highlightVisibilityChanged, this,
            &HighlightManager::highlightVisibilityChanged);
}

void HighlightManager::disconnectModelSignals() {
    disconnect(m_model.get(), nullptr, this, nullptr);
}

void HighlightManager::scheduleAutoSave() {
    if (m_autoSaveEnabled && !m_autoSaveTimer->isActive()) {
        m_autoSaveTimer->start();
    }
}

// HighlightRenderer implementation
void HighlightRenderer::renderHighlight(QPainter& painter,
                                        const TextHighlight& highlight,
                                        double scaleFactor) {
    if (!highlight.isVisible) {
        return;
    }

    painter.save();

    QColor color = highlight.color;
    color.setAlphaF(highlight.opacity);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);

    for (const auto& rect : highlight.rects) {
        QRectF scaledRect(rect.x() * scaleFactor, rect.y() * scaleFactor,
                          rect.width() * scaleFactor,
                          rect.height() * scaleFactor);
        painter.drawRect(scaledRect);
    }

    painter.restore();
}

void HighlightRenderer::renderHighlights(QPainter& painter,
                                         const QList<TextHighlight>& highlights,
                                         double scaleFactor) {
    for (const auto& highlight : highlights) {
        renderHighlight(painter, highlight, scaleFactor);
    }
}

void HighlightRenderer::renderHighlightBorder(QPainter& painter,
                                              const TextHighlight& highlight,
                                              double scaleFactor) {
    if (!highlight.isVisible) {
        return;
    }

    painter.save();

    QColor borderColor = highlight.color.darker(150);
    QPen pen(borderColor, 1.0);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    for (const auto& rect : highlight.rects) {
        QRectF scaledRect(rect.x() * scaleFactor, rect.y() * scaleFactor,
                          rect.width() * scaleFactor,
                          rect.height() * scaleFactor);
        painter.drawRect(scaledRect);
    }

    painter.restore();
}

void HighlightRenderer::renderHighlightNote(QPainter& painter,
                                            const TextHighlight& highlight,
                                            const QPointF& position,
                                            double scaleFactor) {
    if (highlight.note.isEmpty()) {
        return;
    }

    painter.save();

    QRectF noteRect(position.x() * scaleFactor, position.y() * scaleFactor, 16,
                    16);
    painter.setBrush(QColor(255, 255, 200));
    painter.setPen(Qt::black);
    painter.drawRect(noteRect);
    painter.drawText(noteRect, Qt::AlignCenter, "N");

    painter.restore();
}

// HighlightImportExport implementation
bool HighlightImportExport::exportHighlights(
    const QList<TextHighlight>& highlights, const QString& filePath,
    Format format) {
    QString content;

    switch (format) {
        case Format::JSON:
            content = QJsonDocument(toJSON(highlights))
                          .toJson(QJsonDocument::Indented);
            break;
        case Format::Markdown:
            content = toMarkdown(highlights, "");
            break;
        case Format::PlainText:
            content = toPlainText(highlights, "");
            break;
        default:
            return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    file.write(content.toUtf8());
    return true;
}

QJsonArray HighlightImportExport::toJSON(
    const QList<TextHighlight>& highlights) {
    QJsonArray array;
    for (const auto& highlight : highlights) {
        array.append(highlight.toJson());
    }
    return array;
}

QString HighlightImportExport::toMarkdown(
    const QList<TextHighlight>& highlights, const QString& documentPath) {
    QString md;
    md += QString("# Highlights for %1\n\n").arg(documentPath);

    QMap<int, QList<TextHighlight>> byPage;
    for (const auto& h : highlights) {
        byPage[h.pageNumber].append(h);
    }

    for (auto it = byPage.constBegin(); it != byPage.constEnd(); ++it) {
        md += QString("## Page %1\n\n").arg(it.key() + 1);
        for (const auto& h : it.value()) {
            md += QString("- **%1** (%2)\n").arg(h.text).arg(h.getColorName());
            if (!h.note.isEmpty()) {
                md += QString("  > %1\n").arg(h.note);
            }
            md += "\n";
        }
    }

    return md;
}

QString HighlightImportExport::toPlainText(
    const QList<TextHighlight>& highlights, const QString& documentPath) {
    QString text;
    text += QString("Highlights for: %1\n").arg(documentPath);
    text += QString("Total: %1 highlights\n\n").arg(highlights.size());

    for (const auto& h : highlights) {
        text += QString("[Page %1] %2\n").arg(h.pageNumber + 1).arg(h.text);
        if (!h.note.isEmpty()) {
            text += QString("Note: %1\n").arg(h.note);
        }
        text += "\n";
    }

    return text;
}
