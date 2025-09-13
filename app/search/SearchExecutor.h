#pragma once

#include <QObject>
#include <QRegularExpression>
#include <memory>
#include "SearchConfiguration.h"

class TextExtractor;

/**
 * Search execution component
 * Handles the actual search logic and pattern matching
 */
class SearchExecutor : public QObject
{
    Q_OBJECT

public:
    explicit SearchExecutor(QObject* parent = nullptr);
    ~SearchExecutor();

    // Configuration
    void setTextExtractor(TextExtractor* extractor);
    void setOptions(const SearchOptions& options);

    // Search operations
    QList<SearchResult> searchInPage(int pageNumber, const QString& query);
    QList<SearchResult> searchInPages(const QList<int>& pageNumbers, const QString& query);
    QList<SearchResult> searchInText(const QString& text, const QString& query, int pageNumber = 0);

    // Pattern management
    bool validateQuery(const QString& query) const;
    QRegularExpression createSearchPattern(const QString& query) const;
    QRegularExpression createSearchPattern(const QString& query, const SearchOptions& options) const;

signals:
    void searchProgress(int current, int total);
    void resultFound(const SearchResult& result);
    void searchError(const QString& error);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};
