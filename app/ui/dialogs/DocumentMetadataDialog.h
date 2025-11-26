#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QClipboard>
#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>

#include <QObject>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QTabWidget>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

class StyleManager;

class ElaPushButton;
class ElaToolButton;
class ElaLineEdit;
class ElaScrollPageArea;
class ElaText;
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
    void exportMetadata();

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
    void retranslateUi();
    Q_SLOT void onThemeChanged();

protected:
    void changeEvent(QEvent* event) override;

    // UI组件
    QVBoxLayout* m_mainLayout = nullptr;
    QTabWidget* m_tabWidget = nullptr;

    // 标题栏
    QWidget* m_headerWidget = nullptr;
    QHBoxLayout* m_headerLayout = nullptr;
    QLabel* m_iconLabel = nullptr;

    ElaToolButton* m_copyAllButton = nullptr;

    // 基本信息标签页
    QWidget* m_basicTab = nullptr;
    QVBoxLayout* m_basicTabLayout = nullptr;
    QScrollArea* m_basicScrollArea = nullptr;
    QWidget* m_basicContentWidget = nullptr;
    QVBoxLayout* m_basicContentLayout = nullptr;

    // 基本信息组
    ElaScrollPageArea* m_basicInfoGroup = nullptr;
    ElaText* m_basicInfoTitle = nullptr;
    QGridLayout* m_basicInfoLayout = nullptr;
    ElaLineEdit* m_fileNameEdit = nullptr;
    ElaLineEdit* m_filePathEdit = nullptr;
    ElaLineEdit* m_fileSizeEdit = nullptr;
    ElaLineEdit* m_pageCountEdit = nullptr;
    ElaLineEdit* m_pdfVersionEdit = nullptr;
    ElaLineEdit* m_creationDateFileEdit = nullptr;
    ElaLineEdit* m_modificationDateFileEdit = nullptr;
    ElaToolButton* m_copyPathButton = nullptr;

    // 页面信息组
    ElaScrollPageArea* m_pageInfoGroup = nullptr;
    ElaText* m_pageInfoTitle = nullptr;
    QGridLayout* m_pageInfoLayout = nullptr;
    ElaLineEdit* m_pageSizeEdit = nullptr;
    ElaLineEdit* m_pageOrientationEdit = nullptr;
    ElaLineEdit* m_pageRotationEdit = nullptr;

    // 文档属性标签页
    QWidget* m_propertiesTab = nullptr;
    QVBoxLayout* m_propertiesTabLayout = nullptr;
    QScrollArea* m_propertiesScrollArea = nullptr;
    QWidget* m_propertiesContentWidget = nullptr;
    QVBoxLayout* m_propertiesContentLayout = nullptr;

    // 文档属性组
    ElaScrollPageArea* m_propertiesGroup = nullptr;
    ElaText* m_propertiesTitle = nullptr;
    QGridLayout* m_propertiesLayout = nullptr;
    ElaLineEdit* m_titleEdit = nullptr;
    ElaLineEdit* m_authorEdit = nullptr;
    ElaLineEdit* m_subjectEdit = nullptr;
    QTextEdit* m_keywordsEdit = nullptr;
    ElaLineEdit* m_creatorEdit = nullptr;
    ElaLineEdit* m_producerEdit = nullptr;
    ElaLineEdit* m_creationDateEdit = nullptr;
    ElaLineEdit* m_modificationDateEdit = nullptr;

    // 安全信息标签页
    QWidget* m_securityTab = nullptr;
    QVBoxLayout* m_securityTabLayout = nullptr;

    // 安全信息组
    ElaScrollPageArea* m_securityGroup = nullptr;
    ElaText* m_securityTitle = nullptr;
    QGridLayout* m_securityLayout = nullptr;
    ElaLineEdit* m_encryptedEdit = nullptr;
    ElaLineEdit* m_encryptionMethodEdit = nullptr;
    ElaLineEdit* m_canExtractTextEdit = nullptr;
    ElaLineEdit* m_canPrintEdit = nullptr;
    ElaLineEdit* m_canPrintHighResEdit = nullptr;
    ElaLineEdit* m_canModifyEdit = nullptr;
    ElaLineEdit* m_canModifyAnnotationsEdit = nullptr;
    ElaLineEdit* m_canFillFormsEdit = nullptr;
    ElaLineEdit* m_canAssembleEdit = nullptr;

    // 高级信息标签页
    QWidget* m_advancedTab = nullptr;
    QVBoxLayout* m_advancedTabLayout = nullptr;

    // 字体信息组
    ElaScrollPageArea* m_fontGroup = nullptr;
    ElaText* m_fontTitle = nullptr;
    QVBoxLayout* m_fontLayout = nullptr;
    QTreeWidget* m_fontTree = nullptr;

    // 其他信息组
    ElaScrollPageArea* m_advancedGroup = nullptr;
    ElaText* m_advancedTitle = nullptr;
    QGridLayout* m_advancedLayout = nullptr;
    ElaLineEdit* m_linearizedEdit = nullptr;
    ElaLineEdit* m_hasFormsEdit = nullptr;
    ElaLineEdit* m_hasJavaScriptEdit = nullptr;
    ElaLineEdit* m_hasEmbeddedFilesEdit = nullptr;

    // 按钮
    QHBoxLayout* m_buttonLayout = nullptr;
    ElaPushButton* m_exportButton = nullptr;
    ElaPushButton* m_closeButton = nullptr;

    // 当前文档信息
    QString m_currentFilePath;
    Poppler::Document* m_currentDocument = nullptr;

    // 统计信息
    int m_fontCount = 0;
    int m_embeddedFontCount = 0;
};
