#pragma once

#include <QObject>
#include <QTimer>
#include <memory>
#include "SearchConfiguration.h"

/**
 * Incremental search management component
 * Handles progressive search refinement and optimization
 */
class IncrementalSearchManager : public QObject {
    Q_OBJECT

public:
    static IncrementalSearchManager& instance() {
        static IncrementalSearchManager instance;
        return instance;
    }

    explicit IncrementalSearchManager(QObject* parent = nullptr);
    ~IncrementalSearchManager();

    // Configuration
    void setDelay(int milliseconds);
    int delay() const;
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Search management
    void scheduleSearch(const QString& query, const SearchOptions& options);
    void cancelScheduledSearch();
    bool hasScheduledSearch() const;

    // Incremental logic
    bool canRefineSearch(const QString& newQuery,
                         const QString& previousQuery) const;
    QList<SearchResult> refineResults(
        const QList<SearchResult>& previousResults, const QString& newQuery,
        const QString& previousQuery) const;

    // Query analysis
    bool isQueryExtension(const QString& newQuery,
                          const QString& previousQuery) const;
    bool isQueryReduction(const QString& newQuery,
                          const QString& previousQuery) const;
    QString getCommonPrefix(const QString& query1, const QString& query2) const;

signals:
    void searchTriggered(const QString& query, const SearchOptions& options);
    void searchScheduled();
    void searchCancelled();

private slots:
    void onTimerTimeout();

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};
