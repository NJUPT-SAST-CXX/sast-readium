#pragma once

#include <poppler-qt6.h>
#include <QAbstractListModel>
#include <QColor>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QRandomGenerator>
#include <QRectF>
#include <QString>

/**
 * Annotation types supported by the system
 */
enum class AnnotationType {
    Highlight,  // Text highlighting
    Note,       // Sticky note
    FreeText,   // Free text annotation
    Underline,  // Text underline
    StrikeOut,  // Text strikeout
    Squiggly,   // Squiggly underline
    Rectangle,  // Rectangle shape
    Circle,     // Circle/ellipse shape
    Line,       // Line annotation
    Arrow,      // Arrow annotation
    Ink         // Freehand drawing
};

/**
 * Represents a single annotation
 */
struct PDFAnnotation {
    QString id;              // Unique identifier
    AnnotationType type;     // Type of annotation
    int pageNumber;          // Page number (0-based)
    QRectF boundingRect;     // Bounding rectangle
    QString content;         // Text content/notes
    QString author;          // Author name
    QDateTime createdTime;   // Creation timestamp
    QDateTime modifiedTime;  // Last modification timestamp
    QColor color;            // Annotation color
    double opacity;          // Opacity (0.0-1.0)
    bool isVisible;          // Visibility flag

    // Type-specific properties
    QList<QPointF> inkPath;  // For ink annotations
    QPointF startPoint;      // For line/arrow annotations
    QPointF endPoint;        // For line/arrow annotations
    double lineWidth;        // Line width for shapes
    QString fontFamily;      // Font for text annotations
    int fontSize;            // Font size for text annotations

    PDFAnnotation()
        : type(AnnotationType::Highlight),
          pageNumber(-1),
          opacity(1.0),
          isVisible(true),
          lineWidth(1.0),
          fontSize(12),
          color(Qt::yellow),
          createdTime(QDateTime::currentDateTime()),
          modifiedTime(QDateTime::currentDateTime()) {
        // Generate unique ID
        id = QString("ann_%1_%2")
                 .arg(QDateTime::currentMSecsSinceEpoch())
                 .arg(QRandomGenerator::global()->bounded(10000));
    }

    // Serialization
    QJsonObject toJson() const;
    static PDFAnnotation fromJson(const QJsonObject& json);

    // Poppler integration
    Poppler::Annotation* toPopplerAnnotation() const;
    static PDFAnnotation fromPopplerAnnotation(Poppler::Annotation* annotation,
                                               int pageNum);

    // Utility methods
    bool containsPoint(const QPointF& point) const;
    QString getTypeString() const;
    static AnnotationType typeFromString(const QString& typeStr);
};

/**
 * Model for managing PDF annotations
 */
class AnnotationModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum AnnotationRole {
        IdRole = Qt::UserRole + 1,
        TypeRole,
        PageNumberRole,
        BoundingRectRole,
        ContentRole,
        AuthorRole,
        CreatedTimeRole,
        ModifiedTimeRole,
        ColorRole,
        OpacityRole,
        VisibilityRole
    };

    explicit AnnotationModel(QObject* parent = nullptr);
    ~AnnotationModel() = default;

    // QAbstractListModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value,
                 int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Annotation operations
    bool addAnnotation(const PDFAnnotation& annotation);
    bool removeAnnotation(const QString& annotationId);
    bool updateAnnotation(const QString& annotationId,
                          const PDFAnnotation& updatedAnnotation);
    PDFAnnotation getAnnotation(const QString& annotationId) const;
    QList<PDFAnnotation> getAllAnnotations() const;

    // Page-specific operations
    QList<PDFAnnotation> getAnnotationsForPage(int pageNumber) const;
    bool removeAnnotationsForPage(int pageNumber);
    int getAnnotationCountForPage(int pageNumber) const;

    // Document integration
    void setDocument(Poppler::Document* document);
    bool loadAnnotationsFromDocument();
    bool saveAnnotationsToDocument();
    void clearAnnotations();

    // Search and filtering
    QList<PDFAnnotation> searchAnnotations(const QString& query) const;
    QList<PDFAnnotation> getAnnotationsByType(AnnotationType type) const;
    QList<PDFAnnotation> getAnnotationsByAuthor(const QString& author) const;
    QList<PDFAnnotation> getRecentAnnotations(int count = 10) const;

    // Statistics
    int getTotalAnnotationCount() const { return m_annotations.size(); }
    QMap<AnnotationType, int> getAnnotationCountByType() const;
    QStringList getAuthors() const;

signals:
    void annotationAdded(const PDFAnnotation& annotation);
    void annotationRemoved(const QString& annotationId);
    void annotationUpdated(const PDFAnnotation& annotation);
    void annotationsLoaded(int count);
    void annotationsSaved(int count);
    void annotationsCleared();

private:
    int findAnnotationIndex(const QString& annotationId) const;
    void sortAnnotations();
    QString generateUniqueId() const;

    QList<PDFAnnotation> m_annotations;
    Poppler::Document* m_document;
};
