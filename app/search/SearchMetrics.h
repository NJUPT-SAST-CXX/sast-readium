#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QDateTime>
#include <memory>

/**
 * Search performance metrics and monitoring
 */
class SearchMetrics : public QObject
{
    Q_OBJECT

public:
    struct Metric {
        QString query;
        qint64 duration;        // milliseconds
        int resultCount;
        int pagesSearched;
        bool cacheHit;
        bool incremental;
        QDateTime timestamp;
        qint64 memoryUsage;     // bytes
    };

    explicit SearchMetrics(QObject* parent = nullptr);
    ~SearchMetrics();

    // Recording
    void startMeasurement();
    void endMeasurement();
    void recordSearch(const Metric& metric);
    void recordCacheHit(const QString& query);
    void recordCacheMiss(const QString& query);

    // Statistics
    double averageSearchTime() const;
    double cacheHitRatio() const;
    double incrementalSearchRatio() const;
    qint64 totalSearches() const;
    qint64 totalCacheHits() const;
    qint64 totalCacheMisses() const;

    // History
    QList<Metric> recentMetrics(int count = 100) const;
    QList<Metric> metricsInRange(const QDateTime& start, const QDateTime& end) const;
    void clearHistory();

    // Performance analysis
    Metric fastestSearch() const;
    Metric slowestSearch() const;
    double percentile(double p) const;  // e.g., percentile(0.95) for 95th percentile

signals:
    void metricsUpdated();
    void performanceWarning(const QString& message);

private:
    class Implementation;
    std::unique_ptr<Implementation> d;
};
