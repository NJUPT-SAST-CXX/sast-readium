#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QObject>
#include <QRectF>

/**
 * @brief Page extraction and crop tools (Feature 17)
 */
class PageTools : public QObject {
    Q_OBJECT

public:
    explicit PageTools(QObject* parent = nullptr);
    ~PageTools() = default;

    // Page extraction
    bool extractPages(Poppler::Document* source, const QList<int>& pageNumbers,
                      const QString& outputPath);
    bool extractPageRange(Poppler::Document* source, int startPage, int endPage,
                          const QString& outputPath);

    // Page cropping
    bool cropPage(Poppler::Document* document, int pageNumber,
                  const QRectF& cropRect);
    bool cropPages(Poppler::Document* document, const QList<int>& pageNumbers,
                   const QRectF& cropRect);

    // Page rotation
    bool rotatePage(Poppler::Document* document, int pageNumber, int degrees);
    bool rotatePages(Poppler::Document* document, const QList<int>& pageNumbers,
                     int degrees);

    // Page deletion
    bool deletePages(Poppler::Document* document,
                     const QList<int>& pageNumbers);

signals:
    void operationProgress(int current, int total);
    void operationCompleted(bool success, const QString& message);

private:
    bool validatePageNumber(Poppler::Document* document, int pageNumber) const;
};
