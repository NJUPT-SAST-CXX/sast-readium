#include "DocumentMetadataDialog.h"
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QLocale>
#include <QMessageBox>
#include <QMimeData>
#include <QStringConverter>
#include <QStyle>
#include <QTextStream>
#include <stdexcept>
#include "../../managers/StyleManager.h"
#include "../widgets/ToastNotification.h"

DocumentMetadataDialog::DocumentMetadataDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("📄 文档详细信息"));
    setModal(true);

    // Set responsive size constraints
    setMinimumSize(600, 500);  // Minimum size for readability
    resize(750, 600);

    // Set size policy for proper resizing
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setupUI();
    setupConnections();
    applyCurrentTheme();
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
    m_basicInfoGroup = new QGroupBox(tr("基本信息"), m_propertiesContentWidget);
    m_basicInfoLayout = new QGridLayout(m_basicInfoGroup);
    m_basicInfoLayout->setContentsMargins(
        styleManager.spacingMD(), styleManager.spacingLG(),
        styleManager.spacingMD(), styleManager.spacingMD());
    m_basicInfoLayout->setHorizontalSpacing(styleManager.spacingMD());
    m_basicInfoLayout->setVerticalSpacing(styleManager.spacingSM());
    m_basicInfoLayout->setColumnStretch(1, 1);

    auto* fileNameLabel = new QLabel(tr("文件名:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(fileNameLabel, 0, 0);
    m_fileNameEdit = new QLineEdit(m_basicInfoGroup);
    m_fileNameEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_fileNameEdit, 0, 1);

    auto* filePathLabel = new QLabel(tr("文件路径:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(filePathLabel, 1, 0);
    m_filePathEdit = new QLineEdit(m_basicInfoGroup);
    m_filePathEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_filePathEdit, 1, 1);

    auto* fileSizeLabel = new QLabel(tr("文件大小:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(fileSizeLabel, 2, 0);
    m_fileSizeEdit = new QLineEdit(m_basicInfoGroup);
    m_fileSizeEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_fileSizeEdit, 2, 1);

    auto* pageCountLabel = new QLabel(tr("页数:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(pageCountLabel, 3, 0);
    m_pageCountEdit = new QLineEdit(m_basicInfoGroup);
    m_pageCountEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_pageCountEdit, 3, 1);

    auto* pdfVersionLabel = new QLabel(tr("PDF版本:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(pdfVersionLabel, 4, 0);
    m_pdfVersionEdit = new QLineEdit(m_basicInfoGroup);
    m_pdfVersionEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_pdfVersionEdit, 4, 1);

    auto* creationDateFileLabel =
        new QLabel(tr("文件创建时间:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(creationDateFileLabel, 5, 0);
    m_creationDateFileEdit = new QLineEdit(m_basicInfoGroup);
    m_creationDateFileEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_creationDateFileEdit, 5, 1);

    auto* modificationDateFileLabel =
        new QLabel(tr("文件修改时间:"), m_basicInfoGroup);
    m_basicInfoLayout->addWidget(modificationDateFileLabel, 6, 0);
    m_modificationDateFileEdit = new QLineEdit(m_basicInfoGroup);
    m_modificationDateFileEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_modificationDateFileEdit, 6, 1);

    m_propertiesContentLayout->addWidget(m_basicInfoGroup);
}

void DocumentMetadataDialog::createPropertiesSection(
    StyleManager& styleManager) {
    m_propertiesGroup =
        new QGroupBox(tr("文档属性"), m_propertiesContentWidget);
    m_propertiesLayout = new QGridLayout(m_propertiesGroup);
    m_propertiesLayout->setContentsMargins(
        styleManager.spacingMD(), styleManager.spacingLG(),
        styleManager.spacingMD(), styleManager.spacingMD());
    m_propertiesLayout->setHorizontalSpacing(styleManager.spacingMD());
    m_propertiesLayout->setVerticalSpacing(styleManager.spacingSM());
    m_propertiesLayout->setColumnStretch(1, 1);

    auto* titleLabel = new QLabel(tr("标题:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(titleLabel, 0, 0);
    m_titleEdit = new QLineEdit(m_propertiesGroup);
    m_titleEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_titleEdit, 0, 1);

    auto* authorLabel = new QLabel(tr("作者:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(authorLabel, 1, 0);
    m_authorEdit = new QLineEdit(m_propertiesGroup);
    m_authorEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_authorEdit, 1, 1);

    auto* subjectLabel = new QLabel(tr("主题:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(subjectLabel, 2, 0);
    m_subjectEdit = new QLineEdit(m_propertiesGroup);
    m_subjectEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_subjectEdit, 2, 1);

    auto* keywordsLabel = new QLabel(tr("关键词:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(keywordsLabel, 3, 0);
    m_keywordsEdit = new QTextEdit(m_propertiesGroup);
    m_keywordsEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_keywordsEdit, 3, 1);

    auto* creatorLabel = new QLabel(tr("创建者:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(creatorLabel, 4, 0);
    m_creatorEdit = new QLineEdit(m_propertiesGroup);
    m_creatorEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_creatorEdit, 4, 1);

    auto* producerLabel = new QLabel(tr("生成者:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(producerLabel, 5, 0);
    m_producerEdit = new QLineEdit(m_propertiesGroup);
    m_producerEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_producerEdit, 5, 1);

    auto* creationDateLabel = new QLabel(tr("创建时间:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(creationDateLabel, 6, 0);
    m_creationDateEdit = new QLineEdit(m_propertiesGroup);
    m_creationDateEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_creationDateEdit, 6, 1);

    auto* modificationDateLabel =
        new QLabel(tr("修改时间:"), m_propertiesGroup);
    m_propertiesLayout->addWidget(modificationDateLabel, 7, 0);
    m_modificationDateEdit = new QLineEdit(m_propertiesGroup);
    m_modificationDateEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_modificationDateEdit, 7, 1);

    m_propertiesContentLayout->addWidget(m_propertiesGroup);
}

void DocumentMetadataDialog::createSecuritySection(StyleManager& styleManager) {
    m_securityGroup = new QGroupBox(tr("安全信息"), m_propertiesContentWidget);
    m_securityLayout = new QGridLayout(m_securityGroup);
    m_securityLayout->setContentsMargins(
        styleManager.spacingMD(), styleManager.spacingLG(),
        styleManager.spacingMD(), styleManager.spacingMD());
    m_securityLayout->setHorizontalSpacing(styleManager.spacingMD());
    m_securityLayout->setVerticalSpacing(styleManager.spacingSM());
    m_securityLayout->setColumnStretch(1, 1);

    auto* encryptedLabel = new QLabel(tr("加密状态:"), m_securityGroup);
    m_securityLayout->addWidget(encryptedLabel, 0, 0);
    m_encryptedEdit = new QLineEdit(m_securityGroup);
    m_encryptedEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_encryptedEdit, 0, 1);

    auto* encryptionMethodLabel = new QLabel(tr("加密方法:"), m_securityGroup);
    m_securityLayout->addWidget(encryptionMethodLabel, 1, 0);
    m_encryptionMethodEdit = new QLineEdit(m_securityGroup);
    m_encryptionMethodEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_encryptionMethodEdit, 1, 1);

    auto* extractLabel = new QLabel(tr("可提取文本:"), m_securityGroup);
    m_securityLayout->addWidget(extractLabel, 2, 0);
    m_canExtractTextEdit = new QLineEdit(m_securityGroup);
    m_canExtractTextEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canExtractTextEdit, 2, 1);

    auto* printLabel = new QLabel(tr("可打印:"), m_securityGroup);
    m_securityLayout->addWidget(printLabel, 3, 0);
    m_canPrintEdit = new QLineEdit(m_securityGroup);
    m_canPrintEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canPrintEdit, 3, 1);

    auto* printHighResLabel =
        new QLabel(tr("可高分辨率打印:"), m_securityGroup);
    m_securityLayout->addWidget(printHighResLabel, 4, 0);
    m_canPrintHighResEdit = new QLineEdit(m_securityGroup);
    m_canPrintHighResEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canPrintHighResEdit, 4, 1);

    auto* modifyLabel = new QLabel(tr("可修改:"), m_securityGroup);
    m_securityLayout->addWidget(modifyLabel, 5, 0);
    m_canModifyEdit = new QLineEdit(m_securityGroup);
    m_canModifyEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canModifyEdit, 5, 1);

    auto* modifyAnnotationsLabel =
        new QLabel(tr("可修改注释:"), m_securityGroup);
    m_securityLayout->addWidget(modifyAnnotationsLabel, 6, 0);
    m_canModifyAnnotationsEdit = new QLineEdit(m_securityGroup);
    m_canModifyAnnotationsEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canModifyAnnotationsEdit, 6, 1);

    auto* fillFormsLabel = new QLabel(tr("可填写表单:"), m_securityGroup);
    m_securityLayout->addWidget(fillFormsLabel, 7, 0);
    m_canFillFormsEdit = new QLineEdit(m_securityGroup);
    m_canFillFormsEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canFillFormsEdit, 7, 1);

    auto* assembleLabel = new QLabel(tr("可组装文档:"), m_securityGroup);
    m_securityLayout->addWidget(assembleLabel, 8, 0);
    m_canAssembleEdit = new QLineEdit(m_securityGroup);
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

    m_exportButton = new QPushButton(tr("导出信息"));
    m_exportButton->setToolTip(tr("将文档信息导出到文本文件"));
    m_buttonLayout->addWidget(m_exportButton);

    m_closeButton = new QPushButton(tr("关闭"));
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
        TOAST_ERROR(this, tr("获取文档元数据时发生错误: %1").arg(e.what()));
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
        m_pageCountEdit->setText(tr("未知"));
        m_pdfVersionEdit->setText(tr("未知"));
    }

    // 文件创建和修改时间
    QDateTime creationTime = fileInfo.birthTime();
    if (!creationTime.isValid()) {
        creationTime = fileInfo.metadataChangeTime();
    }
    m_creationDateFileEdit->setText(
        creationTime.isValid()
            ? formatDateTime(creationTime.toString(Qt::ISODate))
            : tr("未知"));

    QDateTime modificationTime = fileInfo.lastModified();
    m_modificationDateFileEdit->setText(
        modificationTime.isValid()
            ? formatDateTime(modificationTime.toString(Qt::ISODate))
            : tr("未知"));
}

void DocumentMetadataDialog::populateDocumentProperties(
    Poppler::Document* document) {
    if (document == nullptr) {
        return;
    }

    // 直接使用Poppler::Document的info方法获取元数据
    QString title = document->info("Title");
    m_titleEdit->setText(title.isEmpty() ? tr("未设置") : title);

    QString author = document->info("Author");
    m_authorEdit->setText(author.isEmpty() ? tr("未设置") : author);

    QString subject = document->info("Subject");
    m_subjectEdit->setText(subject.isEmpty() ? tr("未设置") : subject);

    QString keywords = document->info("Keywords");
    m_keywordsEdit->setText(keywords.isEmpty() ? tr("未设置") : keywords);

    QString creator = document->info("Creator");
    m_creatorEdit->setText(creator.isEmpty() ? tr("未设置") : creator);

    QString producer = document->info("Producer");
    m_producerEdit->setText(producer.isEmpty() ? tr("未设置") : producer);

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
        // 加密状态
        bool isEncrypted = document->isEncrypted();
        m_encryptedEdit->setText(isEncrypted ? tr("是") : tr("否"));

        // 加密方法
        if (isEncrypted) {
            m_encryptionMethodEdit->setText(tr("标准加密"));
        } else {
            m_encryptionMethodEdit->setText(tr("无"));
        }

        // 获取文档权限 - 如果文档已解锁，我们可以检查权限
        bool canExtractText = true;  // 如果能打开文档，通常可以提取文本
        bool canPrint = true;               // 默认允许打印
        bool canPrintHighRes = true;        // 默认允许高分辨率打印
        bool canModify = false;             // PDF查看器通常不允许修改
        bool canModifyAnnotations = false;  // 默认不允许修改注释
        bool canFillForms = true;           // 默认允许填写表单
        bool canAssemble = false;           // 默认不允许组装文档

        // 如果文档加密，权限可能受限
        if (isEncrypted) {
            // 这里可以根据实际的权限检查来设置
            canModify = false;
            canModifyAnnotations = false;
            canAssemble = false;
        }

        m_canExtractTextEdit->setText(canExtractText ? tr("是") : tr("否"));
        m_canPrintEdit->setText(canPrint ? tr("是") : tr("否"));
        m_canPrintHighResEdit->setText(canPrintHighRes ? tr("是") : tr("否"));
        m_canModifyEdit->setText(canModify ? tr("是") : tr("否"));
        m_canModifyAnnotationsEdit->setText(canModifyAnnotations ? tr("是")
                                                                 : tr("否"));
        m_canFillFormsEdit->setText(canFillForms ? tr("是") : tr("否"));
        m_canAssembleEdit->setText(canAssemble ? tr("是") : tr("否"));

    } catch (const std::exception& e) {
        // 如果获取安全信息失败，设置为未知
        m_encryptedEdit->setText(tr("未知"));
        m_encryptionMethodEdit->setText(tr("未知"));
        m_canExtractTextEdit->setText(tr("未知"));
        m_canPrintEdit->setText(tr("未知"));
        m_canPrintHighResEdit->setText(tr("未知"));
        m_canModifyEdit->setText(tr("未知"));
        m_canModifyAnnotationsEdit->setText(tr("未知"));
        m_canFillFormsEdit->setText(tr("未知"));
        m_canAssembleEdit->setText(tr("未知"));
    }
}

QString DocumentMetadataDialog::formatDateTime(const QString& dateTimeStr) {
    if (dateTimeStr.isEmpty()) {
        return tr("未设置");
    }

    // PDF日期格式通常是: D:YYYYMMDDHHmmSSOHH'mm'
    // 尝试解析不同的日期格式
    QDateTime dateTime;

    // 尝试ISO格式
    dateTime = QDateTime::fromString(dateTimeStr, Qt::ISODate);
    if (dateTime.isValid()) {
        return QLocale::system().toString(dateTime, QLocale::ShortFormat);
    }

    // 尝试PDF格式 D:YYYYMMDDHHmmSS
    if (dateTimeStr.startsWith("D:") && dateTimeStr.length() >= 16) {
        QString cleanDate = dateTimeStr.mid(2, 14);  // 取YYYYMMDDHHMMSS部分
        dateTime = QDateTime::fromString(cleanDate, "yyyyMMddhhmmss");
        if (dateTime.isValid()) {
            return QLocale::system().toString(dateTime, QLocale::ShortFormat);
        }
    }

    // 如果无法解析，返回原始字符串
    return dateTimeStr;
}

QString DocumentMetadataDialog::formatFileSize(qint64 bytes) {
    if (bytes < 0) {
        return tr("未知");
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
    return QString("%1 字节").arg(bytes);
}

QString DocumentMetadataDialog::getPdfVersion(Poppler::Document* document) {
    if (!document) {
        return tr("未知");
    }

    try {
        Poppler::Document::PdfVersion version = document->getPdfVersion();
        return QString("PDF %1.%2").arg(version.major).arg(version.minor);
    } catch (...) {
        return tr("未知");
    }
}

void DocumentMetadataDialog::exportMetadata() {
    if (m_currentFilePath.isEmpty()) {
        QMessageBox::warning(this, tr("导出错误"), tr("没有可导出的文档信息"));
        return;
    }

    // 获取建议的文件名
    QFileInfo fileInfo(m_currentFilePath);
    QString suggestedName = fileInfo.baseName() + "_metadata.txt";

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("导出文档信息"), QDir::homePath() + "/" + suggestedName,
        tr("文本文件 (*.txt);;所有文件 (*)"));

    if (fileName.isEmpty()) {
        return;
    }

    try {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw std::runtime_error(
                tr("无法创建文件: %1").arg(file.errorString()).toStdString());
        }

        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);

        // 写入文档信息
        out << tr("PDF文档信息报告") << "\n";
        out << QString("=").repeated(50) << "\n\n";

        // 基本信息
        out << tr("基本信息:") << "\n";
        out << tr("文件名: %1").arg(m_fileNameEdit->text()) << "\n";
        out << tr("文件路径: %1").arg(m_filePathEdit->text()) << "\n";
        out << tr("文件大小: %1").arg(m_fileSizeEdit->text()) << "\n";
        out << tr("页数: %1").arg(m_pageCountEdit->text()) << "\n";
        out << tr("PDF版本: %1").arg(m_pdfVersionEdit->text()) << "\n";
        out << tr("文件创建时间: %1").arg(m_creationDateFileEdit->text())
            << "\n";
        out << tr("文件修改时间: %1").arg(m_modificationDateFileEdit->text())
            << "\n\n";

        // 文档属性
        out << tr("文档属性:") << "\n";
        out << tr("标题: %1").arg(m_titleEdit->text()) << "\n";
        out << tr("作者: %1").arg(m_authorEdit->text()) << "\n";
        out << tr("主题: %1").arg(m_subjectEdit->text()) << "\n";
        out << tr("关键词: %1").arg(m_keywordsEdit->toPlainText()) << "\n";
        out << tr("创建者: %1").arg(m_creatorEdit->text()) << "\n";
        out << tr("生成者: %1").arg(m_producerEdit->text()) << "\n";
        out << tr("创建时间: %1").arg(m_creationDateEdit->text()) << "\n";
        out << tr("修改时间: %1").arg(m_modificationDateEdit->text()) << "\n\n";

        // 安全信息
        out << tr("安全信息:") << "\n";
        out << tr("加密状态: %1").arg(m_encryptedEdit->text()) << "\n";
        out << tr("加密方法: %1").arg(m_encryptionMethodEdit->text()) << "\n";
        out << tr("可提取文本: %1").arg(m_canExtractTextEdit->text()) << "\n";
        out << tr("可打印: %1").arg(m_canPrintEdit->text()) << "\n";
        out << tr("可高分辨率打印: %1").arg(m_canPrintHighResEdit->text())
            << "\n";
        out << tr("可修改: %1").arg(m_canModifyEdit->text()) << "\n";
        out << tr("可修改注释: %1").arg(m_canModifyAnnotationsEdit->text())
            << "\n";
        out << tr("可填写表单: %1").arg(m_canFillFormsEdit->text()) << "\n";
        out << tr("可组装文档: %1").arg(m_canAssembleEdit->text()) << "\n\n";

        // 导出信息
        out << QString("-").repeated(50) << "\n";
        out << tr("导出时间: %1").arg(QDateTime::currentDateTime().toString())
            << "\n";
        out << tr("导出工具: SAST Readium PDF Reader") << "\n";

        file.close();

        TOAST_SUCCESS(
            this,
            tr("文档信息已成功导出到: %1").arg(QFileInfo(fileName).fileName()));

    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("导出错误"),
                              tr("导出文档信息时发生错误: %1").arg(e.what()));
    }
}
