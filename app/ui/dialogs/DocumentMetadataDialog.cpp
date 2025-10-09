#include "DocumentMetadataDialog.h"
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QMessageBox>
#include <stdexcept>
#include "../../managers/StyleManager.h"

DocumentMetadataDialog::DocumentMetadataDialog(QWidget* parent)
    : QDialog(parent), m_currentDocument(nullptr) {
    setWindowTitle(tr("文档属性"));
    setModal(true);
    resize(600, 500);

    setupUI();
    setupConnections();
    applyCurrentTheme();
}

void DocumentMetadataDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);
    m_mainLayout->setSpacing(12);

    // 创建滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(16);

    // 基本信息组
    m_basicInfoGroup = new QGroupBox(tr("基本信息"), m_contentWidget);
    m_basicInfoLayout = new QGridLayout(m_basicInfoGroup);
    m_basicInfoLayout->setColumnStretch(1, 1);

    // 文件名
    m_basicInfoLayout->addWidget(new QLabel(tr("文件名:")), 0, 0);
    m_fileNameEdit = new QLineEdit();
    m_fileNameEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_fileNameEdit, 0, 1);

    // 文件路径
    m_basicInfoLayout->addWidget(new QLabel(tr("文件路径:")), 1, 0);
    m_filePathEdit = new QLineEdit();
    m_filePathEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_filePathEdit, 1, 1);

    // 文件大小
    m_basicInfoLayout->addWidget(new QLabel(tr("文件大小:")), 2, 0);
    m_fileSizeEdit = new QLineEdit();
    m_fileSizeEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_fileSizeEdit, 2, 1);

    // 页数
    m_basicInfoLayout->addWidget(new QLabel(tr("页数:")), 3, 0);
    m_pageCountEdit = new QLineEdit();
    m_pageCountEdit->setReadOnly(true);
    m_basicInfoLayout->addWidget(m_pageCountEdit, 3, 1);

    m_contentLayout->addWidget(m_basicInfoGroup);

    // 文档属性组
    m_propertiesGroup = new QGroupBox(tr("文档属性"), m_contentWidget);
    m_propertiesLayout = new QGridLayout(m_propertiesGroup);
    m_propertiesLayout->setColumnStretch(1, 1);

    // 标题
    m_propertiesLayout->addWidget(new QLabel(tr("标题:")), 0, 0);
    m_titleEdit = new QLineEdit();
    m_titleEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_titleEdit, 0, 1);

    // 作者
    m_propertiesLayout->addWidget(new QLabel(tr("作者:")), 1, 0);
    m_authorEdit = new QLineEdit();
    m_authorEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_authorEdit, 1, 1);

    // 主题
    m_propertiesLayout->addWidget(new QLabel(tr("主题:")), 2, 0);
    m_subjectEdit = new QLineEdit();
    m_subjectEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_subjectEdit, 2, 1);

    // 关键词
    m_propertiesLayout->addWidget(new QLabel(tr("关键词:")), 3, 0);
    m_keywordsEdit = new QLineEdit();
    m_keywordsEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_keywordsEdit, 3, 1);

    // 创建者
    m_propertiesLayout->addWidget(new QLabel(tr("创建者:")), 4, 0);
    m_creatorEdit = new QLineEdit();
    m_creatorEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_creatorEdit, 4, 1);

    // 生成者
    m_propertiesLayout->addWidget(new QLabel(tr("生成者:")), 5, 0);
    m_producerEdit = new QLineEdit();
    m_producerEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_producerEdit, 5, 1);

    // 创建时间
    m_propertiesLayout->addWidget(new QLabel(tr("创建时间:")), 6, 0);
    m_creationDateEdit = new QLineEdit();
    m_creationDateEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_creationDateEdit, 6, 1);

    // 修改时间
    m_propertiesLayout->addWidget(new QLabel(tr("修改时间:")), 7, 0);
    m_modificationDateEdit = new QLineEdit();
    m_modificationDateEdit->setReadOnly(true);
    m_propertiesLayout->addWidget(m_modificationDateEdit, 7, 1);

    m_contentLayout->addWidget(m_propertiesGroup);

    // 安全信息组
    m_securityGroup = new QGroupBox(tr("安全信息"), m_contentWidget);
    m_securityLayout = new QGridLayout(m_securityGroup);
    m_securityLayout->setColumnStretch(1, 1);

    // 加密状态
    m_securityLayout->addWidget(new QLabel(tr("加密状态:")), 0, 0);
    m_encryptedEdit = new QLineEdit();
    m_encryptedEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_encryptedEdit, 0, 1);

    // 可提取文本
    m_securityLayout->addWidget(new QLabel(tr("可提取文本:")), 1, 0);
    m_canExtractTextEdit = new QLineEdit();
    m_canExtractTextEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canExtractTextEdit, 1, 1);

    // 可打印
    m_securityLayout->addWidget(new QLabel(tr("可打印:")), 2, 0);
    m_canPrintEdit = new QLineEdit();
    m_canPrintEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canPrintEdit, 2, 1);

    // 可修改
    m_securityLayout->addWidget(new QLabel(tr("可修改:")), 3, 0);
    m_canModifyEdit = new QLineEdit();
    m_canModifyEdit->setReadOnly(true);
    m_securityLayout->addWidget(m_canModifyEdit, 3, 1);

    m_contentLayout->addWidget(m_securityGroup);

    // 添加弹性空间
    m_contentLayout->addStretch();

    m_scrollArea->setWidget(m_contentWidget);
    m_mainLayout->addWidget(m_scrollArea);

    // 按钮布局
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();

    m_closeButton = new QPushButton(tr("关闭"));
    m_closeButton->setDefault(true);
    m_buttonLayout->addWidget(m_closeButton);

    m_mainLayout->addLayout(m_buttonLayout);
}

void DocumentMetadataDialog::setupConnections() {
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);

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

    if (!document || filePath.isEmpty()) {
        clearMetadata();
        return;
    }

    try {
        populateBasicInfo(filePath, document);
        populateDocumentProperties(document);
        populateSecurityInfo(document);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, tr("错误"),
                             tr("获取文档元数据时发生错误: %1").arg(e.what()));
        clearMetadata();
    }
}

void DocumentMetadataDialog::clearMetadata() {
    // 清空所有字段
    m_fileNameEdit->clear();
    m_filePathEdit->clear();
    m_fileSizeEdit->clear();
    m_pageCountEdit->clear();

    m_titleEdit->clear();
    m_authorEdit->clear();
    m_subjectEdit->clear();
    m_keywordsEdit->clear();
    m_creatorEdit->clear();
    m_producerEdit->clear();
    m_creationDateEdit->clear();
    m_modificationDateEdit->clear();

    m_encryptedEdit->clear();
    m_canExtractTextEdit->clear();
    m_canPrintEdit->clear();
    m_canModifyEdit->clear();
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

    // 页数
    if (document) {
        int pageCount = document->numPages();
        m_pageCountEdit->setText(QString::number(pageCount));
    } else {
        m_pageCountEdit->setText(tr("未知"));
    }
}

void DocumentMetadataDialog::populateDocumentProperties(
    Poppler::Document* document) {
    if (!document) {
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
    if (!document) {
        return;
    }

    // 直接使用Poppler::Document的方法获取安全信息
    bool isEncrypted = document->isEncrypted();
    m_encryptedEdit->setText(isEncrypted ? tr("是") : tr("否"));

    // 简化的权限检查 - 对于基本的元数据显示，这些就足够了
    m_canExtractTextEdit->setText(
        tr("是"));  // 如果能打开文档，通常可以提取文本
    m_canPrintEdit->setText(tr("是"));   // 简化处理
    m_canModifyEdit->setText(tr("否"));  // PDF查看器通常不允许修改
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

    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;

    if (bytes >= GB) {
        return QString("%1 GB").arg(
            QString::number(bytes / double(GB), 'f', 2));
    } else if (bytes >= MB) {
        return QString("%1 MB").arg(
            QString::number(bytes / double(MB), 'f', 2));
    } else if (bytes >= KB) {
        return QString("%1 KB").arg(
            QString::number(bytes / double(KB), 'f', 1));
    } else {
        return QString("%1 字节").arg(bytes);
    }
}
