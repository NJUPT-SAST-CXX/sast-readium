#include "TextExtractor.h"
#include <poppler-qt6.h>
#include <QDebug>
#include <QRectF>
#include <QMutexLocker>

class TextExtractor::Implementation
{
public:
    Implementation(TextExtractor* q)
        : q_ptr(q)
        , document(nullptr)
        , cacheEnabled(true)
    {
    }

    QString extractPageTextInternal(int pageNumber)
    {
        if (!document || pageNumber < 0 || pageNumber >= document->numPages()) {
            qWarning() << "Invalid page number or document:" << pageNumber;
            return QString();
        }

        // Check cache first
        if (cacheEnabled) {
            QMutexLocker locker(&cacheMutex);
            if (textCache.contains(pageNumber)) {
                return textCache[pageNumber];
            }
        }

        // Extract text from page
        std::unique_ptr<Poppler::Page> page(document->page(pageNumber));
        if (!page) {
            emit q_ptr->extractionError(pageNumber, "Failed to load page");
            return QString();
        }

        QString text = page->text(QRectF());
        
        // Store in cache
        if (cacheEnabled && !text.isEmpty()) {
            QMutexLocker locker(&cacheMutex);
            textCache[pageNumber] = text;
        }

        emit q_ptr->textExtracted(pageNumber, text);
        return text;
    }

    qint64 calculateCacheMemoryUsage() const
    {
        QMutexLocker locker(&cacheMutex);
        qint64 totalSize = 0;
        for (const QString& text : textCache) {
            totalSize += text.size() * sizeof(QChar);
        }
        return totalSize;
    }

    TextExtractor* q_ptr;
    Poppler::Document* document;
    QHash<int, QString> textCache;
    mutable QMutex cacheMutex;
    bool cacheEnabled;
};

TextExtractor::TextExtractor(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Implementation>(this))
{
}

TextExtractor::~TextExtractor() = default;

void TextExtractor::setDocument(Poppler::Document* document)
{
    if (d->document != document) {
        clearCache();
        d->document = document;
    }
}

void TextExtractor::clearDocument()
{
    d->document = nullptr;
    clearCache();
}

QString TextExtractor::extractPageText(int pageNumber)
{
    return d->extractPageTextInternal(pageNumber);
}

QStringList TextExtractor::extractPagesText(const QList<int>& pageNumbers)
{
    QStringList texts;
    int total = pageNumbers.size();
    int current = 0;

    for (int pageNumber : pageNumbers) {
        texts.append(d->extractPageTextInternal(pageNumber));
        current++;
        emit extractionProgress(current, total);
    }

    return texts;
}

QString TextExtractor::extractAllText()
{
    if (!d->document) {
        return QString();
    }

    QString allText;
    int pageCount = d->document->numPages();

    for (int i = 0; i < pageCount; ++i) {
        allText.append(d->extractPageTextInternal(i));
        allText.append("\n\n");
        emit extractionProgress(i + 1, pageCount);
    }

    return allText;
}

void TextExtractor::setCacheEnabled(bool enabled)
{
    d->cacheEnabled = enabled;
    if (!enabled) {
        clearCache();
    }
}

bool TextExtractor::isCacheEnabled() const
{
    return d->cacheEnabled;
}

void TextExtractor::clearCache()
{
    QMutexLocker locker(&d->cacheMutex);
    d->textCache.clear();
}

qint64 TextExtractor::cacheMemoryUsage() const
{
    return d->calculateCacheMemoryUsage();
}

void TextExtractor::prefetchPages(const QList<int>& pageNumbers)
{
    for (int pageNumber : pageNumbers) {
        d->extractPageTextInternal(pageNumber);
    }
}

void TextExtractor::prefetchRange(int startPage, int endPage)
{
    if (!d->document) {
        return;
    }

    int maxPage = d->document->numPages() - 1;
    startPage = qMax(0, startPage);
    endPage = qMin(maxPage, endPage);

    for (int i = startPage; i <= endPage; ++i) {
        d->extractPageTextInternal(i);
    }
}
