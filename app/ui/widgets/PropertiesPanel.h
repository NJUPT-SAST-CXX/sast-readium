#ifndef PROPERTIESPANEL_H
#define PROPERTIESPANEL_H

#include <poppler/qt6/poppler-qt6.h>
#include <QWidget>

// Forward declarations
class ElaText;
class QVBoxLayout;
class ElaScrollArea;

/**
 * @brief PropertiesPanel - Document properties display panel
 *
 * Displays comprehensive document metadata and statistics:
 * - Basic metadata (title, author, subject, keywords)
 * - Creation info (creator, producer, dates)
 * - Document statistics (page count, file size, PDF version)
 * - Security information (encryption, permissions)
 */
class PropertiesPanel : public QWidget {
    Q_OBJECT

public:
    explicit PropertiesPanel(QWidget* parent = nullptr);
    ~PropertiesPanel() override;

    // Document management
    void setDocument(Poppler::Document* document, const QString& filePath);
    void clearDocument();
    bool hasDocument() const;

signals:
    void documentChanged();

private:
    // UI Components
    ElaScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_mainLayout;

    // Metadata labels
    ElaText* m_titleLabel;
    ElaText* m_titleValue;

    ElaText* m_authorLabel;
    ElaText* m_authorValue;

    ElaText* m_subjectLabel;
    ElaText* m_subjectValue;

    ElaText* m_keywordsLabel;
    ElaText* m_keywordsValue;

    ElaText* m_creatorLabel;
    ElaText* m_creatorValue;

    ElaText* m_producerLabel;
    ElaText* m_producerValue;

    ElaText* m_creationDateLabel;
    ElaText* m_creationDateValue;

    ElaText* m_modificationDateLabel;
    ElaText* m_modificationDateValue;

    // Statistics labels
    ElaText* m_pageCountLabel;
    ElaText* m_pageCountValue;

    ElaText* m_fileSizeLabel;
    ElaText* m_fileSizeValue;

    ElaText* m_pdfVersionLabel;
    ElaText* m_pdfVersionValue;

    // Security labels
    ElaText* m_encryptedLabel;
    ElaText* m_encryptedValue;

    ElaText* m_linearizedLabel;
    ElaText* m_linearizedValue;

    // Document data
    Poppler::Document* m_document;
    QString m_filePath;

    // UI setup
    void setupUi();
    void createMetadataSection();
    void createStatisticsSection();
    void createSecuritySection();

    // Helper methods
    void updateMetadata();
    void updateStatistics();
    void updateSecurity();
    QString formatFileSize(qint64 bytes) const;
    QString formatDateTime(const QDateTime& dateTime) const;
    ElaText* createValueLabel(const QString& text = QString());
    ElaText* createSectionLabel(const QString& text);
    void addSeparator();
};

#endif  // PROPERTIESPANEL_H
