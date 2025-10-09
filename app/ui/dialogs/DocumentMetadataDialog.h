#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
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

    QString formatDateTime(const QString& dateTimeStr);
    QString formatFileSize(qint64 bytes);

    // UI组件
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;

    // 基本信息组
    QGroupBox* m_basicInfoGroup;
    QGridLayout* m_basicInfoLayout;
    QLineEdit* m_fileNameEdit;
    QLineEdit* m_filePathEdit;
    QLineEdit* m_fileSizeEdit;
    QLineEdit* m_pageCountEdit;

    // 文档属性组
    QGroupBox* m_propertiesGroup;
    QGridLayout* m_propertiesLayout;
    QLineEdit* m_titleEdit;
    QLineEdit* m_authorEdit;
    QLineEdit* m_subjectEdit;
    QLineEdit* m_keywordsEdit;
    QLineEdit* m_creatorEdit;
    QLineEdit* m_producerEdit;
    QLineEdit* m_creationDateEdit;
    QLineEdit* m_modificationDateEdit;

    // 安全信息组
    QGroupBox* m_securityGroup;
    QGridLayout* m_securityLayout;
    QLineEdit* m_encryptedEdit;
    QLineEdit* m_canExtractTextEdit;
    QLineEdit* m_canPrintEdit;
    QLineEdit* m_canModifyEdit;

    // 按钮
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_closeButton;

    // 当前文档信息
    QString m_currentFilePath;
    Poppler::Document* m_currentDocument;
};
