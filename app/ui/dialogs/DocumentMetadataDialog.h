#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QClipboard>
#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

class StyleManager;

class DocumentMetadataDialog : public QDialog {
    Q_OBJECT

public:
    explicit DocumentMetadataDialog(QWidget* parent = nullptr);

    // 设置要显示元数据的PDF文档
    void setDocument(Poppler::Document* document, const QString& filePath);

private:
    void setupUI();
    void setupConnections();
    void applyCurrentTheme();
    void clearMetadata();
    void populateBasicInfo(const QString& filePath,
                           Poppler::Document* document);
    void populateDocumentProperties(Poppler::Document* document);
    void populateSecurityInfo(Poppler::Document* document);
    void populateAdvancedInfo(Poppler::Document* document);
    void populateFontInfo(Poppler::Document* document);
    void populatePageInfo(Poppler::Document* document);
    void copyToClipboard(const QString& text);
    void copyAllMetadata();

    static QString formatDateTime(const QString& dateTimeStr);
    static QString formatFileSize(qint64 bytes);
    QString getPdfVersion(Poppler::Document* document);
    QString getPageSizeString(const QSizeF& size);
    QIcon getSectionIcon(const QString& section);
    void initializeMainLayout(StyleManager& styleManager);
    void createBasicInfoSection(StyleManager& styleManager);
    void createPropertiesSection(StyleManager& styleManager);
    void createSecuritySection(StyleManager& styleManager);
    void createActionButtons();
    Q_SLOT void onThemeChanged();

    // UI组件
    QVBoxLayout* m_mainLayout = nullptr;
    QTabWidget* m_tabWidget = nullptr;

    // 标题栏
    QWidget* m_headerWidget = nullptr;
    QHBoxLayout* m_headerLayout = nullptr;
    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QToolButton* m_copyAllButton = nullptr;

    // 基本信息标签页
    QWidget* m_basicTab = nullptr;
    QVBoxLayout* m_basicTabLayout = nullptr;
    QScrollArea* m_basicScrollArea = nullptr;
    QWidget* m_basicContentWidget = nullptr;
    QVBoxLayout* m_basicContentLayout = nullptr;

    // 基本信息组
    QGroupBox* m_basicInfoGroup = nullptr;
    QGridLayout* m_basicInfoLayout = nullptr;
    QLineEdit* m_fileNameEdit = nullptr;
    QLineEdit* m_filePathEdit = nullptr;
    QLineEdit* m_fileSizeEdit = nullptr;
    QLineEdit* m_pageCountEdit = nullptr;
    QLineEdit* m_pdfVersionEdit = nullptr;
    QLineEdit* m_creationDateFileEdit = nullptr;
    QLineEdit* m_modificationDateFileEdit = nullptr;
    QToolButton* m_copyPathButton = nullptr;

    // 页面信息组
    QGroupBox* m_pageInfoGroup = nullptr;
    QGridLayout* m_pageInfoLayout = nullptr;
    QLineEdit* m_pageSizeEdit = nullptr;
    QLineEdit* m_pageOrientationEdit = nullptr;
    QLineEdit* m_pageRotationEdit = nullptr;

    // 文档属性标签页
    QWidget* m_propertiesTab = nullptr;
    QVBoxLayout* m_propertiesTabLayout = nullptr;
    QScrollArea* m_propertiesScrollArea = nullptr;
    QWidget* m_propertiesContentWidget = nullptr;
    QVBoxLayout* m_propertiesContentLayout = nullptr;

    // 文档属性组
    QGroupBox* m_propertiesGroup = nullptr;
    QGridLayout* m_propertiesLayout = nullptr;
    QLineEdit* m_titleEdit = nullptr;
    QLineEdit* m_authorEdit = nullptr;
    QLineEdit* m_subjectEdit = nullptr;
    QTextEdit* m_keywordsEdit = nullptr;
    QLineEdit* m_creatorEdit = nullptr;
    QLineEdit* m_producerEdit = nullptr;
    QLineEdit* m_creationDateEdit = nullptr;
    QLineEdit* m_modificationDateEdit = nullptr;

    // 安全信息标签页
    QWidget* m_securityTab = nullptr;
    QVBoxLayout* m_securityTabLayout = nullptr;

    // 安全信息组
    QGroupBox* m_securityGroup = nullptr;
    QGridLayout* m_securityLayout = nullptr;
    QLineEdit* m_encryptedEdit = nullptr;
    QLineEdit* m_encryptionMethodEdit = nullptr;
    QLineEdit* m_canExtractTextEdit = nullptr;
    QLineEdit* m_canPrintEdit = nullptr;
    QLineEdit* m_canPrintHighResEdit = nullptr;
    QLineEdit* m_canModifyEdit = nullptr;
    QLineEdit* m_canModifyAnnotationsEdit = nullptr;
    QLineEdit* m_canFillFormsEdit = nullptr;
    QLineEdit* m_canAssembleEdit = nullptr;

    // 高级信息标签页
    QWidget* m_advancedTab = nullptr;
    QVBoxLayout* m_advancedTabLayout = nullptr;

    // 字体信息组
    QGroupBox* m_fontGroup = nullptr;
    QVBoxLayout* m_fontLayout = nullptr;
    QTreeWidget* m_fontTree = nullptr;
    QLabel* m_fontCountLabel = nullptr;

    // 其他信息组
    QGroupBox* m_advancedGroup = nullptr;
    QGridLayout* m_advancedLayout = nullptr;
    QLineEdit* m_linearizedEdit = nullptr;
    QLineEdit* m_hasFormsEdit = nullptr;
    QLineEdit* m_hasJavaScriptEdit = nullptr;
    QLineEdit* m_hasEmbeddedFilesEdit = nullptr;

    // 按钮
    QHBoxLayout* m_buttonLayout = nullptr;
    QPushButton* m_exportButton = nullptr;
    QPushButton* m_closeButton = nullptr;

    // 当前文档信息
    QString m_currentFilePath;
    Poppler::Document* m_currentDocument = nullptr;

    // 统计信息
    int m_fontCount = 0;
    int m_embeddedFontCount = 0;
};
