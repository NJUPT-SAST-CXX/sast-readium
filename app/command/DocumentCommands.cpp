#include "DocumentCommands.h"

#include <poppler/qt6/poppler-qt6.h>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <algorithm>

#include "../controller/DocumentController.h"
#include "../model/DocumentModel.h"
#include "../ui/dialogs/DocumentComparison.h"
#include "../ui/dialogs/DocumentMetadataDialog.h"

// DocumentCommand base class implementation
DocumentCommand::DocumentCommand(DocumentController* controller,
                                 const QString& name, QObject* parent)
    : QObject(parent),
      m_controller(controller),
      m_name(name),
      m_logger("DocumentCommand") {
    m_logger.debug(QString("Created document command: %1").arg(name));
}

bool DocumentCommand::canExecute() const { return !m_controller.isNull(); }

// OpenDocumentCommand implementation
OpenDocumentCommand::OpenDocumentCommand(DocumentController* controller,
                                         const QString& filePath,
                                         QObject* parent)
    : DocumentCommand(controller, "Open Document", parent),
      m_filePath(filePath),
      m_promptUser(filePath.isEmpty()) {
    if (!filePath.isEmpty()) {
        setDescription(
            QString("Open document: %1").arg(QFileInfo(filePath).fileName()));
        setActionId(ActionMap::openFile);
    } else {
        setDescription("Open document from file dialog");
        setActionId(ActionMap::openFile);
    }
}

bool OpenDocumentCommand::execute() {
    if (controller() == nullptr) {
        setErrorMessage("DocumentController is null");
        m_logger.error("DocumentController is null");
        emit executed(false);
        return false;
    }

    QString fileToOpen = m_filePath;

    // If no file path provided, prompt user
    if (m_promptUser || fileToOpen.isEmpty()) {
        fileToOpen = QFileDialog::getOpenFileName(
            nullptr, "Open PDF Document", QString(),
            "PDF Files (*.pdf);;All Files (*)");

        if (fileToOpen.isEmpty()) {
            m_logger.debug("User cancelled file selection");
            emit executed(false);
            return false;
        }
    }

    // Validate file exists
    if (!QFileInfo::exists(fileToOpen)) {
        setErrorMessage(QString("File does not exist: %1").arg(fileToOpen));
        m_logger.error(QString("File does not exist: %1").arg(fileToOpen));
        emit executed(false);
        return false;
    }

    try {
        bool success = controller()->openDocument(fileToOpen);

        if (success) {
            m_logger.info(
                QString("Successfully opened document: %1").arg(fileToOpen));
            emit statusMessage(
                QString("Opened: %1").arg(QFileInfo(fileToOpen).fileName()));
            emit executed(true);
            return true;
        }
        setErrorMessage(QString("Failed to open document: %1").arg(fileToOpen));
        m_logger.error(QString("Failed to open document: %1").arg(fileToOpen));
        emit executed(false);
        return false;

    } catch (const std::exception& e) {
        setErrorMessage(
            QString("Exception while opening document: %1").arg(e.what()));
        m_logger.error(
            QString("Exception while opening document: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool OpenDocumentCommand::canExecute() const {
    if (!DocumentCommand::canExecute()) {
        return false;
    }

    // If we have a specific file path, check if it exists
    if (!m_filePath.isEmpty()) {
        return QFileInfo::exists(m_filePath);
    }

    // If we're prompting user, we can always execute
    return true;
}

// OpenDocumentsCommand implementation
OpenDocumentsCommand::OpenDocumentsCommand(DocumentController* controller,
                                           const QStringList& filePaths,
                                           QObject* parent)
    : DocumentCommand(controller, "Open Documents", parent),
      m_filePaths(filePaths),
      m_promptUser(filePaths.isEmpty()) {
    if (!filePaths.isEmpty()) {
        setDescription(QString("Open %1 documents").arg(filePaths.size()));
        setActionId(ActionMap::openFile);
    } else {
        setDescription("Open multiple documents from file dialog");
        setActionId(ActionMap::openFile);
    }
}

bool OpenDocumentsCommand::execute() {
    if (controller() == nullptr) {
        setErrorMessage("DocumentController is null");
        m_logger.error("DocumentController is null");
        emit executed(false);
        return false;
    }

    QStringList filesToOpen = m_filePaths;

    // If no file paths provided, prompt user
    if (m_promptUser || filesToOpen.isEmpty()) {
        filesToOpen = QFileDialog::getOpenFileNames(
            nullptr, "Open PDF Documents", QString(),
            "PDF Files (*.pdf);;All Files (*)");

        if (filesToOpen.isEmpty()) {
            m_logger.debug("User cancelled file selection");
            emit executed(false);
            return false;
        }
    }

    // Validate all files exist
    QStringList validFiles;
    for (const QString& file : filesToOpen) {
        if (QFileInfo::exists(file)) {
            validFiles.append(file);
        } else {
            m_logger.warning(
                QString("Skipping non-existent file: %1").arg(file));
        }
    }

    if (validFiles.isEmpty()) {
        setErrorMessage("No valid files to open");
        m_logger.error("No valid files to open");
        emit executed(false);
        return false;
    }

    try {
        int successCount = 0;
        qsizetype totalCount = validFiles.size();

        emit progress(0, static_cast<int>(totalCount));

        for (qsizetype i = 0; i < validFiles.size(); ++i) {
            const QString& file = validFiles[i];

            bool success = controller()->openDocument(file);
            if (success) {
                successCount++;
                m_logger.debug(QString("Opened document %1/%2: %3")
                                   .arg(i + 1)
                                   .arg(totalCount)
                                   .arg(file));
            } else {
                m_logger.warning(QString("Failed to open document %1/%2: %3")
                                     .arg(i + 1)
                                     .arg(totalCount)
                                     .arg(file));
            }

            emit progress(static_cast<int>(i + 1),
                          static_cast<int>(totalCount));
        }

        if (successCount > 0) {
            m_logger.info(QString("Successfully opened %1/%2 documents")
                              .arg(successCount)
                              .arg(totalCount));
            emit statusMessage(QString("Opened %1/%2 documents")
                                   .arg(successCount)
                                   .arg(totalCount));
            emit executed(true);
            return true;
        }
        setErrorMessage("Failed to open any documents");
        m_logger.error("Failed to open any documents");
        emit executed(false);
        return false;

    } catch (const std::exception& e) {
        setErrorMessage(
            QString("Exception while opening documents: %1").arg(e.what()));
        m_logger.error(
            QString("Exception while opening documents: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool OpenDocumentsCommand::canExecute() const {
    if (!DocumentCommand::canExecute()) {
        return false;
    }

    // If we have specific file paths, check if at least one exists
    if (!m_filePaths.isEmpty()) {
        return std::any_of(
            m_filePaths.begin(), m_filePaths.end(),
            [](const QString& file) { return QFileInfo::exists(file); });
    }

    // If we're prompting user, we can always execute
    return true;
}

// CloseDocumentCommand implementation
CloseDocumentCommand::CloseDocumentCommand(DocumentController* controller,
                                           int index, QObject* parent)
    : DocumentCommand(controller, "Close Document", parent),
      m_index(index),
      m_closeCurrent(index == -1) {
    if (m_closeCurrent) {
        setDescription("Close current document");
        setActionId(ActionMap::closeFile);
    } else {
        setDescription(QString("Close document at index %1").arg(index));
        setActionId(ActionMap::closeFile);
    }
}

bool CloseDocumentCommand::execute() {
    if (controller() == nullptr) {
        setErrorMessage("DocumentController is null");
        m_logger.error("DocumentController is null");
        emit executed(false);
        return false;
    }

    try {
        bool success = false;

        if (m_closeCurrent) {
            success = controller()->closeCurrentDocument();
            m_logger.debug("Attempted to close current document");
        } else {
            success = controller()->closeDocument(m_index);
            m_logger.debug(QString("Attempted to close document at index: %1")
                               .arg(m_index));
        }

        if (success) {
            m_logger.info("Successfully closed document");
            emit statusMessage("Document closed");
            emit executed(true);
            return true;
        }
        setErrorMessage("Failed to close document");
        m_logger.error("Failed to close document");
        emit executed(false);
        return false;

    } catch (const std::exception& e) {
        setErrorMessage(
            QString("Exception while closing document: %1").arg(e.what()));
        m_logger.error(
            QString("Exception while closing document: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool CloseDocumentCommand::canExecute() const {
    if (!DocumentCommand::canExecute()) {
        return false;
    }

    DocumentModel* model = controller()->getDocumentModel();
    if (model == nullptr) {
        return false;
    }

    if (m_closeCurrent) {
        return !model->isEmpty();
    }
    return model->isValidIndex(m_index);
}

// SaveDocumentAsCommand implementation
SaveDocumentAsCommand::SaveDocumentAsCommand(DocumentController* controller,
                                             const QString& targetPath,
                                             QObject* parent)
    : DocumentCommand(controller, "Save Document As", parent),
      m_targetPath(targetPath),
      m_promptUser(targetPath.isEmpty()) {
    setDescription("Save document as new file");
    setActionId(ActionMap::saveFile);
}

bool SaveDocumentAsCommand::execute() {
    if (controller() == nullptr) {
        setErrorMessage("DocumentController is null");
        m_logger.error("DocumentController is null");
        emit executed(false);
        return false;
    }

    DocumentModel* model = controller()->getDocumentModel();
    if (model == nullptr || model->isEmpty()) {
        setErrorMessage("No document open to save");
        m_logger.error("No document open to save");
        emit executed(false);
        return false;
    }

    try {
        // Get current document path
        QString currentPath = model->getCurrentFilePath();
        if (currentPath.isEmpty()) {
            setErrorMessage("Current document has no file path");
            m_logger.error("Current document has no file path");
            emit executed(false);
            return false;
        }

        QString savePath = m_targetPath;

        // If no output path provided, prompt user
        if (m_promptUser || savePath.isEmpty()) {
            QFileInfo currentFile(currentPath);
            QString defaultName =
                currentFile.baseName() + "_copy." + currentFile.suffix();

            savePath = QFileDialog::getSaveFileName(
                nullptr, "Save Document Copy", defaultName,
                "PDF Files (*.pdf);;All Files (*)");

            if (savePath.isEmpty()) {
                m_logger.debug("User cancelled save operation");
                emit executed(false);
                return false;
            }
        }

        // Copy the file
        if (QFile::copy(currentPath, savePath)) {
            m_logger.info(QString("Successfully saved document copy to: %1")
                              .arg(savePath));
            emit statusMessage(
                QString("Saved copy: %1").arg(QFileInfo(savePath).fileName()));
            emit executed(true);
            return true;
        }
        setErrorMessage(
            QString("Failed to save document copy to: %1").arg(savePath));
        m_logger.error(
            QString("Failed to save document copy to: %1").arg(savePath));
        emit executed(false);
        return false;

    } catch (const std::exception& e) {
        setErrorMessage(
            QString("Exception while saving document: %1").arg(e.what()));
        m_logger.error(
            QString("Exception while saving document: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool SaveDocumentAsCommand::canExecute() const {
    if (!DocumentCommand::canExecute()) {
        return false;
    }

    DocumentModel* model = controller()->getDocumentModel();
    return model != nullptr && !model->isEmpty();
}

// ExportDocumentCommand implementation
ExportDocumentCommand::ExportDocumentCommand(DocumentController* controller,
                                             ExportFormat format,
                                             QObject* parent)
    : DocumentCommand(controller, "Export Document", parent), m_format(format) {
    QString formatStr;
    switch (format) {
        case PDF:
            formatStr = "PDF";
            break;
        case Images:
            formatStr = "Images";
            break;
        case Text:
            formatStr = "Text";
            break;
        case HTML:
            formatStr = "HTML";
            break;
    }
    setDescription(QString("Export document as %1").arg(formatStr));
    setActionId(ActionMap::exportFile);
}

// Helper method to export PDF (simple file copy)
bool ExportDocumentCommand::exportToPDF(Poppler::Document* document,
                                        const QString& sourcePath,
                                        const QString& outputPath) {
    Q_UNUSED(document);

    try {
        // Validate paths
        if (sourcePath.isEmpty() || outputPath.isEmpty()) {
            m_logger.error("Invalid source or output path for PDF export");
            return false;
        }

        // Check if source file exists
        if (!QFile::exists(sourcePath)) {
            m_logger.error(
                QString("Source file does not exist: %1").arg(sourcePath));
            return false;
        }

        // Remove destination file if it exists
        if (QFile::exists(outputPath)) {
            if (!QFile::remove(outputPath)) {
                m_logger.error(QString("Failed to remove existing file: %1")
                                   .arg(outputPath));
                return false;
            }
        }

        // Copy file
        if (!QFile::copy(sourcePath, outputPath)) {
            m_logger.error(QString("Failed to copy PDF from %1 to %2")
                               .arg(sourcePath)
                               .arg(outputPath));
            return false;
        }

        m_logger.info(
            QString("Successfully exported PDF to: %1").arg(outputPath));
        return true;

    } catch (const std::exception& e) {
        m_logger.error(
            QString("Exception during PDF export: %1").arg(e.what()));
        return false;
    }
}

// Helper method to export to images
bool ExportDocumentCommand::exportToImages(Poppler::Document* document,
                                           const QString& outputPath,
                                           int totalPages) {
    try {
        // Determine output format from file extension
        QFileInfo fileInfo(outputPath);
        QString extension = fileInfo.suffix().toLower();
        const char* format = "PNG";  // Default

        if (extension == "jpg" || extension == "jpeg") {
            format = "JPEG";
        } else if (extension == "png") {
            format = "PNG";
        } else {
            m_logger.warning(
                QString("Unknown image format '%1', using PNG").arg(extension));
        }

        // Get options for page range
        int startPage = 0;
        int endPage = totalPages - 1;

        if (m_options.contains("startPage")) {
            startPage = m_options["startPage"].toInt();
        }
        if (m_options.contains("endPage")) {
            endPage = m_options["endPage"].toInt();
        }

        // Validate page range
        startPage = qBound(0, startPage, totalPages - 1);
        endPage = qBound(startPage, endPage, totalPages - 1);

        // Get DPI from options or use default
        double dpi = 150.0;  // Default DPI for export
        if (m_options.contains("dpi")) {
            dpi = m_options["dpi"].toDouble();
            dpi = qBound(72.0, dpi, 600.0);  // Limit DPI range
        }

        m_logger.info(QString("Exporting pages %1-%2 to %3 at %4 DPI")
                          .arg(startPage + 1)
                          .arg(endPage + 1)
                          .arg(format)
                          .arg(dpi));

        // Export each page
        int pageCount = endPage - startPage + 1;
        for (int pageIndex = startPage; pageIndex <= endPage; ++pageIndex) {
            // Emit progress
            emit this->progress(pageIndex - startPage + 1, pageCount);
            emit statusMessage(QString("Exporting page %1 of %2...")
                                   .arg(pageIndex - startPage + 1)
                                   .arg(pageCount));

            // Get page
            std::unique_ptr<Poppler::Page> page(document->page(pageIndex));
            if (!page) {
                m_logger.warning(QString("Failed to load page %1, skipping")
                                     .arg(pageIndex + 1));
                continue;
            }

            // Render page to image
            QImage pageImage = page->renderToImage(dpi, dpi);
            if (pageImage.isNull()) {
                m_logger.warning(QString("Failed to render page %1, skipping")
                                     .arg(pageIndex + 1));
                continue;
            }

            // Generate output filename
            QString pageOutputPath;
            if (pageCount == 1) {
                // Single page - use the provided path directly
                pageOutputPath = outputPath;
            } else {
                // Multiple pages - append page number
                QString baseName = fileInfo.completeBaseName();
                QString dirPath = fileInfo.absolutePath();
                pageOutputPath = QString("%1/%2_page_%3.%4")
                                     .arg(dirPath)
                                     .arg(baseName)
                                     .arg(pageIndex + 1, 4, 10, QChar('0'))
                                     .arg(extension);
            }

            // Save image
            if (!pageImage.save(pageOutputPath, format)) {
                m_logger.error(
                    QString("Failed to save image: %1").arg(pageOutputPath));
                return false;
            }

            m_logger.debug(QString("Exported page %1 to: %2")
                               .arg(pageIndex + 1)
                               .arg(pageOutputPath));
        }

        m_logger.info(
            QString("Successfully exported %1 pages to images").arg(pageCount));
        return true;

    } catch (const std::exception& e) {
        m_logger.error(
            QString("Exception during image export: %1").arg(e.what()));
        return false;
    }
}

// Helper method to export to text
bool ExportDocumentCommand::exportToText(Poppler::Document* document,
                                         const QString& outputPath,
                                         int totalPages) {
    try {
        // Open output file
        QFile outputFile(outputPath);
        if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            m_logger.error(
                QString("Failed to open output file: %1").arg(outputPath));
            return false;
        }

        QTextStream out(&outputFile);
        out.setEncoding(QStringConverter::Utf8);

        // Get page range from options
        int startPage = 0;
        int endPage = totalPages - 1;

        if (m_options.contains("startPage")) {
            startPage = m_options["startPage"].toInt();
        }
        if (m_options.contains("endPage")) {
            endPage = m_options["endPage"].toInt();
        }

        // Validate page range
        startPage = qBound(0, startPage, totalPages - 1);
        endPage = qBound(startPage, endPage, totalPages - 1);

        m_logger.info(QString("Extracting text from pages %1-%2")
                          .arg(startPage + 1)
                          .arg(endPage + 1));

        // Extract text from each page
        int pageCount = endPage - startPage + 1;
        for (int pageIndex = startPage; pageIndex <= endPage; ++pageIndex) {
            // Emit progress
            emit this->progress(pageIndex - startPage + 1, pageCount);
            emit statusMessage(QString("Extracting text from page %1 of %2...")
                                   .arg(pageIndex - startPage + 1)
                                   .arg(pageCount));

            // Get page
            std::unique_ptr<Poppler::Page> page(document->page(pageIndex));
            if (!page) {
                m_logger.warning(QString("Failed to load page %1, skipping")
                                     .arg(pageIndex + 1));
                out << QString("[Page %1: Failed to load]\n\n")
                           .arg(pageIndex + 1);
                continue;
            }

            // Extract text
            QString pageText = page->text(QRectF());
            if (pageText.isEmpty()) {
                m_logger.debug(
                    QString("Page %1 has no text").arg(pageIndex + 1));
                out << QString("[Page %1: No text content]\n\n")
                           .arg(pageIndex + 1);
            } else {
                // Write page header and text
                out << QString("========== Page %1 ==========\n")
                           .arg(pageIndex + 1);
                out << pageText;
                out << "\n\n";
            }
        }

        outputFile.close();
        m_logger.info(
            QString("Successfully exported text to: %1").arg(outputPath));
        return true;

    } catch (const std::exception& e) {
        m_logger.error(
            QString("Exception during text export: %1").arg(e.what()));
        return false;
    }
}

// Helper method to export to HTML
bool ExportDocumentCommand::exportToHTML(Poppler::Document* document,
                                         const QString& outputPath,
                                         int totalPages) {
    try {
        // Open output file
        QFile outputFile(outputPath);
        if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            m_logger.error(
                QString("Failed to open output file: %1").arg(outputPath));
            return false;
        }

        QTextStream out(&outputFile);
        out.setEncoding(QStringConverter::Utf8);

        // Get page range from options
        int startPage = 0;
        int endPage = totalPages - 1;

        if (m_options.contains("startPage")) {
            startPage = m_options["startPage"].toInt();
        }
        if (m_options.contains("endPage")) {
            endPage = m_options["endPage"].toInt();
        }

        // Validate page range
        startPage = qBound(0, startPage, totalPages - 1);
        endPage = qBound(startPage, endPage, totalPages - 1);

        m_logger.info(QString("Exporting to HTML from pages %1-%2")
                          .arg(startPage + 1)
                          .arg(endPage + 1));

        // Get document metadata
        QString title = document->info("Title");
        QString author = document->info("Author");
        if (title.isEmpty()) {
            title = "PDF Document";
        }

        // Write HTML header
        out << "<!DOCTYPE html>\n";
        out << "<html lang=\"en\">\n";
        out << "<head>\n";
        out << "    <meta charset=\"UTF-8\">\n";
        out << R"(    <meta name="viewport" content="width=device-width, initial-scale=1.0">)"
            << "\n";
        out << "    <title>" << title.toHtmlEscaped() << "</title>\n";
        if (!author.isEmpty()) {
            out << R"(    <meta name="author" content=")"
                << author.toHtmlEscaped() << "\">\n";
        }
        out << "    <style>\n";
        out << "        body {\n";
        out << R"(            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;)"
            << "\n";
        out << "            max-width: 900px;\n";
        out << "            margin: 0 auto;\n";
        out << "            padding: 20px;\n";
        out << "            line-height: 1.6;\n";
        out << "            background-color: #f5f5f5;\n";
        out << "        }\n";
        out << "        .page {\n";
        out << "            background-color: white;\n";
        out << "            padding: 40px;\n";
        out << "            margin-bottom: 30px;\n";
        out << "            box-shadow: 0 2px 8px rgba(0,0,0,0.1);\n";
        out << "            border-radius: 4px;\n";
        out << "        }\n";
        out << "        .page-header {\n";
        out << "            color: #333;\n";
        out << "            border-bottom: 2px solid #007bff;\n";
        out << "            padding-bottom: 10px;\n";
        out << "            margin-bottom: 20px;\n";
        out << "            font-size: 1.2em;\n";
        out << "            font-weight: bold;\n";
        out << "        }\n";
        out << "        .page-content {\n";
        out << "            white-space: pre-wrap;\n";
        out << "            word-wrap: break-word;\n";
        out << "            color: #333;\n";
        out << "        }\n";
        out << "        .no-content {\n";
        out << "            color: #999;\n";
        out << "            font-style: italic;\n";
        out << "        }\n";
        out << "        h1 {\n";
        out << "            color: #007bff;\n";
        out << "            border-bottom: 3px solid #007bff;\n";
        out << "            padding-bottom: 10px;\n";
        out << "        }\n";
        out << "        .metadata {\n";
        out << "            background-color: #e9ecef;\n";
        out << "            padding: 15px;\n";
        out << "            border-radius: 4px;\n";
        out << "            margin-bottom: 30px;\n";
        out << "        }\n";
        out << "    </style>\n";
        out << "</head>\n";
        out << "<body>\n";
        out << "    <h1>" << title.toHtmlEscaped() << "</h1>\n";

        // Write metadata section
        out << "    <div class=\"metadata\">\n";
        out << "        <strong>Document Information:</strong><br>\n";
        if (!author.isEmpty()) {
            out << "        Author: " << author.toHtmlEscaped() << "<br>\n";
        }
        out << "        Total Pages: " << totalPages << "<br>\n";
        out << "        Exported Pages: " << (endPage - startPage + 1)
            << " (Page " << (startPage + 1) << " to " << (endPage + 1)
            << ")<br>\n";
        out << "        Export Date: "
            << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
        out << "    </div>\n\n";

        // Extract and write content from each page
        int pageCount = endPage - startPage + 1;
        for (int pageIndex = startPage; pageIndex <= endPage; ++pageIndex) {
            // Emit progress
            emit this->progress(pageIndex - startPage + 1, pageCount);
            emit statusMessage(QString("Exporting page %1 of %2 to HTML...")
                                   .arg(pageIndex - startPage + 1)
                                   .arg(pageCount));

            // Get page
            std::unique_ptr<Poppler::Page> page(document->page(pageIndex));
            if (!page) {
                m_logger.warning(QString("Failed to load page %1, skipping")
                                     .arg(pageIndex + 1));
                out << "    <div class=\"page\">\n";
                out << "        <div class=\"page-header\">Page "
                    << (pageIndex + 1) << "</div>\n";
                out << "        <div class=\"page-content no-content\">[Failed "
                       "to load page]</div>\n";
                out << "    </div>\n\n";
                continue;
            }

            // Extract text
            QString pageText = page->text(QRectF());

            // Write page div
            out << "    <div class=\"page\">\n";
            out << "        <div class=\"page-header\">Page " << (pageIndex + 1)
                << "</div>\n";

            if (pageText.isEmpty()) {
                m_logger.debug(
                    QString("Page %1 has no text").arg(pageIndex + 1));
                out << "        <div class=\"page-content no-content\">[No "
                       "text "
                       "content on this page]</div>\n";
            } else {
                // Escape HTML and write content
                out << "        <div class=\"page-content\">"
                    << pageText.toHtmlEscaped() << "</div>\n";
            }

            out << "    </div>\n\n";
        }

        // Write HTML footer
        out << "</body>\n";
        out << "</html>\n";

        outputFile.close();
        m_logger.info(
            QString("Successfully exported to HTML: %1").arg(outputPath));
        return true;

    } catch (const std::exception& e) {
        m_logger.error(
            QString("Exception during HTML export: %1").arg(e.what()));
        return false;
    }
}

bool ExportDocumentCommand::execute() {
    if (controller() == nullptr) {
        setErrorMessage("DocumentController is null");
        m_logger.error("DocumentController is null");
        emit executed(false);
        return false;
    }

    DocumentModel* model = controller()->getDocumentModel();
    if (model == nullptr || model->isEmpty()) {
        setErrorMessage("No document open to export");
        m_logger.error("No document open to export");
        emit executed(false);
        return false;
    }

    try {
        QString outputPath = m_outputPath;

        // If no output path provided, prompt user
        if (outputPath.isEmpty()) {
            QString currentPath = model->getCurrentFilePath();
            QFileInfo currentFile(currentPath);

            QString filter;
            QString defaultExt;
            switch (m_format) {
                case PDF:
                    filter = "PDF Files (*.pdf)";
                    defaultExt = "pdf";
                    break;
                case Images:
                    filter = "PNG Files (*.png);;JPEG Files (*.jpg)";
                    defaultExt = "png";
                    break;
                case Text:
                    filter = "Text Files (*.txt)";
                    defaultExt = "txt";
                    break;
                case HTML:
                    filter = "HTML Files (*.html)";
                    defaultExt = "html";
                    break;
            }

            QString defaultName = currentFile.baseName() + "." + defaultExt;

            outputPath = QFileDialog::getSaveFileName(
                nullptr, "Export Document", defaultName, filter);

            if (outputPath.isEmpty()) {
                m_logger.debug("User cancelled export operation");
                emit executed(false);
                return false;
            }
        }

        // Get the Poppler document
        Poppler::Document* document = model->getCurrentDocument();
        if (document == nullptr) {
            setErrorMessage("Failed to get document for export");
            m_logger.error("Document pointer is null");
            emit executed(false);
            return false;
        }

        int totalPages = document->numPages();
        if (totalPages == 0) {
            setErrorMessage("Document has no pages to export");
            m_logger.error("Document has no pages");
            emit executed(false);
            return false;
        }

        // Perform export based on format
        bool success = false;
        QString formatStr;

        switch (m_format) {
            case PDF:
                formatStr = "PDF";
                success = exportToPDF(document, model->getCurrentFilePath(),
                                      outputPath);
                break;
            case Images:
                formatStr = "Images";
                success = exportToImages(document, outputPath, totalPages);
                break;
            case Text:
                formatStr = "Text";
                success = exportToText(document, outputPath, totalPages);
                break;
            case HTML:
                formatStr = "HTML";
                success = exportToHTML(document, outputPath, totalPages);
                break;
        }

        if (success) {
            m_logger.info(QString("Successfully exported to %1: %2")
                              .arg(formatStr)
                              .arg(outputPath));
            emit statusMessage(
                QString("Successfully exported to %1").arg(formatStr));
            emit executed(true);
            return true;
        }
        setErrorMessage(QString("Failed to export to %1").arg(formatStr));
        emit executed(false);
        return false;

    } catch (const std::exception& e) {
        setErrorMessage(
            QString("Exception while exporting document: %1").arg(e.what()));
        m_logger.error(
            QString("Exception while exporting document: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool ExportDocumentCommand::canExecute() const {
    if (!DocumentCommand::canExecute()) {
        return false;
    }

    DocumentModel* model = controller()->getDocumentModel();
    return model != nullptr && !model->isEmpty();
}

// PrintDocumentCommand implementation
PrintDocumentCommand::PrintDocumentCommand(DocumentController* controller,
                                           QObject* parent)
    : DocumentCommand(controller, "Print Document", parent) {
    setDescription("Print document");
    setActionId(ActionMap::printFile);
}

void PrintDocumentCommand::setPageRange(int start, int end) {
    m_startPage = start;
    m_endPage = end;
}

bool PrintDocumentCommand::execute() {
    if (controller() == nullptr) {
        setErrorMessage("DocumentController is null");
        m_logger.error("DocumentController is null");
        emit executed(false);
        return false;
    }

    DocumentModel* model = controller()->getDocumentModel();
    if (model == nullptr || model->isEmpty()) {
        setErrorMessage("No document open to print");
        m_logger.error("No document open to print");
        emit executed(false);
        return false;
    }

    try {
        // Get the Poppler document
        Poppler::Document* document = model->getCurrentDocument();
        if (document == nullptr) {
            setErrorMessage("Failed to get document for printing");
            m_logger.error("Document pointer is null");
            emit executed(false);
            return false;
        }

        int totalPages = document->numPages();
        if (totalPages == 0) {
            setErrorMessage("Document has no pages to print");
            m_logger.error("Document has no pages");
            emit executed(false);
            return false;
        }

        // Create printer
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::NativeFormat);
        printer.setColorMode(QPrinter::Color);

        // Set page range from options or use all pages
        int startPage = (m_startPage >= 0) ? m_startPage : 0;
        int endPage = (m_endPage >= 0 && m_endPage < totalPages)
                          ? m_endPage
                          : totalPages - 1;

        // Validate page range
        if (startPage > endPage || startPage < 0 || endPage >= totalPages) {
            setErrorMessage(
                QString("Invalid page range: %1-%2 (document has %3 pages)")
                    .arg(startPage + 1)
                    .arg(endPage + 1)
                    .arg(totalPages));
            m_logger.error(QString("Invalid page range: %1-%2")
                               .arg(startPage)
                               .arg(endPage));
            emit executed(false);
            return false;
        }

        // Apply print options from m_printOptions if provided
        if (m_printOptions.contains("copies")) {
            printer.setCopyCount(m_printOptions["copies"].toInt());
        }
        if (m_printOptions.contains("orientation")) {
            QString orientation = m_printOptions["orientation"].toString();
            if (orientation == "landscape") {
                printer.setPageOrientation(QPageLayout::Landscape);
            } else {
                printer.setPageOrientation(QPageLayout::Portrait);
            }
        }
        if (m_printOptions.contains("colorMode")) {
            QString colorMode = m_printOptions["colorMode"].toString();
            if (colorMode == "grayscale") {
                printer.setColorMode(QPrinter::GrayScale);
            }
        }

        // Show print dialog
        QPrintDialog printDialog(&printer, nullptr);
        printDialog.setWindowTitle(tr("Print Document"));

        // Set page range in dialog
        printDialog.setMinMax(1, totalPages);
        if (startPage >= 0 && endPage >= 0) {
            printDialog.setFromTo(startPage + 1, endPage + 1);
        }

        if (printDialog.exec() != QDialog::Accepted) {
            m_logger.debug("User cancelled print operation");
            emit executed(false);
            return false;
        }

        // Get updated page range from dialog
        if (printDialog.printRange() == QAbstractPrintDialog::PageRange) {
            startPage = printDialog.fromPage() - 1;
            endPage = printDialog.toPage() - 1;
        }

        m_logger.info(QString("Printing pages %1-%2 of %3")
                          .arg(startPage + 1)
                          .arg(endPage + 1)
                          .arg(totalPages));

        // Start printing
        QPainter painter;
        if (!painter.begin(&printer)) {
            setErrorMessage("Failed to start printing");
            m_logger.error("QPainter::begin() failed");
            emit executed(false);
            return false;
        }

        // Configure rendering for high quality
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        // Print each page
        int pageCount = endPage - startPage + 1;
        for (int pageIndex = startPage; pageIndex <= endPage; ++pageIndex) {
            // Emit progress
            emit this->progress(pageIndex - startPage + 1, pageCount);
            emit statusMessage(QString("Printing page %1 of %2...")
                                   .arg(pageIndex - startPage + 1)
                                   .arg(pageCount));

            // Get page
            std::unique_ptr<Poppler::Page> page(document->page(pageIndex));
            if (!page) {
                m_logger.warning(QString("Failed to load page %1, skipping")
                                     .arg(pageIndex + 1));
                continue;
            }

            // Render page to image at high resolution (300 DPI)
            QImage pageImage = page->renderToImage(300.0, 300.0);
            if (pageImage.isNull()) {
                m_logger.warning(QString("Failed to render page %1, skipping")
                                     .arg(pageIndex + 1));
                continue;
            }

            // Scale image to fit printer page
            QRect pageRect = printer.pageRect(QPrinter::DevicePixel).toRect();
            QImage scaledImage = pageImage.scaled(
                pageRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

            // Center image on page
            int horizontalOffset = (pageRect.width() - scaledImage.width()) / 2;
            int verticalOffset = (pageRect.height() - scaledImage.height()) / 2;

            painter.drawImage(horizontalOffset, verticalOffset, scaledImage);

            // New page for next iteration (except for last page)
            if (pageIndex < endPage) {
                if (!printer.newPage()) {
                    setErrorMessage(
                        QString("Failed to create new page at page %1")
                            .arg(pageIndex + 2));
                    m_logger.error(
                        QString("QPrinter::newPage() failed at page %1")
                            .arg(pageIndex + 1));
                    painter.end();
                    emit executed(false);
                    return false;
                }
            }
        }

        painter.end();

        m_logger.info(QString("Successfully printed %1 pages").arg(pageCount));
        emit statusMessage(
            QString("Successfully printed %1 pages").arg(pageCount));
        emit executed(true);
        return true;

    } catch (const std::exception& e) {
        setErrorMessage(
            QString("Exception while printing document: %1").arg(e.what()));
        m_logger.error(
            QString("Exception while printing document: %1").arg(e.what()));
        emit executed(false);
        return false;
    } catch (...) {
        setErrorMessage("Unknown exception occurred during printing");
        m_logger.error("Unknown exception during printing");
        emit executed(false);
        return false;
    }
}

bool PrintDocumentCommand::canExecute() const {
    if (!DocumentCommand::canExecute()) {
        return false;
    }

    DocumentModel* model = controller()->getDocumentModel();
    return model != nullptr && !model->isEmpty();
}

// ReloadDocumentCommand implementation
ReloadDocumentCommand::ReloadDocumentCommand(DocumentController* controller,
                                             QObject* parent)
    : DocumentCommand(controller, "Reload Document", parent) {
    setDescription("Reload current document");
    setActionId(ActionMap::reloadFile);
}

bool ReloadDocumentCommand::execute() {
    if (controller() == nullptr) {
        setErrorMessage("DocumentController is null");
        m_logger.error("DocumentController is null");
        emit executed(false);
        return false;
    }

    DocumentModel* model = controller()->getDocumentModel();
    if (model == nullptr || model->isEmpty()) {
        setErrorMessage("No document open to reload");
        m_logger.error("No document open to reload");
        emit executed(false);
        return false;
    }

    try {
        QString currentPath = model->getCurrentFilePath();
        if (currentPath.isEmpty()) {
            setErrorMessage("Current document has no file path");
            m_logger.error("Current document has no file path");
            emit executed(false);
            return false;
        }

        // Emit the documentReloadRequested signal which will be handled by
        // ApplicationController ApplicationController will preserve the current
        // page and zoom state from ViewWidget, close the document, reopen it,
        // and restore the state
        emit controller() -> documentReloadRequested(currentPath, 0, 1.0);

        m_logger.info(
            QString("Document reload requested: %1").arg(currentPath));
        emit statusMessage(
            QString("Reloading: %1").arg(QFileInfo(currentPath).fileName()));
        emit executed(true);
        return true;

    } catch (const std::exception& e) {
        setErrorMessage(
            QString("Exception while reloading document: %1").arg(e.what()));
        m_logger.error(
            QString("Exception while reloading document: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool ReloadDocumentCommand::canExecute() const {
    if (!DocumentCommand::canExecute()) {
        return false;
    }

    DocumentModel* model = controller()->getDocumentModel();
    return model != nullptr && !model->isEmpty();
}

// DocumentMacroCommand implementation
DocumentMacroCommand::DocumentMacroCommand(DocumentController* controller,
                                           const QString& name, QObject* parent)
    : DocumentCommand(controller, name, parent) {
    setDescription(QString("Macro command: %1").arg(name));
}

DocumentMacroCommand::~DocumentMacroCommand() { clearCommands(); }

void DocumentMacroCommand::addCommand(DocumentCommand* command) {
    if (command != nullptr) {
        m_commands.append(command);
        command->setParent(this);  // Take ownership
        m_logger.debug(
            QString("Added command to macro: %1").arg(command->name()));
    }
}

void DocumentMacroCommand::clearCommands() {
    // Delete all commands since we own them
    qDeleteAll(m_commands);
    m_commands.clear();
    m_executedCommands.clear();
    m_logger.debug("Cleared all commands from macro");
}

bool DocumentMacroCommand::execute() {
    if (controller() == nullptr) {
        setErrorMessage("DocumentController is null");
        m_logger.error("DocumentController is null");
        emit executed(false);
        return false;
    }

    if (m_commands.isEmpty()) {
        setErrorMessage("No commands to execute in macro");
        m_logger.warning("No commands to execute in macro");
        emit executed(false);
        return false;
    }

    m_executedCommands.clear();

    try {
        qsizetype totalCommands = m_commands.size();
        emit progress(0, static_cast<int>(totalCommands));

        for (qsizetype i = 0; i < m_commands.size(); ++i) {
            DocumentCommand* command = m_commands[i];

            if (!command->canExecute()) {
                setErrorMessage(QString("Command %1 cannot be executed: %2")
                                    .arg(i + 1)
                                    .arg(command->name()));
                m_logger.error(QString("Command %1 cannot be executed: %2")
                                   .arg(i + 1)
                                   .arg(command->name()));

                // Undo previously executed commands
                undo();
                emit executed(false);
                return false;
            }

            bool success = command->execute();

            if (success) {
                m_executedCommands.append(command);
                m_logger.debug(QString("Executed command %1/%2: %3")
                                   .arg(i + 1)
                                   .arg(totalCommands)
                                   .arg(command->name()));
            } else {
                setErrorMessage(QString("Command %1 failed: %2 - %3")
                                    .arg(i + 1)
                                    .arg(command->name())
                                    .arg(command->errorMessage()));
                m_logger.error(QString("Command %1 failed: %2")
                                   .arg(i + 1)
                                   .arg(command->name()));

                // Undo previously executed commands
                undo();
                emit executed(false);
                return false;
            }

            emit progress(static_cast<int>(i + 1),
                          static_cast<int>(totalCommands));
        }

        m_logger.info(QString("Successfully executed macro with %1 commands")
                          .arg(totalCommands));
        emit statusMessage(QString("Executed macro: %1 (%2 commands)")
                               .arg(name())
                               .arg(totalCommands));
        emit executed(true);
        return true;

    } catch (const std::exception& e) {
        setErrorMessage(
            QString("Exception while executing macro: %1").arg(e.what()));
        m_logger.error(
            QString("Exception while executing macro: %1").arg(e.what()));

        // Undo previously executed commands
        undo();
        emit executed(false);
        return false;
    }
}

bool DocumentMacroCommand::canExecute() const {
    if (!DocumentCommand::canExecute()) {
        return false;
    }

    if (m_commands.isEmpty()) {
        return false;
    }

    // Check if all commands can be executed
    return std::all_of(
        m_commands.begin(), m_commands.end(),
        [](const DocumentCommand* command) { return command->canExecute(); });
}

bool DocumentMacroCommand::undo() {
    if (m_executedCommands.isEmpty()) {
        m_logger.debug("No commands to undo in macro");
        return true;
    }

    try {
        // Undo commands in reverse order
        bool allUndoSuccessful = true;

        for (qsizetype i = m_executedCommands.size() - 1; i >= 0; --i) {
            DocumentCommand* command = m_executedCommands[i];

            bool undoSuccess = command->undo();
            if (!undoSuccess) {
                m_logger.warning(
                    QString("Failed to undo command: %1").arg(command->name()));
                allUndoSuccessful = false;
            } else {
                m_logger.debug(
                    QString("Undid command: %1").arg(command->name()));
            }
        }

        m_executedCommands.clear();

        if (allUndoSuccessful) {
            m_logger.info("Successfully undid all commands in macro");
        } else {
            m_logger.warning("Some commands in macro could not be undone");
        }

        return allUndoSuccessful;

    } catch (const std::exception& e) {
        m_logger.error(
            QString("Exception while undoing macro: %1").arg(e.what()));
        return false;
    }
}

// DocumentCommandFactory implementation
std::unique_ptr<DocumentCommand> DocumentCommandFactory::createOpenCommand(
    DocumentController* controller, const QString& filePath) {
    if (filePath.isEmpty()) {
        return std::make_unique<OpenDocumentCommand>(controller);
    }
    return std::make_unique<OpenDocumentCommand>(controller, filePath);
}

std::unique_ptr<DocumentCommand>
DocumentCommandFactory::createOpenMultipleCommand(
    DocumentController* controller, const QStringList& filePaths) {
    return std::make_unique<OpenDocumentsCommand>(controller, filePaths);
}

std::unique_ptr<DocumentCommand> DocumentCommandFactory::createCloseCommand(
    DocumentController* controller, int index) {
    return std::make_unique<CloseDocumentCommand>(controller, index);
}

std::unique_ptr<DocumentCommand> DocumentCommandFactory::createSaveAsCommand(
    DocumentController* controller, const QString& targetPath) {
    return std::make_unique<SaveDocumentAsCommand>(controller, targetPath);
}

std::unique_ptr<DocumentCommand> DocumentCommandFactory::createExportCommand(
    DocumentController* controller,
    ExportDocumentCommand::ExportFormat format) {
    return std::make_unique<ExportDocumentCommand>(controller, format);
}

std::unique_ptr<DocumentCommand> DocumentCommandFactory::createPrintCommand(
    DocumentController* controller) {
    return std::make_unique<PrintDocumentCommand>(controller);
}

std::unique_ptr<DocumentCommand> DocumentCommandFactory::createReloadCommand(
    DocumentController* controller) {
    return std::make_unique<ReloadDocumentCommand>(controller);
}

std::unique_ptr<DocumentMacroCommand>
DocumentCommandFactory::createMacroCommand(DocumentController* controller,
                                           const QString& name) {
    return std::make_unique<DocumentMacroCommand>(controller, name);
}

std::unique_ptr<DocumentCommand> DocumentCommandFactory::createCommandFromType(
    const QString& type, DocumentController* controller) {
    if (type == "open") {
        return createOpenCommand(controller);
    }
    if (type == "open-multiple") {
        return createOpenMultipleCommand(controller);
    }
    if (type == "close") {
        return createCloseCommand(controller);
    }
    if (type == "close-current") {
        return createCloseCommand(controller, -1);
    }
    if (type == "save-as") {
        return createSaveAsCommand(controller);
    }
    if (type == "export-pdf") {
        return createExportCommand(controller, ExportDocumentCommand::PDF);
    }
    if (type == "export-images") {
        return createExportCommand(controller, ExportDocumentCommand::Images);
    }
    if (type == "export-text") {
        return createExportCommand(controller, ExportDocumentCommand::Text);
    }
    if (type == "export-html") {
        return createExportCommand(controller, ExportDocumentCommand::HTML);
    }
    if (type == "print") {
        return createPrintCommand(controller);
    }
    if (type == "reload") {
        return createReloadCommand(controller);
    }

    return nullptr;
}

// ShowDocumentPropertiesCommand implementation
ShowDocumentPropertiesCommand::ShowDocumentPropertiesCommand(
    DocumentController* controller, QWidget* parentWidget, QObject* parent)
    : DocumentCommand(controller, "Show Document Properties", parent),
      m_parentWidget(parentWidget) {
    setDescription("Display document metadata and properties");
}

bool ShowDocumentPropertiesCommand::execute() {
    if (controller() == nullptr) {
        m_logger.error("No document controller available");
        return false;
    }

    // Get current document through document model
    auto* documentModel = controller()->getDocumentModel();
    if (documentModel == nullptr) {
        m_logger.error("No document model available");
        return false;
    }

    auto* document = documentModel->getCurrentDocument();
    if (document == nullptr) {
        m_logger.warning("No document available to show properties");
        return false;
    }

    try {
        // Create and show document metadata dialog
        auto* dialog = new DocumentMetadataDialog(m_parentWidget);
        dialog->setDocument(document, documentModel->getCurrentFilePath());
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();

        m_logger.info("Document properties dialog shown");
        emit executed(true);
        return true;
    } catch (const std::exception& e) {
        m_logger.error(
            QString("Failed to show document properties: %1").arg(e.what()));
        setErrorMessage(
            QString("Failed to show document properties: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool ShowDocumentPropertiesCommand::canExecute() const {
    if (controller() == nullptr) {
        return false;
    }
    auto* documentModel = controller()->getDocumentModel();
    return documentModel != nullptr &&
           documentModel->getCurrentDocument() != nullptr;
}

// CompareDocumentsCommand implementation
CompareDocumentsCommand::CompareDocumentsCommand(DocumentController* controller,
                                                 QString firstPath,
                                                 QString secondPath,
                                                 QObject* parent)
    : DocumentCommand(controller, "Compare Documents", parent),
      m_firstPath(std::move(firstPath)),
      m_secondPath(std::move(secondPath)) {
    setDescription("Compare two documents side by side");
}

bool CompareDocumentsCommand::execute() {
    if (controller() == nullptr) {
        m_logger.error("No document controller available");
        return false;
    }

    try {
        // Create and show document comparison dialog
        auto* dialog = new DocumentComparison(nullptr);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();

        m_logger.info("Document comparison dialog shown");
        emit executed(true);
        return true;
    } catch (const std::exception& e) {
        m_logger.error(
            QString("Failed to open document comparison: %1").arg(e.what()));
        setErrorMessage(
            QString("Failed to open document comparison: %1").arg(e.what()));
        emit executed(false);
        return false;
    }
}

bool CompareDocumentsCommand::canExecute() const {
    return controller() != nullptr;
}
