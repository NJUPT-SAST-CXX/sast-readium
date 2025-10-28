#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

/**
 * @brief Compact document properties panel for sidebar display
 *
 * @details Displays essential PDF document metadata in a compact format
 * suitable for sidebar display. Provides a "View Full Details" button
 * to open the complete DocumentMetadataDialog.
 *
 * Properties displayed:
 * - File name
 * - Page count
 * - File size
 * - PDF version
 * - Author
 * - Title
 * - Creation date
 * - Modification date
 */
class DocumentPropertiesPanel : public QWidget {
    Q_OBJECT

public:
    explicit DocumentPropertiesPanel(QWidget* parent = nullptr);
    ~DocumentPropertiesPanel() = default;

    /**
     * @brief Update the panel with document information
     * @param document Poppler document pointer (can be nullptr to clear)
     * @param filePath Full path to the PDF file
     */
    void setDocument(Poppler::Document* document, const QString& filePath);

    /**
     * @brief Clear all displayed properties
     */
    void clearProperties();

signals:
    /**
     * @brief Emitted when user clicks "View Full Details" button
     * @param document Current document pointer
     * @param filePath Current file path
     */
    void viewFullDetailsRequested(Poppler::Document* document,
                                  const QString& filePath);

protected:
    void changeEvent(QEvent* event) override;

private:
    void setupUI();
    void setupConnections();
    void retranslateUi();
    void applyTheme();
    void updatePropertyField(QLineEdit* field, const QString& value);

    // Helper methods
    static QString formatFileSize(qint64 bytes);
    static QString formatDateTime(const QDateTime& dateTime);
    QString getPdfVersion(Poppler::Document* document);

    // UI Components
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;

    // Property sections
    QGroupBox* m_fileInfoGroup;
    QFormLayout* m_fileInfoLayout;
    QLineEdit* m_fileNameField;
    QLineEdit* m_fileSizeField;
    QLineEdit* m_pageCountField;
    QLineEdit* m_pdfVersionField;

    QGroupBox* m_documentInfoGroup;
    QFormLayout* m_documentInfoLayout;
    QLineEdit* m_titleField;
    QLineEdit* m_authorField;
    QLineEdit* m_subjectField;
    QLineEdit* m_creatorField;

    QGroupBox* m_datesGroup;
    QFormLayout* m_datesLayout;
    QLineEdit* m_creationDateField;
    QLineEdit* m_modificationDateField;

    // Actions
    QPushButton* m_viewFullDetailsButton;
    QFrame* m_separatorLine;

    // Current document info
    Poppler::Document* m_currentDocument;
    QString m_currentFilePath;
};
