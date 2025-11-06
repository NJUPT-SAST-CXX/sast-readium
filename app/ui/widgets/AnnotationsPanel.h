#ifndef ANNOTATIONSPANEL_H
#define ANNOTATIONSPANEL_H

#include <poppler/qt6/poppler-qt6.h>
#include <QWidget>
#include "model/AnnotationModel.h"

// Forward declarations
class ElaText;
class ElaListView;
class ElaPushButton;
class QVBoxLayout;

/**
 * @brief AnnotationsPanel - Document annotations display and navigation panel
 *
 * Displays all annotations in the current document:
 * - List of annotations with type, author, and content
 * - Page number for each annotation
 * - Creation and modification dates
 * - Click to navigate to annotation location
 * - Handles documents with no annotations gracefully
 */
class AnnotationsPanel : public QWidget {
    Q_OBJECT

public:
    explicit AnnotationsPanel(QWidget* parent = nullptr);
    ~AnnotationsPanel() override;

    // Document management
    void setDocument(Poppler::Document* document);
    void clearDocument();
    bool hasDocument() const;

    // Annotation count
    int getAnnotationCount() const;

signals:
    void documentChanged();
    void annotationClicked(int pageNumber, const QString& annotationId);
    void navigateToPage(int pageNumber);

private slots:
    void onAnnotationItemClicked(const QModelIndex& index);
    void onRefreshClicked();

private:
    // UI Components
    QVBoxLayout* m_mainLayout;
    ElaText* m_titleLabel;
    ElaText* m_countLabel;
    ElaListView* m_annotationsList;
    ElaPushButton* m_refreshButton;
    ElaText* m_emptyLabel;

    // Data
    Poppler::Document* m_document;
    AnnotationModel* m_annotationModel;

    // UI setup
    void setupUi();
    void updateAnnotationsList();
    void showEmptyState();
    void showAnnotationsList();

    // Helper methods
    QString getAnnotationTypeString(AnnotationType type) const;
    QString formatAnnotationSummary(const PDFAnnotation& annotation) const;
};

#endif  // ANNOTATIONSPANEL_H
