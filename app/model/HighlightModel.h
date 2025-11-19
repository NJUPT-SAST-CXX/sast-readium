#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QRectF>
#include <QString>
#include <QUuid>
#include <QVariant>

/**
 * @brief Highlight color presets
 */
enum class HighlightColor {
    Yellow,  // Default yellow highlight
    Green,   // Green highlight
    Blue,    // Blue highlight
    Pink,    // Pink highlight
    Orange,  // Orange highlight
    Purple,  // Purple highlight
    Red,     // Red highlight
    Custom   // Custom color
};

/**
 * @brief Represents a single text highlight annotation
 */
struct TextHighlight {
    QString id;                  // Unique identifier (UUID)
    int pageNumber;              // Page number (0-based)
    QList<QRectF> rects;         // Bounding rectangles for highlighted text
    QString text;                // Highlighted text content
    QColor color;                // Highlight color
    double opacity;              // Opacity (0.0-1.0)
    QString note;                // Optional note/comment
    QString author;              // Author name
    QDateTime createdTime;       // Creation timestamp
    QDateTime modifiedTime;      // Last modification timestamp
    bool isVisible;              // Visibility flag
    HighlightColor colorPreset;  // Color preset type

    // Selection metadata
    int startCharIndex;  // Start character index in page text
    int endCharIndex;    // End character index in page text
    QPointF startPoint;  // Start point in page coordinates
    QPointF endPoint;    // End point in page coordinates

    TextHighlight()
        : pageNumber(-1),
          color(Qt::yellow),
          opacity(0.4),
          createdTime(QDateTime::currentDateTime()),
          modifiedTime(QDateTime::currentDateTime()),
          isVisible(true),
          colorPreset(HighlightColor::Yellow),
          startCharIndex(-1),
          endCharIndex(-1) {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    // Serialization
    QJsonObject toJson() const;
    static TextHighlight fromJson(const QJsonObject& json);

    // Utility methods
    bool containsPoint(const QPointF& point) const;
    QRectF boundingRect() const;
    bool isEmpty() const { return rects.isEmpty() || text.isEmpty(); }
    QString getColorName() const;
    static QColor getColorFromPreset(HighlightColor preset);
    static HighlightColor getPresetFromColor(const QColor& color);
};

/**
 * @brief Model for managing text highlights in PDF documents
 */
class HighlightModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum HighlightRole {
        IdRole = Qt::UserRole + 1,
        PageNumberRole,
        RectsRole,
        TextRole,
        ColorRole,
        OpacityRole,
        NoteRole,
        AuthorRole,
        CreatedTimeRole,
        ModifiedTimeRole,
        VisibilityRole,
        ColorPresetRole
    };

    explicit HighlightModel(QObject* parent = nullptr);
    ~HighlightModel() override = default;

    // QAbstractListModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value,
                 int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Highlight operations
    bool addHighlight(const TextHighlight& highlight);
    bool removeHighlight(const QString& highlightId);
    bool updateHighlight(const QString& highlightId,
                         const TextHighlight& updatedHighlight);
    TextHighlight getHighlight(const QString& highlightId) const;
    QList<TextHighlight> getAllHighlights() const { return m_highlights; }

    // Page-specific operations
    QList<TextHighlight> getHighlightsForPage(int pageNumber) const;
    bool removeHighlightsForPage(int pageNumber);
    int getHighlightCountForPage(int pageNumber) const;

    // Search and filtering
    QList<TextHighlight> searchHighlights(const QString& query) const;
    QList<TextHighlight> getHighlightsByColor(HighlightColor color) const;
    QList<TextHighlight> getHighlightsByAuthor(const QString& author) const;
    QList<TextHighlight> getRecentHighlights(int count = 10) const;
    QList<TextHighlight> getHighlightsWithNotes() const;

    // Editing operations
    bool editHighlightNote(const QString& highlightId, const QString& newNote);
    bool changeHighlightColor(const QString& highlightId,
                              const QColor& newColor);
    bool changeHighlightOpacity(const QString& highlightId, double opacity);
    bool toggleHighlightVisibility(const QString& highlightId);

    // Batch operations
    bool mergeHighlights(const QStringList& highlightIds);
    bool splitHighlight(const QString& highlightId, int splitCharIndex);
    bool removeAllHighlights();

    // Document integration
    void setDocumentPath(const QString& path) { m_documentPath = path; }
    QString getDocumentPath() const { return m_documentPath; }

    // Persistence
    bool saveToFile(const QString& filePath) const;
    bool loadFromFile(const QString& filePath);
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);

    // Statistics
    int getTotalHighlightCount() const { return m_highlights.size(); }
    QMap<HighlightColor, int> getHighlightCountByColor() const;
    QStringList getAuthors() const;
    int getTotalPages() const;
    double getAverageHighlightsPerPage() const;

    // Hit testing
    TextHighlight findHighlightAtPoint(int pageNumber,
                                       const QPointF& point) const;
    QList<TextHighlight> findHighlightsInRect(int pageNumber,
                                              const QRectF& rect) const;

    // Export
    QString exportToMarkdown() const;
    QString exportToPlainText() const;
    QJsonArray exportToJson() const;

signals:
    void highlightAdded(const TextHighlight& highlight);
    void highlightRemoved(const QString& highlightId);
    void highlightUpdated(const TextHighlight& highlight);
    void highlightsLoaded(int count);
    void highlightsSaved(int count);
    void highlightsCleared();
    void highlightVisibilityChanged(const QString& highlightId, bool visible);

private:
    int findHighlightIndex(const QString& highlightId) const;
    void sortHighlights();
    QString generateUniqueId() const;

    QList<TextHighlight> m_highlights;
    QString m_documentPath;
};

/**
 * @brief Statistics and analytics for highlights
 */
struct HighlightStatistics {
    int totalHighlights;
    int totalPages;
    QMap<HighlightColor, int> colorDistribution;
    QMap<int, int> pageDistribution;  // page -> count
    QMap<QString, int> authorDistribution;
    int highlightsWithNotes;
    double averageHighlightLength;
    double averageHighlightsPerPage;
    QDateTime oldestHighlight;
    QDateTime newestHighlight;

    HighlightStatistics()
        : totalHighlights(0),
          totalPages(0),
          highlightsWithNotes(0),
          averageHighlightLength(0.0),
          averageHighlightsPerPage(0.0) {}

    QJsonObject toJson() const;
    static HighlightStatistics fromHighlights(
        const QList<TextHighlight>& highlights);
};

/**
 * @brief Helper class for highlight color management
 */
class HighlightColorManager {
public:
    static QColor getDefaultColor(HighlightColor preset);
    static QString getColorName(HighlightColor preset);
    static QList<HighlightColor> getAllPresets();
    static HighlightColor getPresetFromName(const QString& name);
};
