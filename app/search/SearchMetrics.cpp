#include "SearchMetrics.h"
#include <QDebug>
#include <QMutexLocker>
#include <algorithm>

class SearchMetrics::Implementation {
public:
    static const int MAX_HISTORY = 1000;
    static const qint64 SLOW_SEARCH_THRESHOLD = 1000;  // milliseconds

    Implementation(SearchMetrics* q)
        : q_ptr(q), totalCacheHits(0), totalCacheMisses(0) {}

    void addMetric(const Metric& metric) {
        QMutexLocker locker(&mutex);

        metrics.append(metric);
        if (metrics.size() > MAX_HISTORY) {
            metrics.removeFirst();
        }

        // Check for performance issues
        if (metric.duration > SLOW_SEARCH_THRESHOLD) {
            emit q_ptr->performanceWarning(
                QString("Slow search detected: %1ms for query '%2'")
                    .arg(metric.duration)
                    .arg(metric.query));
        }

        emit q_ptr->metricsUpdated();
    }

    double calculateAverage() const {
        QMutexLocker locker(&mutex);

        if (metrics.isEmpty()) {
            return 0.0;
        }

        qint64 total = 0;
        for (const Metric& m : metrics) {
            total += m.duration;
        }

        return static_cast<double>(total) / metrics.size();
    }

    double calculatePercentile(double p) const {
        QMutexLocker locker(&mutex);

        if (metrics.isEmpty()) {
            return 0.0;
        }

        QList<qint64> durations;
        for (const Metric& m : metrics) {
            durations.append(m.duration);
        }

        std::sort(durations.begin(), durations.end());

        int index = static_cast<int>(p * (durations.size() - 1));
        return durations[index];
    }

    SearchMetrics* q_ptr;
    mutable QMutex mutex;
    QList<Metric> metrics;
    QElapsedTimer currentMeasurement;
    qint64 totalCacheHits;
    qint64 totalCacheMisses;
};

SearchMetrics::SearchMetrics(QObject* parent)
    : QObject(parent), d(std::make_unique<Implementation>(this)) {}

SearchMetrics::~SearchMetrics() = default;

void SearchMetrics::startMeasurement() { d->currentMeasurement.start(); }

void SearchMetrics::endMeasurement() {
    // Measurement time is captured when creating the metric
}

void SearchMetrics::recordSearch(const Metric& metric) { d->addMetric(metric); }

void SearchMetrics::recordCacheHit(const QString& query) {
    QMutexLocker locker(&d->mutex);
    d->totalCacheHits++;
    qDebug() << "Cache hit for query:" << query;
}

void SearchMetrics::recordCacheMiss(const QString& query) {
    QMutexLocker locker(&d->mutex);
    d->totalCacheMisses++;
    qDebug() << "Cache miss for query:" << query;
}

double SearchMetrics::averageSearchTime() const {
    return d->calculateAverage();
}

double SearchMetrics::cacheHitRatio() const {
    QMutexLocker locker(&d->mutex);

    qint64 total = d->totalCacheHits + d->totalCacheMisses;
    if (total == 0) {
        return 0.0;
    }

    return static_cast<double>(d->totalCacheHits) / total;
}

double SearchMetrics::incrementalSearchRatio() const {
    QMutexLocker locker(&d->mutex);

    if (d->metrics.isEmpty()) {
        return 0.0;
    }

    int incrementalCount = 0;
    for (const Metric& m : d->metrics) {
        if (m.incremental) {
            incrementalCount++;
        }
    }

    return static_cast<double>(incrementalCount) / d->metrics.size();
}

qint64 SearchMetrics::totalSearches() const {
    QMutexLocker locker(&d->mutex);
    return d->metrics.size();
}

qint64 SearchMetrics::totalCacheHits() const {
    QMutexLocker locker(&d->mutex);
    return d->totalCacheHits;
}

qint64 SearchMetrics::totalCacheMisses() const {
    QMutexLocker locker(&d->mutex);
    return d->totalCacheMisses;
}

QList<SearchMetrics::Metric> SearchMetrics::recentMetrics(int count) const {
    QMutexLocker locker(&d->mutex);

    int start = qMax(0, d->metrics.size() - count);
    return d->metrics.mid(start);
}

QList<SearchMetrics::Metric> SearchMetrics::metricsInRange(
    const QDateTime& start, const QDateTime& end) const {
    QMutexLocker locker(&d->mutex);

    QList<Metric> result;
    for (const Metric& m : d->metrics) {
        if (m.timestamp >= start && m.timestamp <= end) {
            result.append(m);
        }
    }

    return result;
}

void SearchMetrics::clearHistory() {
    QMutexLocker locker(&d->mutex);
    d->metrics.clear();
    d->totalCacheHits = 0;
    d->totalCacheMisses = 0;
    emit metricsUpdated();
}

SearchMetrics::Metric SearchMetrics::fastestSearch() const {
    QMutexLocker locker(&d->mutex);

    if (d->metrics.isEmpty()) {
        return Metric();
    }

    return *std::min_element(d->metrics.begin(), d->metrics.end(),
                             [](const Metric& a, const Metric& b) {
                                 return a.duration < b.duration;
                             });
}

SearchMetrics::Metric SearchMetrics::slowestSearch() const {
    QMutexLocker locker(&d->mutex);

    if (d->metrics.isEmpty()) {
        return Metric();
    }

    return *std::max_element(d->metrics.begin(), d->metrics.end(),
                             [](const Metric& a, const Metric& b) {
                                 return a.duration < b.duration;
                             });
}

double SearchMetrics::percentile(double p) const {
    return d->calculatePercentile(p);
}
