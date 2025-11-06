#include "AnnotationsPanel.h"

// ElaWidgetTools
#include "ElaListView.h"
#include "ElaPushButton.h"
#include "ElaText.h"

// Qt
#include <QHBoxLayout>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>

// Logging
#include "logging/SimpleLogging.h"

// ============================================================================
// Constructor and Destructor
// ============================================================================

AnnotationsPanel::AnnotationsPanel(QWidget* parent)
    : QWidget(parent), m_document(nullptr), m_annotationModel(nullptr) {
    SLOG_INFO("AnnotationsPanel: Constructor started");

    // Create annotation model
    m_annotationModel = new AnnotationModel(this);

    setupUi();
    showEmptyState();

    SLOG_INFO("AnnotationsPanel: Constructor completed");
}

AnnotationsPanel::~AnnotationsPanel() {
    SLOG_INFO("AnnotationsPanel: Destructor called");
}

// ============================================================================
// UI Setup
// ============================================================================

void AnnotationsPanel::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    m_mainLayout->setSpacing(10);

    // Title and count
    QHBoxLayout* headerLayout = new QHBoxLayout();

    m_titleLabel = new ElaText(tr("Annotations"), this);
    m_titleLabel->setTextPixelSize(16);
    headerLayout->addWidget(m_titleLabel);

    m_countLabel = new ElaText(tr("(0)"), this);
    m_countLabel->setStyleSheet("ElaText { color: #666666; }");
    headerLayout->addWidget(m_countLabel);

    headerLayout->addStretch();

    // Refresh button
    m_refreshButton = new ElaPushButton(tr("Refresh"), this);
    m_refreshButton->setMaximumWidth(80);
    connect(m_refreshButton, &ElaPushButton::clicked, this,
            &AnnotationsPanel::onRefreshClicked);
    headerLayout->addWidget(m_refreshButton);

    m_mainLayout->addLayout(headerLayout);

    // Annotations list
    m_annotationsList = new ElaListView(this);
    m_annotationsList->setModel(m_annotationModel);
    connect(m_annotationsList, &ElaListView::clicked, this,
            &AnnotationsPanel::onAnnotationItemClicked);
    m_mainLayout->addWidget(m_annotationsList);

    // Empty state label
    m_emptyLabel = new ElaText(tr("No annotations in this document"), this);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("ElaText { color: #999999; padding: 20px; }");
    m_mainLayout->addWidget(m_emptyLabel);
}

// ============================================================================
// Document Management
// ============================================================================

void AnnotationsPanel::setDocument(Poppler::Document* document) {
    if (!document) {
        SLOG_WARNING("AnnotationsPanel::setDocument: Null document provided");
        clearDocument();
        return;
    }

    SLOG_INFO("AnnotationsPanel: Setting document");

    m_document = document;

    // Set document in annotation model
    m_annotationModel->setDocument(document);

    // Update UI
    updateAnnotationsList();

    emit documentChanged();
}

void AnnotationsPanel::clearDocument() {
    SLOG_INFO("AnnotationsPanel: Clearing document");

    m_document = nullptr;

    // Clear annotation model
    m_annotationModel->clearAnnotations();

    // Show empty state
    showEmptyState();
}

bool AnnotationsPanel::hasDocument() const { return m_document != nullptr; }

int AnnotationsPanel::getAnnotationCount() const {
    return m_annotationModel->rowCount();
}

// ============================================================================
// UI Update Methods
// ============================================================================

void AnnotationsPanel::updateAnnotationsList() {
    if (!m_document) {
        showEmptyState();
        return;
    }

    int count = m_annotationModel->rowCount();

    SLOG_INFO("AnnotationsPanel: Found " + QString::number(count) +
              " annotations");

    // Update count label
    m_countLabel->setText(QString("(%1)").arg(count));

    if (count == 0) {
        showEmptyState();
    } else {
        showAnnotationsList();
    }
}

void AnnotationsPanel::showEmptyState() {
    m_annotationsList->setVisible(false);
    m_emptyLabel->setVisible(true);
    m_countLabel->setText(tr("(0)"));
}

void AnnotationsPanel::showAnnotationsList() {
    m_annotationsList->setVisible(true);
    m_emptyLabel->setVisible(false);
}

// ============================================================================
// Slots
// ============================================================================

void AnnotationsPanel::onAnnotationItemClicked(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }

    // Get annotation data from model
    PDFAnnotation annotation = m_annotationModel->getAnnotation(
        index.data(AnnotationModel::IdRole).toString());

    if (annotation.pageNumber >= 0) {
        SLOG_INFO("AnnotationsPanel: Navigating to annotation on page " +
                  QString::number(annotation.pageNumber + 1));

        emit navigateToPage(annotation.pageNumber);
        emit annotationClicked(annotation.pageNumber, annotation.id);
    }
}

void AnnotationsPanel::onRefreshClicked() {
    SLOG_INFO("AnnotationsPanel: Refresh requested");

    if (m_document) {
        // Reload annotations from document
        m_annotationModel->setDocument(m_document);
        updateAnnotationsList();
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

QString AnnotationsPanel::getAnnotationTypeString(AnnotationType type) const {
    switch (type) {
        case AnnotationType::Highlight:
            return tr("Highlight");
        case AnnotationType::Note:
            return tr("Note");
        case AnnotationType::FreeText:
            return tr("Text");
        case AnnotationType::Underline:
            return tr("Underline");
        case AnnotationType::StrikeOut:
            return tr("Strikeout");
        case AnnotationType::Squiggly:
            return tr("Squiggly");
        case AnnotationType::Rectangle:
            return tr("Rectangle");
        case AnnotationType::Circle:
            return tr("Circle");
        case AnnotationType::Line:
            return tr("Line");
        case AnnotationType::Arrow:
            return tr("Arrow");
        case AnnotationType::Ink:
            return tr("Ink");
        default:
            return tr("Unknown");
    }
}

QString AnnotationsPanel::formatAnnotationSummary(
    const PDFAnnotation& annotation) const {
    QString summary;

    // Type and page
    summary += QString("[%1] Page %2")
                   .arg(getAnnotationTypeString(annotation.type))
                   .arg(annotation.pageNumber + 1);

    // Author if available
    if (!annotation.author.isEmpty()) {
        summary += QString(" - %1").arg(annotation.author);
    }

    // Content preview (first 50 characters)
    if (!annotation.content.isEmpty()) {
        QString content = annotation.content;
        if (content.length() > 50) {
            content = content.left(50) + "...";
        }
        summary += QString("\n%1").arg(content);
    }

    return summary;
}
