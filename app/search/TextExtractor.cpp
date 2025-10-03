#include "TextExtractor.h"
#include <poppler-qt6.h>
#include <QDebug>
#include <QMutexLocker>
#include <QRectF>

class TextExtractor::Implementation {
public:
    Implementation(TextExtractor* q)
        : q_ptr(q), document(nullptr), cacheEnabled(true) {}

    QString extractPageTextInternal(int pageNumber) {
        try {
            if (!document || pageNumber < 0 ||
                pageNumber >= document->numPages()) {
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

            // Extract text from page with error handling
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

        } catch (const std::exception& e) {
            QString errorMsg = QString("Text extraction failed for page %1: %2")
                                   .arg(pageNumber)
                                   .arg(e.what());
            emit q_ptr->extractionError(pageNumber, errorMsg);
            return QString();
        } catch (...) {
            QString errorMsg =
                QString("Unknown error during text extraction for page %1")
                    .arg(pageNumber);
            emit q_ptr->extractionError(pageNumber, errorMsg);
            return QString();
        }
    }

    qint64 calculateCacheMemoryUsage() const {
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
    : QObject(parent), m_d(std::make_unique<Implementation>(this)) {}

TextExtractor::~TextExtractor() = default;

void TextExtractor::setDocument(Poppler::Document* document) {
    if (m_d->document != document) {
        clearCache();
        m_d->document = document;
    }
}

void TextExtractor::clearDocument() {
    m_d->document = nullptr;
    clearCache();
}

Poppler::Document* TextExtractor::getDocument() const { return m_d->document; }

QString TextExtractor::extractPageText(int pageNumber) {
    return m_d->extractPageTextInternal(pageNumber);
}

QStringList TextExtractor::extractPagesText(const QList<int>& pageNumbers) {
    QStringList texts;
    int total = pageNumbers.size();
    int current = 0;

    for (int pageNumber : pageNumbers) {
        texts.append(m_d->extractPageTextInternal(pageNumber));
        current++;
        emit extractionProgress(current, total);
    }

    return texts;
}

QString TextExtractor::extractAllText() {
    if (!m_d->document) {
        return QString();
    }

    QString allText;
    int pageCount = m_d->document->numPages();

    for (int i = 0; i < pageCount; ++i) {
        allText.append(m_d->extractPageTextInternal(i));
        allText.append("\n\n");
        emit extractionProgress(i + 1, pageCount);
    }

    return allText;
}

void TextExtractor::setCacheEnabled(bool enabled) {
    m_d->cacheEnabled = enabled;
    if (!enabled) {
        clearCache();
    }
}

bool TextExtractor::isCacheEnabled() const { return m_d->cacheEnabled; }

void TextExtractor::clearCache() {
    QMutexLocker locker(&m_d->cacheMutex);
    m_d->textCache.clear();
}

qint64 TextExtractor::cacheMemoryUsage() const {
    return m_d->calculateCacheMemoryUsage();
}

void TextExtractor::prefetchPages(const QList<int>& pageNumbers) {
    for (int pageNumber : pageNumbers) {
        m_d->extractPageTextInternal(pageNumber);
    }
}

void TextExtractor::prefetchRange(int startPage, int endPage) {
    if (!m_d->document) {
        return;
    }

    int maxPage = m_d->document->numPages() - 1;
    startPage = qMax(0, startPage);
    endPage = qMin(maxPage, endPage);

    for (int i = startPage; i <= endPage; ++i) {
        m_d->extractPageTextInternal(i);
    }
}
