#include "PropertiesPanel.h"

// ElaWidgetTools
#include "ElaScrollArea.h"
#include "ElaText.h"

// Qt
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QDateTime>
#include <QFileInfo>
#include <QFrame>

// Logging
#include "logging/SimpleLogging.h"

// ============================================================================
// Constructor and Destructor
// ============================================================================

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QWidget(parent),
      m_document(nullptr),
      m_scrollArea(nullptr),
      m_contentWidget(nullptr),
      m_mainLayout(nullptr) {
    SLOG_INFO("PropertiesPanel: Constructor started");

    setupUi();
    clearDocument();

    SLOG_INFO("PropertiesPanel: Constructor completed");
}

PropertiesPanel::~PropertiesPanel() {
    SLOG_INFO("PropertiesPanel: Destructor called");
}

// ============================================================================
// UI Setup
// ============================================================================

void PropertiesPanel::setupUi() {
    // Main layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Scroll area for content
    m_scrollArea = new ElaScrollArea(this);
    m_scrollArea->setWidgetResizable(true);

    // Content widget
    m_contentWidget = new QWidget(this);
    m_mainLayout = new QVBoxLayout(m_contentWidget);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    m_mainLayout->setSpacing(10);

    // Create sections
    createMetadataSection();
    addSeparator();
    createStatisticsSection();
    addSeparator();
    createSecuritySection();

    // Add stretch at the end
    m_mainLayout->addStretch();

    m_scrollArea->setWidget(m_contentWidget);
    layout->addWidget(m_scrollArea);
}

void PropertiesPanel::createMetadataSection() {
    // Section title
    ElaText* sectionTitle = createSectionLabel(tr("Document Metadata"));
    sectionTitle->setTextPixelSize(16);
    m_mainLayout->addWidget(sectionTitle);
    m_mainLayout->addSpacing(5);

    // Title
    m_titleLabel = createSectionLabel(tr("Title:"));
    m_titleValue = createValueLabel();
    m_mainLayout->addWidget(m_titleLabel);
    m_mainLayout->addWidget(m_titleValue);
    m_mainLayout->addSpacing(5);

    // Author
    m_authorLabel = createSectionLabel(tr("Author:"));
    m_authorValue = createValueLabel();
    m_mainLayout->addWidget(m_authorLabel);
    m_mainLayout->addWidget(m_authorValue);
    m_mainLayout->addSpacing(5);

    // Subject
    m_subjectLabel = createSectionLabel(tr("Subject:"));
    m_subjectValue = createValueLabel();
    m_mainLayout->addWidget(m_subjectLabel);
    m_mainLayout->addWidget(m_subjectValue);
    m_mainLayout->addSpacing(5);

    // Keywords
    m_keywordsLabel = createSectionLabel(tr("Keywords:"));
    m_keywordsValue = createValueLabel();
    m_keywordsValue->setWordWrap(true);
    m_mainLayout->addWidget(m_keywordsLabel);
    m_mainLayout->addWidget(m_keywordsValue);
    m_mainLayout->addSpacing(5);

    // Creator
    m_creatorLabel = createSectionLabel(tr("Creator:"));
    m_creatorValue = createValueLabel();
    m_mainLayout->addWidget(m_creatorLabel);
    m_mainLayout->addWidget(m_creatorValue);
    m_mainLayout->addSpacing(5);

    // Producer
    m_producerLabel = createSectionLabel(tr("Producer:"));
    m_producerValue = createValueLabel();
    m_mainLayout->addWidget(m_producerLabel);
    m_mainLayout->addWidget(m_producerValue);
    m_mainLayout->addSpacing(5);

    // Creation Date
    m_creationDateLabel = createSectionLabel(tr("Creation Date:"));
    m_creationDateValue = createValueLabel();
    m_mainLayout->addWidget(m_creationDateLabel);
    m_mainLayout->addWidget(m_creationDateValue);
    m_mainLayout->addSpacing(5);

    // Modification Date
    m_modificationDateLabel = createSectionLabel(tr("Modification Date:"));
    m_modificationDateValue = createValueLabel();
    m_mainLayout->addWidget(m_modificationDateLabel);
    m_mainLayout->addWidget(m_modificationDateValue);
}

void PropertiesPanel::createStatisticsSection() {
    // Section title
    ElaText* sectionTitle = createSectionLabel(tr("Document Statistics"));
    sectionTitle->setTextPixelSize(16);
    m_mainLayout->addWidget(sectionTitle);
    m_mainLayout->addSpacing(5);

    // Page Count
    m_pageCountLabel = createSectionLabel(tr("Page Count:"));
    m_pageCountValue = createValueLabel();
    m_mainLayout->addWidget(m_pageCountLabel);
    m_mainLayout->addWidget(m_pageCountValue);
    m_mainLayout->addSpacing(5);

    // File Size
    m_fileSizeLabel = createSectionLabel(tr("File Size:"));
    m_fileSizeValue = createValueLabel();
    m_mainLayout->addWidget(m_fileSizeLabel);
    m_mainLayout->addWidget(m_fileSizeValue);
    m_mainLayout->addSpacing(5);

    // PDF Version
    m_pdfVersionLabel = createSectionLabel(tr("PDF Version:"));
    m_pdfVersionValue = createValueLabel();
    m_mainLayout->addWidget(m_pdfVersionLabel);
    m_mainLayout->addWidget(m_pdfVersionValue);
}

void PropertiesPanel::createSecuritySection() {
    // Section title
    ElaText* sectionTitle = createSectionLabel(tr("Security Information"));
    sectionTitle->setTextPixelSize(16);
    m_mainLayout->addWidget(sectionTitle);
    m_mainLayout->addSpacing(5);

    // Encrypted
    m_encryptedLabel = createSectionLabel(tr("Encrypted:"));
    m_encryptedValue = createValueLabel();
    m_mainLayout->addWidget(m_encryptedLabel);
    m_mainLayout->addWidget(m_encryptedValue);
    m_mainLayout->addSpacing(5);

    // Linearized
    m_linearizedLabel = createSectionLabel(tr("Linearized (Fast Web View):"));
    m_linearizedValue = createValueLabel();
    m_mainLayout->addWidget(m_linearizedLabel);
    m_mainLayout->addWidget(m_linearizedValue);
}

// ============================================================================
// Document Management
// ============================================================================

void PropertiesPanel::setDocument(Poppler::Document* document,
                                  const QString& filePath) {
    if (!document) {
        SLOG_WARNING("PropertiesPanel::setDocument: Null document provided");
        clearDocument();
        return;
    }

    SLOG_INFO("PropertiesPanel: Setting document: " + filePath);

    m_document = document;
    m_filePath = filePath;

    // Update all sections
    updateMetadata();
    updateStatistics();
    updateSecurity();

    emit documentChanged();
}

void PropertiesPanel::clearDocument() {
    SLOG_INFO("PropertiesPanel: Clearing document");

    m_document = nullptr;
    m_filePath.clear();

    // Clear all values
    m_titleValue->setText(tr("No document loaded"));
    m_authorValue->setText(tr("—"));
    m_subjectValue->setText(tr("—"));
    m_keywordsValue->setText(tr("—"));
    m_creatorValue->setText(tr("—"));
    m_producerValue->setText(tr("—"));
    m_creationDateValue->setText(tr("—"));
    m_modificationDateValue->setText(tr("—"));

    m_pageCountValue->setText(tr("—"));
    m_fileSizeValue->setText(tr("—"));
    m_pdfVersionValue->setText(tr("—"));

    m_encryptedValue->setText(tr("—"));
    m_linearizedValue->setText(tr("—"));
}

bool PropertiesPanel::hasDocument() const { return m_document != nullptr; }

// ============================================================================
// Update Methods
// ============================================================================

void PropertiesPanel::updateMetadata() {
    if (!m_document) {
        return;
    }

    try {
        // Title
        QString title = m_document->info("Title");
        m_titleValue->setText(title.isEmpty() ? tr("Untitled") : title);

        // Author
        QString author = m_document->info("Author");
        m_authorValue->setText(author.isEmpty() ? tr("Unknown") : author);

        // Subject
        QString subject = m_document->info("Subject");
        m_subjectValue->setText(subject.isEmpty() ? tr("None") : subject);

        // Keywords
        QString keywords = m_document->info("Keywords");
        m_keywordsValue->setText(keywords.isEmpty() ? tr("None") : keywords);

        // Creator
        QString creator = m_document->info("Creator");
        m_creatorValue->setText(creator.isEmpty() ? tr("Unknown") : creator);

        // Producer
        QString producer = m_document->info("Producer");
        m_producerValue->setText(producer.isEmpty() ? tr("Unknown") : producer);

        // Creation Date
        QString creationDateStr = m_document->info("CreationDate");
        QDateTime creationDate =
            QDateTime::fromString(creationDateStr, Qt::ISODate);
        m_creationDateValue->setText(formatDateTime(creationDate));

        // Modification Date
        QString modDateStr = m_document->info("ModDate");
        QDateTime modDate = QDateTime::fromString(modDateStr, Qt::ISODate);
        m_modificationDateValue->setText(formatDateTime(modDate));

        SLOG_INFO("PropertiesPanel: Metadata updated successfully");

    } catch (const std::exception& e) {
        SLOG_ERROR("PropertiesPanel: Error updating metadata: " +
                   QString(e.what()));
        m_titleValue->setText(tr("Error loading metadata"));
    }
}

void PropertiesPanel::updateStatistics() {
    if (!m_document) {
        return;
    }

    try {
        // Page Count
        int pageCount = m_document->numPages();
        m_pageCountValue->setText(QString::number(pageCount));

        // File Size
        QFileInfo fileInfo(m_filePath);
        if (fileInfo.exists()) {
            qint64 fileSize = fileInfo.size();
            m_fileSizeValue->setText(formatFileSize(fileSize));
        } else {
            m_fileSizeValue->setText(tr("Unknown"));
        }

        // PDF Version
        Poppler::Document::PdfVersion version = m_document->getPdfVersion();
        m_pdfVersionValue->setText(
            QString("PDF %1.%2").arg(version.major).arg(version.minor));

        SLOG_INFO("PropertiesPanel: Statistics updated successfully");

    } catch (const std::exception& e) {
        SLOG_ERROR("PropertiesPanel: Error updating statistics: " +
                   QString(e.what()));
        m_pageCountValue->setText(tr("Error"));
    }
}

void PropertiesPanel::updateSecurity() {
    if (!m_document) {
        return;
    }

    try {
        // Encrypted
        bool isEncrypted = m_document->isEncrypted();
        m_encryptedValue->setText(isEncrypted ? tr("Yes") : tr("No"));

        // Linearized
        bool isLinearized = m_document->isLinearized();
        m_linearizedValue->setText(isLinearized ? tr("Yes") : tr("No"));

        SLOG_INFO("PropertiesPanel: Security information updated successfully");

    } catch (const std::exception& e) {
        SLOG_ERROR("PropertiesPanel: Error updating security: " +
                   QString(e.what()));
        m_encryptedValue->setText(tr("Error"));
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

QString PropertiesPanel::formatFileSize(qint64 bytes) const {
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;

    if (bytes >= GB) {
        return QString("%1 GB").arg(bytes / static_cast<double>(GB), 0, 'f', 2);
    } else if (bytes >= MB) {
        return QString("%1 MB").arg(bytes / static_cast<double>(MB), 0, 'f', 2);
    } else if (bytes >= KB) {
        return QString("%1 KB").arg(bytes / static_cast<double>(KB), 0, 'f', 2);
    } else {
        return QString("%1 bytes").arg(bytes);
    }
}

QString PropertiesPanel::formatDateTime(const QDateTime& dateTime) const {
    if (!dateTime.isValid()) {
        return tr("Unknown");
    }

    return dateTime.toString("yyyy-MM-dd hh:mm:ss");
}

ElaText* PropertiesPanel::createValueLabel(const QString& text) {
    ElaText* label = new ElaText(text, this);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setStyleSheet("ElaText { color: #666666; padding-left: 10px; }");
    return label;
}

ElaText* PropertiesPanel::createSectionLabel(const QString& text) {
    ElaText* label = new ElaText(text, this);
    label->setTextPixelSize(13);
    return label;
}

void PropertiesPanel::addSeparator() {
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("QFrame { color: #E0E0E0; }");
    m_mainLayout->addWidget(separator);
    m_mainLayout->addSpacing(10);
}
