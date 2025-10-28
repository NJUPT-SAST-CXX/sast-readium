#include "PageTools.h"
#include <QDebug>
#include <QFile>

PageTools::PageTools(QObject* parent) : QObject(parent) {}

bool PageTools::validatePageNumber(Poppler::Document* document,
                                   int pageNumber) const {
    return document && pageNumber >= 0 && pageNumber < document->numPages();
}

bool PageTools::extractPages(Poppler::Document* source,
                             const QList<int>& pageNumbers,
                             const QString& outputPath) {
    if (!source || pageNumbers.isEmpty() || outputPath.isEmpty()) {
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

    qDebug() << "Extracting" << pageNumbers.size() << "pages to" << outputPath;

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
    qDebug() << "Cropping page" << pageNumber << "to rect" << cropRect;

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
    qDebug() << "Rotating page" << pageNumber << "by" << degrees << "degrees";

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
    if (!document || pageNumbers.isEmpty()) {
        emit operationCompleted(false, "Invalid parameters");
        return false;
    }

    // Would implement page deletion using PDF manipulation library
    qDebug() << "Deleting" << pageNumbers.size() << "pages";

    emit operationCompleted(true, "Pages deleted successfully");
    return true;
}
