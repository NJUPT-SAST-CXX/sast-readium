#include "IncrementalSearchManager.h"
#include <QDebug>

class IncrementalSearchManager::Implementation {
public:
    Implementation(IncrementalSearchManager* q)
        : q_ptr(q), enabled(true), searchDelay(300) {
        timer = new QTimer(q_ptr);
        timer->setSingleShot(true);
        QObject::connect(timer, &QTimer::timeout, q_ptr,
                         &IncrementalSearchManager::onTimerTimeout);
    }

    IncrementalSearchManager* q_ptr;
    QTimer* timer;
    bool enabled;
    int searchDelay;
    QString pendingQuery;
    SearchOptions pendingOptions;
};

IncrementalSearchManager::IncrementalSearchManager(QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this)) {
    // Ensure custom types used in signals are registered for Qt meta-type
    // system
    qRegisterMetaType<SearchOptions>("SearchOptions");
}

IncrementalSearchManager::~IncrementalSearchManager() {
    // Ensure timer won't fire during or after destruction
    if (d && d->timer) {
        d->timer->stop();
        d->timer->disconnect(this);
    }
    d->pendingQuery.clear();
}

void IncrementalSearchManager::setDelay(int milliseconds) {
    d->searchDelay = milliseconds;
    d->timer->setInterval(milliseconds);
}

int IncrementalSearchManager::delay() const { return d->searchDelay; }

void IncrementalSearchManager::setEnabled(bool enabled) {
    d->enabled = enabled;
    if (!enabled) {
        cancelScheduledSearch();
    }
}

bool IncrementalSearchManager::isEnabled() const { return d->enabled; }

void IncrementalSearchManager::scheduleSearch(const QString& query,
                                              const SearchOptions& options) {
    if (!d->enabled) {
        emit searchTriggered(query, options);
        return;
    }

    d->pendingQuery = query;
    d->pendingOptions = options;

    d->timer->stop();
    d->timer->setInterval(d->searchDelay);
    d->timer->start();

    emit searchScheduled();
}

void IncrementalSearchManager::cancelScheduledSearch() {
    if (d->timer->isActive()) {
        d->timer->stop();
        d->pendingQuery.clear();
        emit searchCancelled();
    }
}

bool IncrementalSearchManager::hasScheduledSearch() const {
    return d->timer->isActive();
}

bool IncrementalSearchManager::canRefineSearch(
    const QString& newQuery, const QString& previousQuery) const {
    if (previousQuery.isEmpty() || newQuery.isEmpty()) {
        return false;
    }

    // Can refine if new query is an extension of previous
    if (isQueryExtension(newQuery, previousQuery)) {
        return true;
    }

    // Can also refine if new query is a reduction (for filtering)
    if (isQueryReduction(newQuery, previousQuery)) {
        return true;
    }

    return false;
}

QList<SearchResult> IncrementalSearchManager::refineResults(
    const QList<SearchResult>& previousResults, const QString& newQuery,
    const QString& previousQuery) const {
    QList<SearchResult> refinedResults;

    if (isQueryExtension(newQuery, previousQuery)) {
        // Filter previous results for the extended query
        for (const SearchResult& result : previousResults) {
            if (result.matchedText.contains(newQuery, Qt::CaseInsensitive)) {
                refinedResults.append(result);
            }
        }
    } else if (isQueryReduction(newQuery, previousQuery)) {
        // All previous results should still match
        refinedResults = previousResults;
    }

    return refinedResults;
}

bool IncrementalSearchManager::isQueryExtension(
    const QString& newQuery, const QString& previousQuery) const {
    return !previousQuery.isEmpty() && !newQuery.isEmpty() &&
           newQuery.startsWith(previousQuery);
}

bool IncrementalSearchManager::isQueryReduction(
    const QString& newQuery, const QString& previousQuery) const {
    return !previousQuery.isEmpty() && !newQuery.isEmpty() &&
           previousQuery.startsWith(newQuery);
}

QString IncrementalSearchManager::getCommonPrefix(const QString& query1,
                                                  const QString& query2) const {
    int minLength = qMin(query1.length(), query2.length());
    int commonLength = 0;

    for (int i = 0; i < minLength; ++i) {
        if (query1[i] == query2[i]) {
            commonLength++;
        } else {
            break;
        }
    }

    return query1.left(commonLength);
}

void IncrementalSearchManager::onTimerTimeout() {
    if (!d->pendingQuery.isEmpty()) {
        emit searchTriggered(d->pendingQuery, d->pendingOptions);
        d->pendingQuery.clear();
    }
}
