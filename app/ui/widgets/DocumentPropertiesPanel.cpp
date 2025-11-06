#include "DocumentPropertiesPanel.h"
#include <QDateTime>
#include <QEvent>
#include <QFileInfo>
#include <QLocale>
#include "../../managers/StyleManager.h"

// ElaWidgetTools
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaText.h"

DocumentPropertiesPanel::DocumentPropertiesPanel(QWidget* parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_scrollArea(nullptr),
      m_contentWidget(nullptr),
      m_contentLayout(nullptr),
      m_fileInfoGroup(nullptr),
      m_fileInfoLayout(nullptr),
      m_fileNameField(nullptr),
      m_fileSizeField(nullptr),
      m_pageCountField(nullptr),
      m_pdfVersionField(nullptr),
      m_documentInfoGroup(nullptr),
      m_documentInfoLayout(nullptr),
      m_titleField(nullptr),
      m_authorField(nullptr),
      m_subjectField(nullptr),
      m_creatorField(nullptr),
      m_datesGroup(nullptr),
      m_datesLayout(nullptr),
      m_creationDateField(nullptr),
      m_modificationDateField(nullptr),
      m_viewFullDetailsButton(nullptr),
      m_separatorLine(nullptr),
      m_currentDocument(nullptr),
      m_currentFilePath() {
    setupUI();
    setupConnections();
    applyTheme();
}

void DocumentPropertiesPanel::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Create scroll area for content
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(8, 8, 8, 8);
    m_contentLayout->setSpacing(12);

    // File Information Group
    m_fileInfoGroup = new QGroupBox(tr("File Information"));
    m_fileInfoLayout = new QFormLayout(m_fileInfoGroup);
    m_fileInfoLayout->setSpacing(6);
    m_fileInfoLayout->setContentsMargins(8, 12, 8, 8);

    m_fileNameField = new ElaLineEdit();
    m_fileNameField->setReadOnly(true);
    m_fileNameField->setFrame(false);
    m_fileInfoLayout->addRow(new ElaText(tr("File:"), m_fileInfoGroup),
                             m_fileNameField);

    m_fileSizeField = new ElaLineEdit();
    m_fileSizeField->setReadOnly(true);
    m_fileSizeField->setFrame(false);
    m_fileInfoLayout->addRow(new ElaText(tr("Size:"), m_fileInfoGroup),
                             m_fileSizeField);

    m_pageCountField = new ElaLineEdit();
    m_pageCountField->setReadOnly(true);
    m_pageCountField->setFrame(false);
    m_fileInfoLayout->addRow(new ElaText(tr("Pages:"), m_fileInfoGroup),
                             m_pageCountField);

    m_pdfVersionField = new ElaLineEdit();
    m_pdfVersionField->setReadOnly(true);
    m_pdfVersionField->setFrame(false);
    m_fileInfoLayout->addRow(new ElaText(tr("Version:"), m_fileInfoGroup),
                             m_pdfVersionField);

    m_contentLayout->addWidget(m_fileInfoGroup);

    // Document Information Group
    m_documentInfoGroup = new QGroupBox(tr("Document Information"));
    m_documentInfoLayout = new QFormLayout(m_documentInfoGroup);
    m_documentInfoLayout->setSpacing(6);
    m_documentInfoLayout->setContentsMargins(8, 12, 8, 8);

    m_titleField = new ElaLineEdit();
    m_titleField->setReadOnly(true);
    m_titleField->setFrame(false);
    m_documentInfoLayout->addRow(new ElaText(tr("Title:"), m_documentInfoGroup),
                                 m_titleField);

    m_authorField = new ElaLineEdit();
    m_authorField->setReadOnly(true);
    m_authorField->setFrame(false);
    m_documentInfoLayout->addRow(
        new ElaText(tr("Author:"), m_documentInfoGroup), m_authorField);

    m_subjectField = new ElaLineEdit();
    m_subjectField->setReadOnly(true);
    m_subjectField->setFrame(false);
    m_documentInfoLayout->addRow(
        new ElaText(tr("Subject:"), m_documentInfoGroup), m_subjectField);

    m_creatorField = new ElaLineEdit();
    m_creatorField->setReadOnly(true);
    m_creatorField->setFrame(false);
    m_documentInfoLayout->addRow(
        new ElaText(tr("Creator:"), m_documentInfoGroup), m_creatorField);

    m_contentLayout->addWidget(m_documentInfoGroup);

    // Dates Group
    m_datesGroup = new QGroupBox(tr("Dates"));
    m_datesLayout = new QFormLayout(m_datesGroup);
    m_datesLayout->setSpacing(6);
    m_datesLayout->setContentsMargins(8, 12, 8, 8);

    m_creationDateField = new ElaLineEdit();
    m_creationDateField->setReadOnly(true);
    m_creationDateField->setFrame(false);
    m_datesLayout->addRow(new ElaText(tr("Created:"), m_datesGroup),
                          m_creationDateField);

    m_modificationDateField = new ElaLineEdit();
    m_modificationDateField->setReadOnly(true);
    m_modificationDateField->setFrame(false);
    m_datesLayout->addRow(new ElaText(tr("Modified:"), m_datesGroup),
                          m_modificationDateField);

    m_contentLayout->addWidget(m_datesGroup);

    // Add stretch to push content to top
    m_contentLayout->addStretch();

    // Separator line
    m_separatorLine = new QFrame();
    m_separatorLine->setFrameShape(QFrame::HLine);
    m_separatorLine->setFrameShadow(QFrame::Sunken);
    m_contentLayout->addWidget(m_separatorLine);

    // View Full Details button
    m_viewFullDetailsButton = new ElaPushButton(tr("View Full Details..."));
    m_viewFullDetailsButton->setEnabled(false);
    m_contentLayout->addWidget(m_viewFullDetailsButton);

    m_scrollArea->setWidget(m_contentWidget);
    m_mainLayout->addWidget(m_scrollArea);

    // Set initial state
    clearProperties();
}

void DocumentPropertiesPanel::setupConnections() {
    connect(m_viewFullDetailsButton, &QPushButton::clicked, this, [this]() {
        if (m_currentDocument && !m_currentFilePath.isEmpty()) {
            emit viewFullDetailsRequested(m_currentDocument, m_currentFilePath);
        }
    });

    // Connect to theme changes
    connect(&StyleManager::instance(), &StyleManager::themeChanged, this,
            [this](Theme) { applyTheme(); });
}

void DocumentPropertiesPanel::setDocument(Poppler::Document* document,
                                          const QString& filePath) {
    m_currentDocument = document;
    m_currentFilePath = filePath;

    if (!document || filePath.isEmpty()) {
        clearProperties();
        return;
    }

    QFileInfo fileInfo(filePath);

    // Update file information
    updatePropertyField(m_fileNameField, fileInfo.fileName());
    updatePropertyField(m_fileSizeField, formatFileSize(fileInfo.size()));
    updatePropertyField(m_pageCountField,
                        QString::number(document->numPages()));
    updatePropertyField(m_pdfVersionField, getPdfVersion(document));

    // Update document information
    updatePropertyField(m_titleField, document->info("Title"));
    updatePropertyField(m_authorField, document->info("Author"));
    updatePropertyField(m_subjectField, document->info("Subject"));
    updatePropertyField(m_creatorField, document->info("Creator"));

    // Update dates
    QDateTime creationDate = document->date("CreationDate");
    QDateTime modificationDate = document->date("ModDate");

    updatePropertyField(m_creationDateField, creationDate.isValid()
                                                 ? formatDateTime(creationDate)
                                                 : tr("N/A"));
    updatePropertyField(m_modificationDateField,
                        modificationDate.isValid()
                            ? formatDateTime(modificationDate)
                            : tr("N/A"));

    // Enable the full details button
    m_viewFullDetailsButton->setEnabled(true);
}

void DocumentPropertiesPanel::clearProperties() {
    m_currentDocument = nullptr;
    m_currentFilePath.clear();

    updatePropertyField(m_fileNameField, tr("No document loaded"));
    updatePropertyField(m_fileSizeField, tr("N/A"));
    updatePropertyField(m_pageCountField, tr("N/A"));
    updatePropertyField(m_pdfVersionField, tr("N/A"));
    updatePropertyField(m_titleField, tr("N/A"));
    updatePropertyField(m_authorField, tr("N/A"));
    updatePropertyField(m_subjectField, tr("N/A"));
    updatePropertyField(m_creatorField, tr("N/A"));
    updatePropertyField(m_creationDateField, tr("N/A"));
    updatePropertyField(m_modificationDateField, tr("N/A"));

    m_viewFullDetailsButton->setEnabled(false);
}

void DocumentPropertiesPanel::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}

void DocumentPropertiesPanel::retranslateUi() {
    m_fileInfoGroup->setTitle(tr("File Information"));
    m_documentInfoGroup->setTitle(tr("Document Information"));
    m_datesGroup->setTitle(tr("Dates"));
    m_viewFullDetailsButton->setText(tr("View Full Details..."));

    // Re-apply current document to update labels
    if (m_currentDocument && !m_currentFilePath.isEmpty()) {
        Poppler::Document* doc = m_currentDocument;
        QString path = m_currentFilePath;
        setDocument(doc, path);
    }
}

void DocumentPropertiesPanel::applyTheme() {
    // Apply theme-aware styling using StyleManager colors
    QString readOnlyStyle = QString(
                                "QLineEdit[readOnly=\"true\"] {"
                                "   background: transparent;"
                                "   border: none;"
                                "   color: %1;"
                                "   padding: 2px;"
                                "}")
                                .arg(STYLE.textColor().name());

    setStyleSheet(readOnlyStyle);
}

void DocumentPropertiesPanel::updatePropertyField(ElaLineEdit* field,
                                                  const QString& value) {
    if (field) {
        field->setText(value.isEmpty() ? tr("N/A") : value);
        field->setCursorPosition(0);  // Scroll to beginning
    }
}

QString DocumentPropertiesPanel::formatFileSize(qint64 bytes) {
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

QString DocumentPropertiesPanel::formatDateTime(const QDateTime& dateTime) {
    if (!dateTime.isValid()) {
        return tr("N/A");
    }
    return QLocale::system().toString(dateTime, QLocale::ShortFormat);
}

QString DocumentPropertiesPanel::getPdfVersion(Poppler::Document* document) {
    if (!document) {
        return tr("N/A");
    }

    Poppler::Document::PdfVersion version = document->getPdfVersion();
    return QString("PDF %1.%2").arg(version.major).arg(version.minor);
}
