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

class DocumentMetadataDialog : public QDialog {
    Q_OBJECT

public:
    explicit DocumentMetadataDialog(QWidget* parent = nullptr);
    ~DocumentMetadataDialog() = default;

    // 设置要显示元数据的PDF文档
    void setDocument(Poppler::Document* document, const QString& filePath);

private slots:
    void onThemeChanged();

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

    QString formatDateTime(const QString& dateTimeStr);
    QString formatFileSize(qint64 bytes);
    QString getPdfVersion(Poppler::Document* document);
    QString getPageSizeString(const QSizeF& size);
    QIcon getSectionIcon(const QString& section);

    // UI组件
    QVBoxLayout* m_mainLayout;
    QTabWidget* m_tabWidget;

    // 标题栏
    QWidget* m_headerWidget;
    QHBoxLayout* m_headerLayout;
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QToolButton* m_copyAllButton;

    // 基本信息标签页
    QWidget* m_basicTab;
    QVBoxLayout* m_basicTabLayout;
    QScrollArea* m_basicScrollArea;
    QWidget* m_basicContentWidget;
    QVBoxLayout* m_basicContentLayout;

    // 基本信息组
    QGroupBox* m_basicInfoGroup;
    QGridLayout* m_basicInfoLayout;
    QLineEdit* m_fileNameEdit;
    QLineEdit* m_filePathEdit;
    QLineEdit* m_fileSizeEdit;
    QLineEdit* m_pageCountEdit;
    QLineEdit* m_pdfVersionEdit;
    QLineEdit* m_creationDateFileEdit;
    QLineEdit* m_modificationDateFileEdit;
    QToolButton* m_copyPathButton;

    // 页面信息组
    QGroupBox* m_pageInfoGroup;
    QGridLayout* m_pageInfoLayout;
    QLineEdit* m_pageSizeEdit;
    QLineEdit* m_pageOrientationEdit;
    QLineEdit* m_pageRotationEdit;

    // 文档属性标签页
    QWidget* m_propertiesTab;
    QVBoxLayout* m_propertiesTabLayout;
    QScrollArea* m_propertiesScrollArea;
    QWidget* m_propertiesContentWidget;
    QVBoxLayout* m_propertiesContentLayout;

    // 文档属性组
    QGroupBox* m_propertiesGroup;
    QGridLayout* m_propertiesLayout;
    QLineEdit* m_titleEdit;
    QLineEdit* m_authorEdit;
    QLineEdit* m_subjectEdit;
    QTextEdit* m_keywordsEdit;
    QLineEdit* m_creatorEdit;
    QLineEdit* m_producerEdit;
    QLineEdit* m_creationDateEdit;
    QLineEdit* m_modificationDateEdit;

    // 安全信息标签页
    QWidget* m_securityTab;
    QVBoxLayout* m_securityTabLayout;

    // 安全信息组
    QGroupBox* m_securityGroup;
    QGridLayout* m_securityLayout;
    QLineEdit* m_encryptedEdit;
    QLineEdit* m_encryptionMethodEdit;
    QLineEdit* m_canExtractTextEdit;
    QLineEdit* m_canPrintEdit;
    QLineEdit* m_canPrintHighResEdit;
    QLineEdit* m_canModifyEdit;
    QLineEdit* m_canModifyAnnotationsEdit;
    QLineEdit* m_canFillFormsEdit;
    QLineEdit* m_canAssembleEdit;

    // 高级信息标签页
    QWidget* m_advancedTab;
    QVBoxLayout* m_advancedTabLayout;

    // 字体信息组
    QGroupBox* m_fontGroup;
    QVBoxLayout* m_fontLayout;
    QTreeWidget* m_fontTree;
    QLabel* m_fontCountLabel;

    // 其他信息组
    QGroupBox* m_advancedGroup;
    QGridLayout* m_advancedLayout;
    QLineEdit* m_linearizedEdit;
    QLineEdit* m_hasFormsEdit;
    QLineEdit* m_hasJavaScriptEdit;
    QLineEdit* m_hasEmbeddedFilesEdit;

    // 按钮
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_exportButton;
    QPushButton* m_closeButton;

    // 当前文档信息
    QString m_currentFilePath;
    Poppler::Document* m_currentDocument;

    // 统计信息
    int m_fontCount;
    int m_embeddedFontCount;
};
