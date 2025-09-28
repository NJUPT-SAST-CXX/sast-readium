#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <memory>

namespace Poppler {
    class Document;
    class Page;
}

/**
 * Text extraction component for PDF documents
 * Handles efficient text extraction with caching
 */
class TextExtractor : public QObject
{
    Q_OBJECT

public:
    explicit TextExtractor(QObject* parent = nullptr);
    ~TextExtractor();

    // Document management
    void setDocument(Poppler::Document* document);
    void clearDocument();

    // Text extraction
    QString extractPageText(int pageNumber);
    QStringList extractPagesText(const QList<int>& pageNumbers);
    QString extractAllText();

    // Cache management
    void setCacheEnabled(bool enabled);
    [[nodiscard]] bool isCacheEnabled() const;
    void clearCache();
    [[nodiscard]] qint64 cacheMemoryUsage() const;

    // Prefetching
    void prefetchPages(const QList<int>& pageNumbers);
    void prefetchRange(int startPage, int endPage);

signals:
    void textExtracted(int pageNumber, const QString& text);
    void extractionProgress(int current, int total);
    void extractionError(int pageNumber, const QString& error);

private:
    class Implementation;
    std::unique_ptr<Implementation> m_d;
};
