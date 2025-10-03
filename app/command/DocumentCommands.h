#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <memory>
#include "../controller/tool.hpp"
#include "../logging/SimpleLogging.h"

// Forward declarations
class DocumentController;
class DocumentModel;
class QWidget;

namespace Poppler {
    class Document;
}

/**
 * @brief Base class for document-related commands
 * 
 * Provides common functionality for all document operations
 * following the Command pattern.
 */
class DocumentCommand : public QObject {
    Q_OBJECT

public:
    explicit DocumentCommand(DocumentController* controller,
                           const QString& name,
                           QObject* parent = nullptr);
    virtual ~DocumentCommand() = default;

    // Command interface
    virtual bool execute() = 0;
    virtual bool canExecute() const;
    virtual bool undo() { return false; }  // Most document commands are not undoable
    
    // Command metadata
    QString name() const { return m_name; }
    QString description() const { return m_description; }
    ActionMap actionId() const { return m_actionId; }
    
    // Error handling
    bool hasError() const { return !m_errorMessage.isEmpty(); }
    QString errorMessage() const { return m_errorMessage; }
    
signals:
    void executed(bool success);
    void progress(int value, int maximum);
    void statusMessage(const QString& message);

protected:
    void setDescription(const QString& desc) { m_description = desc; }
    void setActionId(ActionMap id) { m_actionId = id; }
    void setErrorMessage(const QString& error) { m_errorMessage = error; }
    void clearError() { m_errorMessage.clear(); }
    
    DocumentController* controller() const { return m_controller; }
    
private:
    DocumentController* m_controller;
    QString m_name;
    QString m_description;
    ActionMap m_actionId = ActionMap::openFile; // Default action
    QString m_errorMessage;
    
protected:
    SastLogging::CategoryLogger m_logger{"DocumentCommand"};
};

/**
 * @brief Command to open a single document
 */
class OpenDocumentCommand : public DocumentCommand {
    Q_OBJECT

public:
    explicit OpenDocumentCommand(DocumentController* controller,
                                const QString& filePath = QString(),
                                QObject* parent = nullptr);
    
    void setFilePath(const QString& path) { m_filePath = path; }
    QString filePath() const { return m_filePath; }
    
    bool execute() override;
    bool canExecute() const override;
    
private:
    QString m_filePath;
    bool m_promptUser;
};

/**
 * @brief Command to open multiple documents
 */
class OpenDocumentsCommand : public DocumentCommand {
    Q_OBJECT

public:
    explicit OpenDocumentsCommand(DocumentController* controller,
                                 const QStringList& filePaths = QStringList(),
                                 QObject* parent = nullptr);
    
    void setFilePaths(const QStringList& paths) { m_filePaths = paths; }
    QStringList filePaths() const { return m_filePaths; }
    
    bool execute() override;
    bool canExecute() const override;
    
private:
    QStringList m_filePaths;
    bool m_promptUser = false;
    int m_successCount = 0;
    int m_failureCount = 0;
};

/**
 * @brief Command to close the current document
 */
class CloseDocumentCommand : public DocumentCommand {
    Q_OBJECT

public:
    explicit CloseDocumentCommand(DocumentController* controller,
                                 int index = -1,
                                 QObject* parent = nullptr);
    
    void setDocumentIndex(int index) { m_index = index; }
    int documentIndex() const { return m_index; }
    
    bool execute() override;
    bool canExecute() const override;
    
private:
    int m_index;  // -1 means current document
    bool m_closeCurrent = false;
    bool m_savePrompted = false;
};

/**
 * @brief Command to save document as a new file
 */
class SaveDocumentAsCommand : public DocumentCommand {
    Q_OBJECT

public:
    explicit SaveDocumentAsCommand(DocumentController* controller,
                                  const QString& targetPath = QString(),
                                  QObject* parent = nullptr);
    
    void setTargetPath(const QString& path) { m_targetPath = path; }
    QString targetPath() const { return m_targetPath; }
    
    bool execute() override;
    bool canExecute() const override;
    
private:
    QString m_targetPath;
    bool m_promptUser;
};

/**
 * @brief Command to export document to different format
 */
class ExportDocumentCommand : public DocumentCommand {
    Q_OBJECT

public:
    enum ExportFormat {
        PDF,
        Images,
        Text,
        HTML
    };
    
    explicit ExportDocumentCommand(DocumentController* controller,
                                  ExportFormat format,
                                  QObject* parent = nullptr);
    
    void setFormat(ExportFormat format) { m_format = format; }
    void setOutputPath(const QString& path) { m_outputPath = path; }
    void setOptions(const QVariantMap& options) { m_options = options; }
    
    bool execute() override;
    bool canExecute() const override;

private:
    // Helper methods for different export formats
    bool exportToPDF(Poppler::Document* document, const QString& sourcePath, const QString& outputPath);
    bool exportToImages(Poppler::Document* document, const QString& outputPath, int totalPages);
    bool exportToText(Poppler::Document* document, const QString& outputPath, int totalPages);

    ExportFormat m_format;
    QString m_outputPath;
    QVariantMap m_options;
};

/**
 * @brief Command to print document
 */
class PrintDocumentCommand : public DocumentCommand {
    Q_OBJECT

public:
    explicit PrintDocumentCommand(DocumentController* controller,
                                 QObject* parent = nullptr);
    
    void setPageRange(int start, int end);
    void setPrintOptions(const QVariantMap& options) { m_printOptions = options; }
    
    bool execute() override;
    bool canExecute() const override;
    
private:
    int m_startPage = -1;
    int m_endPage = -1;
    QVariantMap m_printOptions;
};

/**
 * @brief Command to reload the current document
 */
class ReloadDocumentCommand : public DocumentCommand {
    Q_OBJECT

public:
    explicit ReloadDocumentCommand(DocumentController* controller,
                                  QObject* parent = nullptr);
    
    bool execute() override;
    bool canExecute() const override;
    
private:
    int m_previousPage = 0;
    double m_previousZoom = 1.0;
};

/**
 * @brief Command to show document properties/metadata
 */
class ShowDocumentPropertiesCommand : public DocumentCommand {
    Q_OBJECT

public:
    explicit ShowDocumentPropertiesCommand(DocumentController* controller,
                                          QWidget* parentWidget = nullptr,
                                          QObject* parent = nullptr);
    
    void setParentWidget(QWidget* widget) { m_parentWidget = widget; }
    
    bool execute() override;
    bool canExecute() const override;
    
private:
    QWidget* m_parentWidget;
};

/**
 * @brief Command to compare two documents
 */
class CompareDocumentsCommand : public DocumentCommand {
    Q_OBJECT

public:
    explicit CompareDocumentsCommand(DocumentController* controller,
                                    const QString& firstPath = QString(),
                                    const QString& secondPath = QString(),
                                    QObject* parent = nullptr);
    
    void setFirstDocument(const QString& path) { m_firstPath = path; }
    void setSecondDocument(const QString& path) { m_secondPath = path; }
    void setComparisonOptions(const QVariantMap& options) { m_options = options; }
    
    bool execute() override;
    bool canExecute() const override;
    
private:
    QString m_firstPath;
    QString m_secondPath;
    QVariantMap m_options;
};

/**
 * @brief Macro command to execute multiple document commands
 */
class DocumentMacroCommand : public DocumentCommand {
    Q_OBJECT

public:
    explicit DocumentMacroCommand(DocumentController* controller,
                                const QString& name,
                                QObject* parent = nullptr);
    ~DocumentMacroCommand();
    
    void addCommand(DocumentCommand* command);
    void clearCommands();
    int commandCount() const { return m_commands.size(); }

    bool execute() override;
    bool canExecute() const override;
    bool undo() override;

private:
    QList<DocumentCommand*> m_commands;
    QList<DocumentCommand*> m_executedCommands;
};

/**
 * @brief Factory class for creating document commands
 */
class DocumentCommandFactory {
public:
    static std::unique_ptr<DocumentCommand> createOpenCommand(
        DocumentController* controller, const QString& filePath = QString());

    static std::unique_ptr<DocumentCommand> createOpenMultipleCommand(
        DocumentController* controller, const QStringList& filePaths = QStringList());

    static std::unique_ptr<DocumentCommand> createCloseCommand(
        DocumentController* controller, int index = -1);

    static std::unique_ptr<DocumentCommand> createSaveAsCommand(
        DocumentController* controller, const QString& targetPath = QString());

    static std::unique_ptr<DocumentCommand> createExportCommand(
        DocumentController* controller, ExportDocumentCommand::ExportFormat format);

    static std::unique_ptr<DocumentCommand> createPrintCommand(
        DocumentController* controller);

    static std::unique_ptr<DocumentCommand> createReloadCommand(
        DocumentController* controller);

    static std::unique_ptr<DocumentMacroCommand> createMacroCommand(
        DocumentController* controller, const QString& name);

    static std::unique_ptr<DocumentCommand> createCommandFromType(
        const QString& type, DocumentController* controller);
};
