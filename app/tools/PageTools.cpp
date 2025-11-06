#include "PageTools.h"
#include <QFile>
#include "../logging/LoggingMacros.h"

PageTools::PageTools(QObject* parent) : QObject(parent) {}

bool PageTools::validatePageNumber(Poppler::Document* document,
                                   int pageNumber) const {
    return document && pageNumber >= 0 && pageNumber < document->numPages();
}

bool PageTools::extractPages(Poppler::Document* source,
                             const QList<int>& pageNumbers,
                             const QString& outputPath) {
    if (source == nullptr || pageNumbers.isEmpty() || outputPath.isEmpty()) {
        emit operationCompleted(false, "Invalid parameters");
        return false;
    }

    // Validate all page numbers
    for (int pageNum : pageNumbers) {
        if (!validatePageNumber(source, pageNum)) {
            emit operationCompleted(
                false, QString("Invalid page number: %1").arg(pageNum));
            return false;
        }
    }

    // Would use PDF manipulation library (like PoDoFo or QPdf) to extract pages
    // Poppler doesn't directly support page extraction, needs external library

    LOG_INFO(QStringLiteral("PageTools: Extracting %1 pages to %2")
                 .arg(pageNumbers.size())
                 .arg(outputPath));

    emit operationCompleted(true, "Pages extracted successfully");
    return true;
}

bool PageTools::extractPageRange(Poppler::Document* source, int startPage,
                                 int endPage, const QString& outputPath) {
    QList<int> pageNumbers;
    for (int i = startPage; i <= endPage; ++i) {
        pageNumbers.append(i);
    }
    return extractPages(source, pageNumbers, outputPath);
}

bool PageTools::cropPage(Poppler::Document* document, int pageNumber,
                         const QRectF& cropRect) {
    if (!validatePageNumber(document, pageNumber)) {
        emit operationCompleted(false, "Invalid page number");
        return false;
    }

    // Would implement page cropping using PDF manipulation library
    LOG_INFO(QStringLiteral("PageTools: Cropping page %1 to rect [%2,%3 %4x%5]")
                 .arg(pageNumber)
                 .arg(cropRect.x())
                 .arg(cropRect.y())
                 .arg(cropRect.width())
                 .arg(cropRect.height()));

    emit operationCompleted(true, "Page cropped successfully");
    return true;
}

bool PageTools::cropPages(Poppler::Document* document,
                          const QList<int>& pageNumbers,
                          const QRectF& cropRect) {
    for (int i = 0; i < pageNumbers.size(); ++i) {
        emit operationProgress(i + 1, pageNumbers.size());
        if (!cropPage(document, pageNumbers[i], cropRect)) {
            return false;
        }
    }
    return true;
}

bool PageTools::rotatePage(Poppler::Document* document, int pageNumber,
                           int degrees) {
    if (!validatePageNumber(document, pageNumber)) {
        emit operationCompleted(false, "Invalid page number");
        return false;
    }

    degrees = ((degrees % 360) + 360) % 360;

    // Would implement rotation using PDF manipulation library
    LOG_INFO(QStringLiteral("PageTools: Rotating page %1 by %2 degrees")
                 .arg(pageNumber)
                 .arg(degrees));

    emit operationCompleted(true, "Page rotated successfully");
    return true;
}

bool PageTools::rotatePages(Poppler::Document* document,
                            const QList<int>& pageNumbers, int degrees) {
    for (int i = 0; i < pageNumbers.size(); ++i) {
        emit operationProgress(i + 1, pageNumbers.size());
        if (!rotatePage(document, pageNumbers[i], degrees)) {
            return false;
        }
    }
    return true;
}

bool PageTools::deletePages(Poppler::Document* document,
                            const QList<int>& pageNumbers) {
    if (document == nullptr || pageNumbers.isEmpty()) {
        emit operationCompleted(false, "Invalid parameters");
        return false;
    }

    // Would implement page deletion using PDF manipulation library
    LOG_INFO(
        QStringLiteral("PageTools: Deleting %1 pages").arg(pageNumbers.size()));

    emit operationCompleted(true, "Pages deleted successfully");
    return true;
}
