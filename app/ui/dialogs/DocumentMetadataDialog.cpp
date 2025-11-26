#include "DocumentMetadataDialog.h"
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QLocale>
#include <QMimeData>
#include <QStringConverter>
#include <QStyle>
#include <QTextStream>
#include <stdexcept>
#include "../../managers/I18nManager.h"
#include "../../managers/StyleManager.h"
#include "../widgets/ToastNotification.h"
#include "ElaContentDialog.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"
#include "ElaScrollPageArea.h"
#include "ElaText.h"

DocumentMetadataDialog::DocumentMetadataDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Document Details"));
    setModal(true);

    // Set responsive size constraints
    setMinimumSize(600, 500);  // Minimum size for readability
    resize(750, 600);

    // Set size policy for proper resizing
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setupUI();
    setupConnections();
    applyCurrentTheme();

    // Connect to language change signal (disambiguate overloaded signal)
    connect(&I18nManager::instance(),
            static_cast<void (I18nManager::*)(I18nManager::Language)>(
                &I18nManager::languageChanged),
            this, [this](I18nManager::Language) { retranslateUi(); });
}

void DocumentMetadataDialog::setupUI() {
    StyleManager& styleManager = StyleManager::instance();
    initializeMainLayout(styleManager);
    createBasicInfoSection(styleManager);
    createPropertiesSection(styleManager);
    createSecuritySection(styleManager);
    createActionButtons();
}

void DocumentMetadataDialog::initializeMainLayout(StyleManager& styleManager) {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(
        styleManager.spacingLG(), styleManager.spacingLG(),
        styleManager.spacingLG(), styleManager.spacingLG());
    m_mainLayout->setSpacing(styleManager.spacingMD());

    m_propertiesScrollArea = new QScrollArea(this);
    m_propertiesScrollArea->setWidgetResizable(true);
    m_propertiesScrollArea->setFrameShape(QFrame::NoFrame);
    m_propertiesScrollArea->setSizePolicy(QSizePolicy::Expanding,
                                          QSizePolicy::Expanding);

    m_propertiesContentWidget = new QWidget();
    m_propertiesContentWidget->setSizePolicy(QSizePolicy::Expanding,
                                             QSizePolicy::Preferred);

    m_propertiesContentLayout = new QVBoxLayout(m_propertiesContentWidget);
    m_propertiesContentLayout->setContentsMargins(
        styleManager.spacingSM(), styleManager.spacingSM(),
        styleManager.spacingSM(), styleManager.spacingSM());
    m_propertiesContentLayout->setSpacing(styleManager.spacingLG());
}

void DocumentMetadataDialog::createBasicInfoSection(
    StyleManager& styleManager) {
    m_basicInfoGroup = new ElaScrollPageArea(m_propertiesContentWidget);
    auto* basicInfoVLayout = new QVBoxLayout(m_basicInfoGroup);
    basicInfoVLayout->setContentsMargins(12, 8, 12, 12);

    m_basicInfoTitle = new ElaText(tr("Basic Information"), m_basicInfoGroup);
    m_basicInfoTitle->setTextPixelSize(14);
    basicInfoVLayout->addWidget(m_basicInfoTitle);

    auto* basicInfoContent = new QWidget(m_basicInfoGroup);
    m_basicInfoLayout = new QGridLayout(basicInfoContent);
    m_basicInfoLayout->setContentsMargins(0, styleManager.spacingSM(), 0, 0);
    m_basicInfoLayout->setHorizontalSpacing(styleManager.spacingMD());
    m_basicInfoLayout->setVerticalSpacing(styleManager.spacingSM());
    m_basicInfoLayout->setColumnStretch(1, 1);
    basicInfoVLayout->addWidget(basicInfoContent);

    auto* fileNameLabel = new ElaText(tr("File Name:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(fileNameLabel, 0, 0);
    m_fileNameEdit = new ElaLineEdit(m_basicInfoGroup);
    m_fileNameEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_fileNameEdit, 0, 1);

    auto* filePathLabel = new ElaText(tr("File Path:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(filePathLabel, 1, 0);
    m_filePathEdit = new ElaLineEdit(m_basicInfoGroup);
    m_filePathEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_filePathEdit, 1, 1);

    auto* fileSizeLabel = new ElaText(tr("File Size:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(fileSizeLabel, 2, 0);
    m_fileSizeEdit = new ElaLineEdit(m_basicInfoGroup);
    m_fileSizeEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_fileSizeEdit, 2, 1);

    auto* pageCountLabel = new ElaText(tr("Pages:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(pageCountLabel, 3, 0);
    m_pageCountEdit = new ElaLineEdit(m_basicInfoGroup);
    m_pageCountEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_pageCountEdit, 3, 1);

    auto* pdfVersionLabel = new ElaText(tr("PDF Version:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(pdfVersionLabel, 4, 0);
    m_pdfVersionEdit = new ElaLineEdit(m_basicInfoGroup);
    m_pdfVersionEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_pdfVersionEdit, 4, 1);

    auto* creationDateFileLabel =
        new ElaText(tr("File Created:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(creationDateFileLabel, 5, 0);
    m_creationDateFileEdit = new ElaLineEdit(m_basicInfoGroup);
    m_creationDateFileEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_creationDateFileEdit, 5, 1);

    auto* modificationDateFileLabel =
        new ElaText(tr("File Modified:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(modificationDateFileLabel, 6, 0);
    m_modificationDateFileEdit = new ElaLineEdit(m_basicInfoGroup);
    m_modificationDateFileEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_modificationDateFileEdit, 6, 1);

    m_propertiesContentLayout->addWidget(m_basicInfoGroup);
}

void DocumentMetadataDialog::createPropertiesSection(
    StyleManager& styleManager) {
    m_propertiesGroup = new ElaScrollPageArea(m_propertiesContentWidget);
    auto* propertiesVLayout = new QVBoxLayout(m_propertiesGroup);
    propertiesVLayout->setContentsMargins(12, 8, 12, 12);

    m_propertiesTitle =
        new ElaText(tr("Document Properties"), m_propertiesGroup);
    m_propertiesTitle->setTextPixelSize(14);
    propertiesVLayout->addWidget(m_propertiesTitle);

    auto* propertiesContent = new QWidget(m_propertiesGroup);
    m_propertiesLayout = new QGridLayout(propertiesContent);
    m_propertiesLayout->setContentsMargins(0, styleManager.spacingSM(), 0, 0);
    m_propertiesLayout->setHorizontalSpacing(styleManager.spacingMD());
    m_propertiesLayout->setVerticalSpacing(styleManager.spacingSM());
    m_propertiesLayout->setColumnStretch(1, 1);
    propertiesVLayout->addWidget(propertiesContent);

    auto* titleLabel = new ElaText(tr("Title:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(titleLabel, 0, 0);
    m_titleEdit = new ElaLineEdit(m_propertiesGroup);
    m_titleEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_titleEdit, 0, 1);

    auto* authorLabel = new ElaText(tr("Author:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(authorLabel, 1, 0);
    m_authorEdit = new ElaLineEdit(m_propertiesGroup);
    m_authorEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_authorEdit, 1, 1);

    auto* subjectLabel = new ElaText(tr("Subject:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(subjectLabel, 2, 0);
    m_subjectEdit = new ElaLineEdit(m_propertiesGroup);
    m_subjectEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_subjectEdit, 2, 1);

    auto* keywordsLabel = new ElaText(tr("Keywords:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(keywordsLabel, 3, 0);
    m_keywordsEdit = new QTextEdit(m_propertiesGroup);
    m_keywordsEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_keywordsEdit, 3, 1);

    auto* creatorLabel = new ElaText(tr("Creator:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(creatorLabel, 4, 0);
    m_creatorEdit = new ElaLineEdit(m_propertiesGroup);
    m_creatorEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_creatorEdit, 4, 1);

    auto* producerLabel = new ElaText(tr("Producer:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(producerLabel, 5, 0);
    m_producerEdit = new ElaLineEdit(m_propertiesGroup);
    m_producerEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_producerEdit, 5, 1);

    auto* creationDateLabel = new ElaText(tr("Created:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(creationDateLabel, 6, 0);
    m_creationDateEdit = new ElaLineEdit(m_propertiesGroup);
    m_creationDateEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_creationDateEdit, 6, 1);

    auto* modificationDateLabel =
        new ElaText(tr("Modified:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(modificationDateLabel, 7, 0);
    m_modificationDateEdit = new ElaLineEdit(m_propertiesGroup);
    m_modificationDateEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_modificationDateEdit, 7, 1);

    m_propertiesContentLayout->addWidget(m_propertiesGroup);
}

void DocumentMetadataDialog::createSecuritySection(StyleManager& styleManager) {
    m_securityGroup = new ElaScrollPageArea(m_propertiesContentWidget);
    auto* securityVLayout = new QVBoxLayout(m_securityGroup);
    securityVLayout->setContentsMargins(12, 8, 12, 12);

    m_securityTitle = new ElaText(tr("Security Information"), m_securityGroup);
    m_securityTitle->setTextPixelSize(14);
    securityVLayout->addWidget(m_securityTitle);

    auto* securityContent = new QWidget(m_securityGroup);
    m_securityLayout = new QGridLayout(securityContent);
    m_securityLayout->setContentsMargins(0, styleManager.spacingSM(), 0, 0);
    m_securityLayout->setHorizontalSpacing(styleManager.spacingMD());
    m_securityLayout->setVerticalSpacing(styleManager.spacingSM());
    m_securityLayout->setColumnStretch(1, 1);
    securityVLayout->addWidget(securityContent);

    auto* encryptedLabel = new ElaText(tr("Encrypted:"), m_securityGroup);
    m_securityLayout->addWidget(encryptedLabel, 0, 0);
    m_encryptedEdit = new ElaLineEdit(m_securityGroup);
    m_encryptedEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_encryptedEdit, 0, 1);

    auto* encryptionMethodLabel =
        new ElaText(tr("Encryption Method:"), m_securityGroup);
    m_securityLayout->addWidget(encryptionMethodLabel, 1, 0);
    m_encryptionMethodEdit = new ElaLineEdit(m_securityGroup);
    m_encryptionMethodEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_encryptionMethodEdit, 1, 1);

    auto* extractLabel = new ElaText(tr("Can Extract Text:"), m_securityGroup);
    m_securityLayout->addWidget(extractLabel, 2, 0);
    m_canExtractTextEdit = new ElaLineEdit(m_securityGroup);
    m_canExtractTextEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canExtractTextEdit, 2, 1);

    auto* printLabel = new ElaText(tr("Can Print:"), m_securityGroup);
    m_securityLayout->addWidget(printLabel, 3, 0);
    m_canPrintEdit = new ElaLineEdit(m_securityGroup);
    m_canPrintEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canPrintEdit, 3, 1);

    auto* printHighResLabel =
        new ElaText(tr("Can Print High Resolution:"), m_securityGroup);
    m_securityLayout->addWidget(printHighResLabel, 4, 0);
    m_canPrintHighResEdit = new ElaLineEdit(m_securityGroup);
    m_canPrintHighResEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canPrintHighResEdit, 4, 1);

    auto* modifyLabel = new ElaText(tr("Can Modify:"), m_securityGroup);
    m_securityLayout->addWidget(modifyLabel, 5, 0);
    m_canModifyEdit = new ElaLineEdit(m_securityGroup);
    m_canModifyEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canModifyEdit, 5, 1);

    auto* modifyAnnotationsLabel =
        new ElaText(tr("Can Modify Annotations:"), m_securityGroup);
    m_securityLayout->addWidget(modifyAnnotationsLabel, 6, 0);
    m_canModifyAnnotationsEdit = new ElaLineEdit(m_securityGroup);
    m_canModifyAnnotationsEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canModifyAnnotationsEdit, 6, 1);

    auto* fillFormsLabel = new ElaText(tr("Can Fill Forms:"), m_securityGroup);
    m_securityLayout->addWidget(fillFormsLabel, 7, 0);
    m_canFillFormsEdit = new ElaLineEdit(m_securityGroup);
    m_canFillFormsEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canFillFormsEdit, 7, 1);

    auto* assembleLabel =
        new ElaText(tr("Can Assemble Document:"), m_securityGroup);
    m_securityLayout->addWidget(assembleLabel, 8, 0);
    m_canAssembleEdit = new ElaLineEdit(m_securityGroup);
    m_canAssembleEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canAssembleEdit, 8, 1);

    m_propertiesContentLayout->addWidget(m_securityGroup);
}

void DocumentMetadataDialog::createActionButtons() {
    m_propertiesContentLayout->addStretch();

    if (m_propertiesScrollArea != nullptr) {
        m_propertiesScrollArea->setWidget(m_propertiesContentWidget);
        m_mainLayout->addWidget(m_propertiesScrollArea);
    }

    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();

    m_exportButton = new ElaPushButton(tr("Export Information"));
    m_exportButton->setToolTip(tr("Export document information to text file"));
    m_buttonLayout->addWidget(m_exportButton);

    m_closeButton = new ElaPushButton(tr("Close"));
    m_closeButton->setDefault(true);
    m_buttonLayout->addWidget(m_closeButton);

    m_mainLayout->addLayout(m_buttonLayout);
}

void DocumentMetadataDialog::setupConnections() {
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_exportButton, &QPushButton::clicked, this,
            &DocumentMetadataDialog::exportMetadata);

    // 连接主题变化信号
    connect(&StyleManager::instance(), &StyleManager::themeChanged, this,
            &DocumentMetadataDialog::onThemeChanged);
}

void DocumentMetadataDialog::onThemeChanged() { applyCurrentTheme(); }

void DocumentMetadataDialog::applyCurrentTheme() {
    // 应用StyleManager的样式
    setStyleSheet(StyleManager::instance().getApplicationStyleSheet());
}

void DocumentMetadataDialog::setDocument(Poppler::Document* document,
                                         const QString& filePath) {
    m_currentDocument = document;
    m_currentFilePath = filePath;

    if (document == nullptr || filePath.isEmpty()) {
        clearMetadata();
        return;
    }

    try {
        populateBasicInfo(filePath, document);
        populateDocumentProperties(document);
        populateSecurityInfo(document);
    } catch (const std::exception& e) {
        TOAST_ERROR(this,
                    tr("Error retrieving document metadata: %1").arg(e.what()));
        clearMetadata();
    }
}

void DocumentMetadataDialog::clearMetadata() {
    // 清空基本信息字段
    m_fileNameEdit->clear();
    m_filePathEdit->clear();
    m_fileSizeEdit->clear();
    m_pageCountEdit->clear();
    m_pdfVersionEdit->clear();
    m_creationDateFileEdit->clear();
    m_modificationDateFileEdit->clear();

    // 清空文档属性字段
    m_titleEdit->clear();
    m_authorEdit->clear();
    m_subjectEdit->clear();
    m_keywordsEdit->clear();
    m_creatorEdit->clear();
    m_producerEdit->clear();
    m_creationDateEdit->clear();
    m_modificationDateEdit->clear();

    // 清空安全信息字段
    m_encryptedEdit->clear();
    m_encryptionMethodEdit->clear();
    m_canExtractTextEdit->clear();
    m_canPrintEdit->clear();
    m_canPrintHighResEdit->clear();
    m_canModifyEdit->clear();
    m_canModifyAnnotationsEdit->clear();
    m_canFillFormsEdit->clear();
    m_canAssembleEdit->clear();
}

void DocumentMetadataDialog::populateBasicInfo(const QString& filePath,
                                               Poppler::Document* document) {
    QFileInfo fileInfo(filePath);

    // 文件名
    m_fileNameEdit->setText(fileInfo.fileName());

    // 文件路径
    m_filePathEdit->setText(
        QDir::toNativeSeparators(fileInfo.absoluteFilePath()));

    // 文件大小
    qint64 fileSize = fileInfo.size();
    m_fileSizeEdit->setText(formatFileSize(fileSize));

    // 页数和PDF版本
    if (document != nullptr) {
        int pageCount = document->numPages();
        m_pageCountEdit->setText(QString::number(pageCount));

        // PDF版本
        QString pdfVersion = getPdfVersion(document);
        m_pdfVersionEdit->setText(pdfVersion);
    } else {
        m_pageCountEdit->setText(tr("Unknown"));
        m_pdfVersionEdit->setText(tr("Unknown"));
    }

    // File creation and modification times
    QDateTime creationTime = fileInfo.birthTime();
    if (!creationTime.isValid()) {
        creationTime = fileInfo.metadataChangeTime();
    }
    m_creationDateFileEdit->setText(
        creationTime.isValid()
            ? formatDateTime(creationTime.toString(Qt::ISODate))
            : tr("Unknown"));

    QDateTime modificationTime = fileInfo.lastModified();
    m_modificationDateFileEdit->setText(
        modificationTime.isValid()
            ? formatDateTime(modificationTime.toString(Qt::ISODate))
            : tr("Unknown"));
}

void DocumentMetadataDialog::populateDocumentProperties(
    Poppler::Document* document) {
    if (document == nullptr) {
        return;
    }

    // Get metadata using Poppler::Document's info method
    QString title = document->info("Title");
    m_titleEdit->setText(title.isEmpty() ? tr("Not Set") : title);

    QString author = document->info("Author");
    m_authorEdit->setText(author.isEmpty() ? tr("Not Set") : author);

    QString subject = document->info("Subject");
    m_subjectEdit->setText(subject.isEmpty() ? tr("Not Set") : subject);

    QString keywords = document->info("Keywords");
    m_keywordsEdit->setText(keywords.isEmpty() ? tr("Not Set") : keywords);

    QString creator = document->info("Creator");
    m_creatorEdit->setText(creator.isEmpty() ? tr("Not Set") : creator);

    QString producer = document->info("Producer");
    m_producerEdit->setText(producer.isEmpty() ? tr("Not Set") : producer);

    QString creationDate = document->info("CreationDate");
    m_creationDateEdit->setText(formatDateTime(creationDate));

    QString modificationDate = document->info("ModDate");
    m_modificationDateEdit->setText(formatDateTime(modificationDate));
}

void DocumentMetadataDialog::populateSecurityInfo(Poppler::Document* document) {
    if (document == nullptr) {
        return;
    }

    try {
        // Encryption status
        bool isEncrypted = document->isEncrypted();
        m_encryptedEdit->setText(isEncrypted ? tr("Yes") : tr("No"));

        // Encryption method
        if (isEncrypted) {
            m_encryptionMethodEdit->setText(tr("Standard Encryption"));
        } else {
            m_encryptionMethodEdit->setText(tr("None"));
        }

        // Get document permissions - if document is unlocked, we can check
        // permissions
        bool canExtractText =
            true;  // If we can open the document, we can usually extract text
        bool canPrint = true;         // Default allow printing
        bool canPrintHighRes = true;  // Default allow high-resolution printing
        bool canModify =
            false;  // PDF viewers typically don't allow modification
        bool canModifyAnnotations =
            false;                 // Default don't allow modifying annotations
        bool canFillForms = true;  // Default allow filling forms
        bool canAssemble = false;  // Default don't allow assembling documents

        // If document is encrypted, permissions may be restricted
        if (isEncrypted) {
            // Can set based on actual permission checks here
            canModify = false;
            canModifyAnnotations = false;
            canAssemble = false;
        }

        m_canExtractTextEdit->setText(canExtractText ? tr("Yes") : tr("No"));
        m_canPrintEdit->setText(canPrint ? tr("Yes") : tr("No"));
        m_canPrintHighResEdit->setText(canPrintHighRes ? tr("Yes") : tr("No"));
        m_canModifyEdit->setText(canModify ? tr("Yes") : tr("No"));
        m_canModifyAnnotationsEdit->setText(canModifyAnnotations ? tr("Yes")
                                                                 : tr("No"));
        m_canFillFormsEdit->setText(canFillForms ? tr("Yes") : tr("No"));
        m_canAssembleEdit->setText(canAssemble ? tr("Yes") : tr("No"));

    } catch (const std::exception& e) {
        // If getting security info fails, set to unknown
        m_encryptedEdit->setText(tr("Unknown"));
        m_encryptionMethodEdit->setText(tr("Unknown"));
        m_canExtractTextEdit->setText(tr("Unknown"));
        m_canPrintEdit->setText(tr("Unknown"));
        m_canPrintHighResEdit->setText(tr("Unknown"));
        m_canModifyEdit->setText(tr("Unknown"));
        m_canModifyAnnotationsEdit->setText(tr("Unknown"));
        m_canFillFormsEdit->setText(tr("Unknown"));
        m_canAssembleEdit->setText(tr("Unknown"));
    }
}

QString DocumentMetadataDialog::formatDateTime(const QString& dateTimeStr) {
    if (dateTimeStr.isEmpty()) {
        return tr("Not Set");
    }

    // PDF date format is usually: D:YYYYMMDDHHmmSSOHH'mm'
    // Try to parse different date formats
    QDateTime dateTime;

    // Try ISO format
    dateTime = QDateTime::fromString(dateTimeStr, Qt::ISODate);
    if (dateTime.isValid()) {
        return QLocale::system().toString(dateTime, QLocale::ShortFormat);
    }

    // Try PDF format D:YYYYMMDDHHmmSS
    if (dateTimeStr.startsWith("D:") && dateTimeStr.length() >= 16) {
        QString cleanDate = dateTimeStr.mid(2, 14);  // Get YYYYMMDDHHMMSS part
        dateTime = QDateTime::fromString(cleanDate, "yyyyMMddhhmmss");
        if (dateTime.isValid()) {
            return QLocale::system().toString(dateTime, QLocale::ShortFormat);
        }
    }

    // If unable to parse, return original string
    return dateTimeStr;
}

QString DocumentMetadataDialog::formatFileSize(qint64 bytes) {
    if (bytes < 0) {
        return tr("Unknown");
    }

    constexpr qint64 KB_VALUE = 1024;
    constexpr qint64 MB_VALUE = KB_VALUE * 1024;
    constexpr qint64 GB_VALUE = MB_VALUE * 1024;

    if (bytes >= GB_VALUE) {
        double sizeInGigabytes =
            static_cast<double>(bytes) / static_cast<double>(GB_VALUE);
        return QString("%1 GB").arg(QString::number(sizeInGigabytes, 'f', 2));
    }
    if (bytes >= MB_VALUE) {
        double sizeInMegabytes =
            static_cast<double>(bytes) / static_cast<double>(MB_VALUE);
        return QString("%1 MB").arg(QString::number(sizeInMegabytes, 'f', 2));
    }
    if (bytes >= KB_VALUE) {
        double sizeInKilobytes =
            static_cast<double>(bytes) / static_cast<double>(KB_VALUE);
        return QString("%1 KB").arg(QString::number(sizeInKilobytes, 'f', 1));
    }
    return tr("%1 bytes").arg(bytes);
}

QString DocumentMetadataDialog::getPdfVersion(Poppler::Document* document) {
    if (!document) {
        return tr("Unknown");
    }

    try {
        Poppler::Document::PdfVersion version = document->getPdfVersion();
        return QString("PDF %1.%2").arg(version.major).arg(version.minor);
    } catch (...) {
        return tr("Unknown");
    }
}

void DocumentMetadataDialog::exportMetadata() {
    if (m_currentFilePath.isEmpty()) {
        auto* dialog = new ElaContentDialog(this);
        dialog->setWindowTitle(tr("Export Error"));
        auto* w = new QWidget(dialog);
        auto* l = new QVBoxLayout(w);
        l->addWidget(new ElaText(tr("No document information to export"), w));
        dialog->setCentralWidget(w);
        dialog->setLeftButtonText(QString());
        dialog->setMiddleButtonText(QString());
        dialog->setRightButtonText(tr("OK"));
        connect(dialog, &ElaContentDialog::rightButtonClicked, dialog,
                &ElaContentDialog::close);
        dialog->exec();
        dialog->deleteLater();
        return;
    }

    // Get suggested file name
    QFileInfo fileInfo(m_currentFilePath);
    QString suggestedName = fileInfo.baseName() + "_metadata.txt";

    QString fileName =
        QFileDialog::getSaveFileName(this, tr("Export Document Information"),
                                     QDir::homePath() + "/" + suggestedName,
                                     tr("Text Files (*.txt);;All Files (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    try {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw std::runtime_error(tr("Cannot create file: %1")
                                         .arg(file.errorString())
                                         .toStdString());
        }

        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);

        // Write document information
        out << tr("PDF Document Information Report") << "\n";
        out << QString("=").repeated(50) << "\n\n";

        // Basic information
        out << tr("Basic Information:") << "\n";
        out << tr("File Name: %1").arg(m_fileNameEdit->text()) << "\n";
        out << tr("File Path: %1").arg(m_filePathEdit->text()) << "\n";
        out << tr("File Size: %1").arg(m_fileSizeEdit->text()) << "\n";
        out << tr("Pages: %1").arg(m_pageCountEdit->text()) << "\n";
        out << tr("PDF Version: %1").arg(m_pdfVersionEdit->text()) << "\n";
        out << tr("File Created: %1").arg(m_creationDateFileEdit->text())
            << "\n";
        out << tr("File Modified: %1").arg(m_modificationDateFileEdit->text())
            << "\n\n";

        // Document properties
        out << tr("Document Properties:") << "\n";
        out << tr("Title: %1").arg(m_titleEdit->text()) << "\n";
        out << tr("Author: %1").arg(m_authorEdit->text()) << "\n";
        out << tr("Subject: %1").arg(m_subjectEdit->text()) << "\n";
        out << tr("Keywords: %1").arg(m_keywordsEdit->toPlainText()) << "\n";
        out << tr("Creator: %1").arg(m_creatorEdit->text()) << "\n";
        out << tr("Producer: %1").arg(m_producerEdit->text()) << "\n";
        out << tr("Created: %1").arg(m_creationDateEdit->text()) << "\n";
        out << tr("Modified: %1").arg(m_modificationDateEdit->text()) << "\n\n";

        // Security information
        out << tr("Security Information:") << "\n";
        out << tr("Encrypted: %1").arg(m_encryptedEdit->text()) << "\n";
        out << tr("Encryption Method: %1").arg(m_encryptionMethodEdit->text())
            << "\n";
        out << tr("Can Extract Text: %1").arg(m_canExtractTextEdit->text())
            << "\n";
        out << tr("Can Print: %1").arg(m_canPrintEdit->text()) << "\n";
        out << tr("Can Print High Resolution: %1")
                   .arg(m_canPrintHighResEdit->text())
            << "\n";
        out << tr("Can Modify: %1").arg(m_canModifyEdit->text()) << "\n";
        out << tr("Can Modify Annotations: %1")
                   .arg(m_canModifyAnnotationsEdit->text())
            << "\n";
        out << tr("Can Fill Forms: %1").arg(m_canFillFormsEdit->text()) << "\n";
        out << tr("Can Assemble Document: %1").arg(m_canAssembleEdit->text())
            << "\n\n";

        // Export information
        out << QString("-").repeated(50) << "\n";
        out << tr("Export Time: %1")
                   .arg(QDateTime::currentDateTime().toString())
            << "\n";
        out << tr("Export Tool: SAST Readium PDF Reader") << "\n";

        file.close();

        TOAST_SUCCESS(this,
                      tr("Document information successfully exported to: %1")
                          .arg(QFileInfo(fileName).fileName()));

    } catch (const std::exception& e) {
        auto* dialog = new ElaContentDialog(this);
        dialog->setWindowTitle(tr("Export Error"));
        auto* w = new QWidget(dialog);
        auto* l = new QVBoxLayout(w);
        l->addWidget(new ElaText(
            tr("Error exporting document information: %1").arg(e.what()), w));
        dialog->setCentralWidget(w);
        dialog->setLeftButtonText(QString());
        dialog->setMiddleButtonText(QString());
        dialog->setRightButtonText(tr("OK"));
        connect(dialog, &ElaContentDialog::rightButtonClicked, dialog,
                &ElaContentDialog::close);
        dialog->exec();
        dialog->deleteLater();
    }
}

void DocumentMetadataDialog::retranslateUi() {
    // Update window title
    setWindowTitle(tr("Document Details"));

    // Update basic info section title
    if (m_basicInfoTitle) {
        m_basicInfoTitle->setText(tr("Basic Information"));
    }

    // Update properties section title
    if (m_propertiesTitle) {
        m_propertiesTitle->setText(tr("Document Properties"));
    }

    // Update security section title
    if (m_securityTitle) {
        m_securityTitle->setText(tr("Security Information"));
    }

    // Update buttons
    if (m_exportButton) {
        m_exportButton->setText(tr("Export Information"));
        m_exportButton->setToolTip(
            tr("Export document information to text file"));
    }
    if (m_closeButton) {
        m_closeButton->setText(tr("Close"));
    }

    // Note: We don't retranslate the data values themselves (file names, dates,
    // etc.) as they are actual data, not UI labels. The labels are created in
    // the create*Section methods and would need to be stored as member
    // variables to be retranslated here. For now, we only update the group box
    // titles and buttons which are the main translatable UI elements.
}

void DocumentMetadataDialog::changeEvent(QEvent* event) {
    QDialog::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
}
