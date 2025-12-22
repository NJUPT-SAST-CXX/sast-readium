#include "CacheOptimizerPlugin.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

CacheOptimizerPlugin::CacheOptimizerPlugin(QObject* parent)
    : PluginBase(parent),
      m_minSizeToCache(1024),
      m_maxSizeToCache(100 * 1024 * 1024),
      m_priorityBoostForRecent(10),
      m_priorityDecayRate(1),
      m_cacheHits(0),
      m_cacheMisses(0),
      m_evictions(0),
      m_optimizations(0) {
    m_metadata.name = "Cache Optimizer";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "LFU-based cache strategy with persistence and optimization";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();
    m_capabilities.provides = QStringList()
                              << "cache.strategy" << "cache.lfu"
                              << "cache.persistence" << "cache.optimize";
}

CacheOptimizerPlugin::~CacheOptimizerPlugin() {}

bool CacheOptimizerPlugin::onInitialize() {
    m_logger.info("CacheOptimizerPlugin: Initializing...");
    m_minSizeToCache = m_configuration.value("minSizeToCache").toInteger(1024);
    m_maxSizeToCache =
        m_configuration.value("maxSizeToCache").toInteger(100 * 1024 * 1024);
    m_priorityBoostForRecent =
        m_configuration.value("priorityBoostForRecent").toInt(10);
    m_priorityDecayRate = m_configuration.value("priorityDecayRate").toInt(1);
    registerHooks();
    m_logger.info("CacheOptimizerPlugin: Initialized successfully");
    return true;
}

void CacheOptimizerPlugin::onShutdown() {
    m_logger.info("CacheOptimizerPlugin: Shutting down...");
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
    m_logger.info(
        QString("CacheOptimizerPlugin: Hits: %1, Misses: %2, Evictions: %3")
            .arg(m_cacheHits)
            .arg(m_cacheMisses)
            .arg(m_evictions));
}

void CacheOptimizerPlugin::handleMessage(const QString& from,
                                         const QVariant& message) {
    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "get_statistics") {
        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["cacheHits"] = m_cacheHits;
        data["cacheMisses"] = m_cacheMisses;
        data["evictions"] = m_evictions;
        data["optimizations"] = m_optimizations;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);
    } else if (action == "optimize") {
        qint64 currentSize = msgMap.value("currentSize").toLongLong();
        qint64 maxSize = msgMap.value("maxSize").toLongLong();
        int result = optimizeCache(currentSize, maxSize);

        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["optimizationsPerformed"] = result;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);
    }
}

void CacheOptimizerPlugin::registerHooks() {
    auto& registry = PluginHookRegistry::instance();
    registry.registerCallback(
        StandardHooks::CACHE_PRE_ADD, name(),
        [this](const QVariantMap& ctx) { return onCachePreAdd(ctx); });
    registry.registerCallback(
        StandardHooks::CACHE_PRE_EVICT, name(),
        [this](const QVariantMap& ctx) { return onCachePreEvict(ctx); });
    registry.registerCallback(
        StandardHooks::CACHE_OPTIMIZE, name(),
        [this](const QVariantMap& ctx) { return onCacheOptimize(ctx); });
}

void CacheOptimizerPlugin::unregisterHooks() {
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
}

QVariant CacheOptimizerPlugin::onCachePreAdd(const QVariantMap& context) {
    QString key = context.value("key").toString();
    qint64 size = context.value("size").toLongLong();

    bool shouldAdd = shouldCache(key, size, context);
    QVariantMap result;
    result["allow"] = shouldAdd;
    result["reason"] =
        shouldAdd ? "Approved by LFU strategy" : "Size out of range";
    return result;
}

QVariant CacheOptimizerPlugin::onCachePreEvict(const QVariantMap& context) {
    Q_UNUSED(context)
    m_evictions++;
    QVariantMap result;
    result["acknowledged"] = true;
    result["totalEvictions"] = m_evictions;
    return result;
}

QVariant CacheOptimizerPlugin::onCacheOptimize(const QVariantMap& context) {
    qint64 currentSize = context.value("currentSize").toLongLong();
    qint64 maxSize = context.value("maxSize").toLongLong();
    int optimized = optimizeCache(currentSize, maxSize);

    QVariantMap result;
    result["optimizationsPerformed"] = optimized;
    result["totalOptimizations"] = m_optimizations;
    return result;
}

QString CacheOptimizerPlugin::strategyName() const { return "LFU-Optimized"; }

CacheEvictionStrategy CacheOptimizerPlugin::evictionStrategy() const {
    return CacheEvictionStrategy::LFU;
}

bool CacheOptimizerPlugin::shouldCache(const QString& key, qint64 size,
                                       const QVariantMap& metadata) const {
    Q_UNUSED(key)
    Q_UNUSED(metadata)
    return size >= m_minSizeToCache && size <= m_maxSizeToCache;
}

QString CacheOptimizerPlugin::selectEvictionCandidate(
    const QList<CacheEntryMetadata>& entries, qint64 newEntrySize) const {
    Q_UNUSED(newEntrySize)

    if (entries.isEmpty()) {
        return QString();
    }

    // LFU: Find entry with lowest access count
    const CacheEntryMetadata* candidate = &entries.first();
    int lowestScore = calculatePriority(entries.first());

    for (const auto& entry : entries) {
        int score = calculatePriority(entry);
        if (score < lowestScore) {
            lowestScore = score;
            candidate = &entry;
        }
    }

    return candidate->key;
}

int CacheOptimizerPlugin::calculatePriority(
    const CacheEntryMetadata& metadata) const {
    // LFU priority: based on access count with recency boost
    int basePriority = metadata.accessCount * 10;

    // Add recency boost
    qint64 secondsSinceAccess =
        metadata.lastAccessedAt.secsTo(QDateTime::currentDateTime());
    int recencyBoost = qMax(0, m_priorityBoostForRecent -
                                   static_cast<int>(secondsSinceAccess / 3600) *
                                       m_priorityDecayRate);

    // Factor in custom priority if set
    int customPriority = metadata.priority;

    return basePriority + recencyBoost + customPriority;
}

int CacheOptimizerPlugin::optimizeCache(qint64 currentSize, qint64 maxSize) {
    m_optimizations++;

    // Calculate optimization actions
    int actions = 0;

    // If over 90% capacity, suggest aggressive cleanup
    if (currentSize > maxSize * 0.9) {
        actions = 3;  // High priority optimization
    } else if (currentSize > maxSize * 0.7) {
        actions = 1;  // Normal optimization
    }

    m_logger.info(
        QString("CacheOptimizerPlugin: Optimization - %1 actions suggested")
            .arg(actions));
    return actions;
}

bool CacheOptimizerPlugin::persistCache(
    const QString& cachePath, const QList<CacheEntryMetadata>& entries) {
    QFile file(cachePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_logger.error(QString("Failed to open cache file: %1").arg(cachePath));
        return false;
    }

    QJsonArray entriesArray;
    for (const auto& entry : entries) {
        QJsonObject obj;
        obj["key"] = entry.key;
        obj["size"] = entry.size;
        obj["createdAt"] = entry.createdAt.toString(Qt::ISODate);
        obj["lastAccessedAt"] = entry.lastAccessedAt.toString(Qt::ISODate);
        obj["accessCount"] = entry.accessCount;
        obj["priority"] = entry.priority;
        entriesArray.append(obj);
    }

    QJsonObject root;
    root["version"] = "1.0";
    root["strategy"] = strategyName();
    root["entries"] = entriesArray;
    root["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    file.write(QJsonDocument(root).toJson());
    file.close();

    m_logger.info(QString("CacheOptimizerPlugin: Persisted %1 entries")
                      .arg(entries.size()));
    return true;
}

QList<CacheEntryMetadata> CacheOptimizerPlugin::loadCache(
    const QString& cachePath) {
    QList<CacheEntryMetadata> entries;

    QFile file(cachePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_logger.warning(QString("Cache file not found: %1").arg(cachePath));
        return entries;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonArray entriesArray = doc.object()["entries"].toArray();
    for (const auto& val : entriesArray) {
        QJsonObject obj = val.toObject();
        CacheEntryMetadata entry;
        entry.key = obj["key"].toString();
        entry.size = obj["size"].toInteger();
        entry.createdAt =
            QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
        entry.lastAccessedAt = QDateTime::fromString(
            obj["lastAccessedAt"].toString(), Qt::ISODate);
        entry.accessCount = obj["accessCount"].toInt();
        entry.priority = obj["priority"].toInt();
        entries.append(entry);
    }

    m_logger.info(
        QString("CacheOptimizerPlugin: Loaded %1 entries").arg(entries.size()));
    return entries;
}
